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
#include <list>

#else

#include "kk-object.h"
#include "kk-script.h"
#include <list>

#endif


namespace kk {
    
    class EventEmitter;
    
    class Event : public Object ,public kk::script::IObject {
    DEF_SCRIPT_CLASS
        
    };
    
    typedef void (*EventCFunction) (EventEmitter * emitter,CString name,Event * event,void * context);
    
    class EventFunction : public Object {
    public:
        virtual void call(EventEmitter * emitter,String & name,Event * event,void * context) = 0;
    };
    
    class EventCallback : public Object {
    public:
        EventCallback(String &name,EventFunction * func,EventCFunction cfunc,kk::script::Object * jsfunc,void * context) :func(func),name(name),context(context),cfunc(cfunc),jsfunc(jsfunc) {}
        EventCallback(EventCallback & cb):func(cb.func),name(cb.name),context(cb.context),cfunc(cb.cfunc),jsfunc(cb.jsfunc) {}
        virtual ~EventCallback() {}
        Strong func;
        Strong jsfunc;
        EventCFunction cfunc;
        String name;
        void * context;
    };
    
    class EventEmitter : public Object ,public kk::script::IObject {
    public:
        virtual ~EventEmitter();
        virtual void on(String name,EventFunction * func,void * context);
        virtual void off(String name,EventFunction * func,void * context);
        virtual void on(String name,EventCFunction func,void * context);
        virtual void off(String name,EventCFunction func,void * context);
        virtual void emit(String name,Event * event);
        virtual kk::Boolean has(String name);
        
        duk_ret_t duk_on(duk_context * ctx);
        duk_ret_t duk_off(duk_context * ctx);
        duk_ret_t duk_emit(duk_context * ctx);
        duk_ret_t duk_has(duk_context * ctx);
        
        DEF_SCRIPT_CLASS
        
    protected:
        virtual void on(String name,EventFunction * func,EventCFunction cfunc,kk::script::Object * jsfunc,void * context);
        virtual void off(String name,EventFunction * func,EventCFunction cfunc,kk::script::Object * jsfunc,void * context);
    protected:
        std::list<Strong> _callbacks;
    };
    
    
}

#endif /* kk_event_h */
