//
//  kk-http.h
//  app
//
//  Created by zhanghailong on 2018/6/11.
//  Copyright © 2018年 kkmofang.cn. All rights reserved.
//

#ifndef kk_http_h
#define kk_http_h

#if defined(KK_PLATFORM_IOS)

#include <KKObject/kk-script.h>
#include <list>

#else

#include "kk-script.h"
#include <list>

#endif

struct event_base;
struct evdns_base;

namespace kk {

    class HttpTask;
    
    class Http : public kk::script::HeapObject, public kk::script::IObject {
    public:
        Http(struct event_base * base,struct evdns_base * dns);
        virtual ~Http();
        virtual duk_ret_t duk_send(duk_context * ctx);
        
        virtual struct event_base * base();
        virtual struct evdns_base * dns();
        
        DEF_SCRIPT_CLASS
    protected:
        Http();
        struct event_base * _base;
        struct evdns_base * _dns;
        std::map<kk::Uint64,HttpTask *> _tasks;
        kk::Uint64 _autoId;
        
        virtual void remove(HttpTask * httpTask);
        friend class HttpTask;
    };
    
}


#endif /* kk_http_h */
