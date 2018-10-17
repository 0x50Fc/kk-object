//
//  kk-event.h
//
//  Created by 张海龙 on 2018/02/01
//  Copyright © 2018年 kkmofang.cn. All rights reserved.
//

#ifndef kk_event_h
#define kk_event_h

#if defined(KK_PLATFORM_IOS)

#include <KKObject/kk-object.h>
#include <KKObject/kk-script.h>
#include <KKObject/kk-block.h>
#include <list>

#else

#include "kk-object.h"
#include "kk-script.h"
#include "kk-block.h"
#include <list>

#endif


namespace kk {
    
    class EventEmitter;
    
    class Event : public kk::script::HeapObject ,public kk::script::IObject {
    DEF_SCRIPT_CLASS
        
    };
    
    typedef void (*EventFunction) (EventEmitter * emitter,CString name,Event * event,BK_DEF_ARG);

    class EventEmitter : public kk::script::ReflectObject ,public kk::script::IObject {
    public:
        virtual ~EventEmitter();
        virtual void on(String name,EventFunction func,BK_DEF_ARG);
        virtual void off(String name,EventFunction func);
        virtual void emit(String name,Event * event);
        virtual kk::Boolean has(String name);
        
        duk_ret_t duk_on(duk_context * ctx);
        duk_ret_t duk_off(duk_context * ctx);
        duk_ret_t duk_emit(duk_context * ctx);
        duk_ret_t duk_has(duk_context * ctx);
        
        DEF_SCRIPT_CLASS

    protected:
        std::list<Strong> _callbacks;
        std::map<kk::String,kk::Int> _keyCounts;
        std::set<kk::String> _keys;
        std::set<kk::String> _prefixs;
        
        virtual void add(String & name);
        virtual void remove(String & name);
        
    };
    
    
}

#endif /* kk_event_h */
