//
//  kk-script-debugger.cc
//  KKObject
//
//  Created by zhanghailong on 2018/9/30.
//  Copyright © 2018年 kkmofang.cn. All rights reserved.
//

#include "kk-config.h"
#include "kk-script.h"
#include "kk-string.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

namespace kk {
    
    namespace script {
        
        class DebuggerClient : public kk::Object {
        public:
            DebuggerClient(Debugger * debugger,int sock):_sock(sock),_debugger(debugger){
                _debugger->retain();
            };
            virtual ~DebuggerClient() {
                if(_sock != -1) {
                    close(_sock);
                }
                _debugger->release();
                kk::Log("[DUK] [DEBUGGER] [CLIENT] [DONE]");
            }
        protected:
            Debugger * _debugger;
            int _sock;
        };
        
        Debugger::Debugger(int port) {
            
            _sock = socket(AF_INET, SOCK_STREAM, 0);
            
            if(_sock < 0) {
                kk::Log("[DUK] [DEBUGGER] [ERROR] %s",strerror(errno));
                return;
            }
            
            int on = 1;
            if (setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on)) < 0) {
                kk::Log("[DUK] [DEBUGGER] [ERROR] %s",strerror(errno));
                close(_sock);
                _sock = -1;
                return;
            }
            
            struct sockaddr_in addr;
            
            memset((void *) &addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(port);
            
            if (bind(_sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
                kk::Log("[DUK] [DEBUGGER] [ERROR] %s",strerror(errno));
                close(_sock);
                _sock = -1;
                return;
            }
            
            if(listen(_sock, 1 ) < 0) {
                close(_sock);
                _sock = -1;
                return;
            }
            
            kk::Log("[DUK] [DEBUGGER] [SERVER] %d",this->port());
            
        }
        
        Debugger::~Debugger() {
            
            if(_sock != -1) {
                close(_sock);
            }
            
            kk::Log("[DUK] [DEBUGGER] [SERVER] [DONE]");
        }
        
        static duk_size_t duk_trans_socket_read_cb(void *udata, char *buffer, duk_size_t length) {
            
            ssize_t ret;
            
            int client_sock = (int) (long) udata;
            
            if (client_sock < 0) {
                return 0;
            }
            
            if (length == 0) {
                kk::Log("[DUK] [DEBUGGER] [ERROR] read request length == 0");
                return 0;
            }
            
            if (buffer == NULL) {
                kk::Log("[DUK] [DEBUGGER] [ERROR] read request buffer == NULL");
                return 0;
            }
           
            ret = read(client_sock, (void *) buffer, (size_t) length);
            
            if (ret < 0) {
                kk::Log("[DUK] [DEBUGGER] [ERROR] debug read failed, closing connection: %s",strerror(errno));
                return 0;
            } else if (ret == 0) {
                kk::Log("[DUK] [DEBUGGER] [ERROR] debug read failed, ret == 0 (EOF)");
                return 0;
            } else if (ret > (ssize_t) length) {
                kk::Log("[DUK] [DEBUGGER] [ERROR] debug read failed, ret too large (%ld > %ld)",(long) ret, (long) length);
                return 0;
            }
            
            return (duk_size_t) ret;
            
        }
        
        duk_size_t duk_trans_socket_write_cb(void *udata, const char *buffer, duk_size_t length) {
            ssize_t ret;
            
            int client_sock = (int) (long) udata;
            
            if (client_sock < 0) {
                return 0;
            }
            
            if (length == 0) {
                kk::Log("[DUK] [DEBUGGER] [ERROR] read request length == 0");
                return 0;
            }
            
            if (buffer == NULL) {
                kk::Log("[DUK] [DEBUGGER] [ERROR] read request buffer == NULL");
                return 0;
            }
            
            ret = write(client_sock, (const void *) buffer, (size_t) length);
            
            if (ret <= 0 || ret > (ssize_t) length) {
                kk::Log("[DUK] [DEBUGGER] [ERROR] debug write failed: %s",strerror(errno));
                return 0;
            }
            
            return (duk_size_t) ret;

        }
        
        duk_size_t duk_trans_socket_peek_cb(void *udata) {

            struct timeval tm;
            fd_set rfds;
            int select_rc;
            
            int client_sock = (int) (long) udata;
            
            if (client_sock < 0) {
                return 0;
            }
            
            FD_ZERO(&rfds);
            FD_SET(client_sock, &rfds);
            tm.tv_sec = tm.tv_usec = 0;
            select_rc = select(client_sock + 1, &rfds, NULL, NULL, &tm);
            if (select_rc == 0) {
                return 0;
            } else if (select_rc == 1) {
                return 1;
            }
            return 0;
        }
        
        void duk_trans_socket_read_flush_cb(void *udata) {
            
        }
        
        void duk_trans_socket_write_flush_cb(void *udata) {
            
        }
        
        static duk_idx_t debugger_request(duk_context *ctx, void *udata, duk_idx_t nvalues) {
            return 0;
        }
        
        static void debugger_detached(duk_context *ctx, void *udata) {
            duk_push_global_object(ctx);
            duk_push_string(ctx, "__debugger");
            duk_push_undefined(ctx);
            duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_CLEAR_WRITABLE | DUK_DEFPROP_CLEAR_CONFIGURABLE | DUK_DEFPROP_SET_CONFIGURABLE);
            duk_pop(ctx);
        }
        
        void Debugger::debug(duk_context * ctx) {
            
            duk_get_global_string(ctx, "__debugger");
            
            if(duk_is_object(ctx, -1)) {
                duk_pop(ctx);
                return;
            }
            
            duk_pop(ctx);
            
            struct sockaddr_in addr;
            socklen_t sz;
            
            if (_sock < 0) {
                kk::Log("[DUK] [DEBUGGER] [ERROR] %s",strerror(errno));
                return;
            }
            
            sz = (socklen_t) sizeof(addr);
            
            int cli = accept(_sock, (struct sockaddr *) &addr, &sz);
            
            if (cli < 0) {
                kk::Log("[DUK] [DEBUGGER] [ERROR] %s",strerror(errno));
                return;
            }
            
            kk::Log("[DUK] [DEBUGGER] [CLIENT] %s:%d ...",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
            
            DebuggerClient * v = new DebuggerClient(this,cli);
            
            duk_push_global_object(ctx);
            duk_push_string(ctx, "__debugger");
            PushObject(ctx, v);
            duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_CLEAR_WRITABLE | DUK_DEFPROP_CLEAR_CONFIGURABLE | DUK_DEFPROP_SET_CONFIGURABLE);
            duk_pop(ctx);
            
            duk_debugger_attach(ctx,
                                duk_trans_socket_read_cb,
                                duk_trans_socket_write_cb,
                                duk_trans_socket_peek_cb,
                                duk_trans_socket_read_flush_cb,
                                duk_trans_socket_write_flush_cb,
                                debugger_request,
                                debugger_detached,
                                (void *) (long) cli);
        }
   
        int Debugger::port() {
            
            if(_sock == -1) {
                return -1;
            }
            
            struct sockaddr_in addr;
            socklen_t len = sizeof(struct sockaddr_in);
            
            if(-1 ==  getsockname(_sock, (struct sockaddr *) &addr, &len)) {
                return -1;
            }
            
            return ntohs(addr.sin_port);
        }
        
    }
    
}

