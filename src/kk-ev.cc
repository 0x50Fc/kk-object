//
//  kk-ev.c
//  app
//
//  Created by zhanghailong on 2018/6/8.
//  Copyright © 2018年 kkmofang.cn. All rights reserved.
//

#include <pthread.h>

#include "kk-config.h"
#include "kk-ev.h"
#include "kk-script.h"

namespace kk {
    
   
    struct evdns_base * ev_dns(duk_context * ctx) {
        
        struct evdns_base * base = nullptr;
        
        duk_get_global_string(ctx, "__evdns_base");
        
        if(duk_is_pointer(ctx, -1)) {
            base = (struct evdns_base *) duk_to_pointer(ctx, -1);
        }
        
        duk_pop(ctx);
        
        return base;
    }
    
    struct event_base * ev_base(duk_context * ctx) {
        
        struct event_base * base = nullptr;
        
        duk_get_global_string(ctx, "__event_base");
        
        if(duk_is_pointer(ctx, -1)) {
            base = (struct event_base *) duk_to_pointer(ctx, -1);
        }
        
        duk_pop(ctx);
        
        return base;
    }
    
    static void ev_Timeout_cb(evutil_socket_t fd, short ev, void * data);
    
    struct timeval NewTimeval(Uint tv) {
        struct timeval v;
        v.tv_sec = tv / 1000;
        v.tv_usec = (tv % 1000) * 1000LL;
        return v;
    }
    
    class EventTimer {
    public:
        
        EventTimer(duk_context * ctx,
                   void * heapptr,
                   struct event_base * base,
                   Uint tv,
                   Uint rv)
            :_ctx(ctx),_rv(rv),
        _isCalling(false),_isRecycled(false),_heapptr(heapptr) {
            
            _event = evtimer_new(base, ev_Timeout_cb, this);
            struct timeval v = NewTimeval(tv);
            evtimer_add(_event, &v);
        }
        
        virtual ~EventTimer() {
            
            evtimer_del(_event);
            event_free(_event);
            
        }
        
        virtual void recycle() {
            
            if(_isRecycled) {
                return;
            }
            
            _isRecycled = true;
            
            if(!_isCalling) {
                delete this;
            }
        }
        
        virtual void call() {
            
            if(_isRecycled) {
                return;
            }
            
            if(_isCalling) {
                return;
            }
            
            duk_context * ctx = _ctx;
            
            _isCalling = true;
            
            duk_push_heapptr(ctx, _heapptr);
            
            if(duk_is_function(ctx, -1)) {
                
                if(duk_pcall(ctx, 0) != DUK_EXEC_SUCCESS) {
                    kk::script::Error(ctx, -1);
                }
                
            }
            
            duk_pop(ctx);
            
            if(_isRecycled) {
                delete this;
                return;
            }
            
            if(_rv != 0) {
                struct timeval tv = NewTimeval(_rv);
                evtimer_add(_event, &tv);
                _isCalling = false;
                return;
            }
            
            duk_push_global_object(ctx);
            duk_push_sprintf(ctx, "__0x%x",(long) this);
            duk_del_prop(ctx, -2);
            duk_pop(ctx);
            
            _isCalling = false;
            
            if(_isRecycled) {
                delete this;
                return;
            }
            
        }
        
    protected:
        Boolean _isCalling;
        Boolean _isRecycled;
        duk_context * _ctx;
        struct event * _event;
        Uint _rv;
        void * _heapptr;
    };
    
    static void ev_Timeout_cb(evutil_socket_t fd, short ev, void * data) {
        EventTimer * v = (EventTimer *) data;
        v->call();
    }
    
    static duk_ret_t ev_Timer_dealloc(duk_context *ctx) {
        
        duk_get_prop_string(ctx, -1, "__object");
        
        if(duk_is_pointer(ctx, -1)) {
            EventTimer * v = (EventTimer *) duk_to_pointer(ctx, -1);
            v->recycle();
        }
        
        duk_pop(ctx);
        
        return 0;
    }
    
    static duk_ret_t ev_newTimer(duk_context * ctx, void * fn, Uint tv, Uint rv) {
        
        event_base * base = ev_base(ctx);
        
        EventTimer * v = new EventTimer(ctx,fn,base,tv,rv);
        
        duk_push_global_object(ctx);
        
        duk_push_sprintf(ctx, "__0x%x",(long) v);
        duk_push_object(ctx);
        {
            duk_push_string(ctx, "__object");
            duk_push_pointer(ctx, v);
            duk_put_prop(ctx, -3);
            
            duk_push_string(ctx, "fn");
            duk_push_heapptr(ctx, fn);
            duk_put_prop(ctx, -3);
            
            duk_push_c_function(ctx, ev_Timer_dealloc, 1);
            duk_set_finalizer(ctx, -2);
        }
        duk_put_prop(ctx, -3);
        
        duk_pop(ctx);
        
        duk_push_sprintf(ctx, "__0x%x",(long) v);
        
        return 1;
        
    }

    static duk_ret_t ev_setTimeout(duk_context * ctx) {
        
        int top = duk_get_top(ctx);
        
        if(top > 0 && duk_is_function(ctx, -top) ) {
            
            void * fn = duk_get_heapptr(ctx, -top);
            
            Uint tv = 0;
            
            if(top > 1) {
                if(duk_is_number(ctx, -top +1)) {
                    tv = duk_to_uint(ctx, -top + 1);
                } else if(duk_is_string(ctx, -top + 1)) {
                    tv = (Uint) atol(duk_to_string(ctx, -top  +1));
                }
            }
            
            return ev_newTimer(ctx,fn,tv,0);
        }
        
        return 0;
    }
    
    static duk_ret_t ev_setInterval(duk_context * ctx) {
        
        int top = duk_get_top(ctx);
        
        if(top > 0 && duk_is_function(ctx, -top) ) {
            
            void * fn = duk_get_heapptr(ctx, -top);
            
            Uint tv = 0;
            
            if(top > 1) {
                if(duk_is_number(ctx, -top +1)) {
                    tv = duk_to_uint(ctx, -top + 1);
                } else if(duk_is_string(ctx, -top + 1)) {
                    tv = (Uint) atoll(duk_to_string(ctx, -top  +1));
                }
            }
            
            return ev_newTimer(ctx,fn,tv,tv);
        }
        
        return 0;
    }
    
    static duk_ret_t ev_clear(duk_context * ctx) {
        
        int top = duk_get_top(ctx);
        
        if(top > 0 && duk_is_string(ctx, -top) ) {
            
            duk_push_global_object(ctx);
            duk_dup(ctx, -top -1);
            duk_del_prop(ctx, -2);
            duk_pop(ctx);
            
        }
        
        return 0;
    }
    
    
    void ev_openlibs(duk_context * ctx,event_base * base, evdns_base * dns) {
        
        duk_push_global_object(ctx);
        
        duk_push_string(ctx, "__event_base");
        duk_push_pointer(ctx, base);
        duk_put_prop(ctx, -3);
        
        duk_push_string(ctx, "__evdns_base");
        duk_push_pointer(ctx, dns);
        duk_put_prop(ctx, -3);
        
        duk_push_string(ctx, "setTimeout");
        duk_push_c_function(ctx, ev_setTimeout, 2);
        duk_put_prop(ctx, -3);
        
        duk_push_string(ctx, "clearTimeout");
        duk_push_c_function(ctx, ev_clear, 1);
        duk_put_prop(ctx, -3);
        
        duk_push_string(ctx, "setInterval");
        duk_push_c_function(ctx, ev_setInterval, 2);
        duk_put_prop(ctx, -3);
        
        duk_push_string(ctx, "clearInterval");
        duk_push_c_function(ctx, ev_clear, 1);
        duk_put_prop(ctx, -3);
        
        duk_pop(ctx);
        
    }
    
}
