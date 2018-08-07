//
//  kk-event.cc
//
//  Created by 张海龙 on 2018/02/01
//  Copyright © 2018年 kkmofang.cn. All rights reserved.
//

#include "kk-config.h"
#include "kk-event.h"
#include "kk-string.h"

namespace kk {
    
    IMP_SCRIPT_CLASS_BEGIN(nullptr, Event, Event)
    IMP_SCRIPT_CLASS_END
    
    IMP_SCRIPT_CLASS_BEGIN(nullptr, EventEmitter, EventEmitter)
    
        static kk::script::Method methods[] = {
            {"on",(kk::script::Function) &EventEmitter::duk_on},
            {"off",(kk::script::Function) &EventEmitter::duk_off},
            {"emit",(kk::script::Function) &EventEmitter::duk_emit},
            {"has",(kk::script::Function) &EventEmitter::duk_has},
        };
        
        kk::script::SetMethod(ctx, -1, methods, sizeof(methods) / sizeof(kk::script::Method));
    
    IMP_SCRIPT_CLASS_END
    
    
    EventEmitter::~EventEmitter() {
        
    }
    
    void EventEmitter::on(String name,EventFunction * func,EventCFunction cfunc,kk::script::Object * jsfunc,void * context) {
        Strong cb(new EventCallback(name,func,cfunc,jsfunc,context));
        _callbacks.push_back(cb);
    }
    
    void EventEmitter::off(String name,EventFunction * func,EventCFunction cfunc,kk::script::Object * jsfunc,void * context) {
        std::list<Strong>::iterator i = _callbacks.begin();
        while(i != _callbacks.end()) {
            
            EventCallback * cb = (EventCallback *) (* i).get();
            
            if((name == "" || name == cb->name)
               && ( cfunc == nullptr || cfunc == cb->cfunc)
               && ( func == nullptr || func == cb->func.get())
               && ( jsfunc == nullptr || kk::script::ObjectEqual(jsfunc, (kk::script::Object*)cb->jsfunc.get()))
               && ( context == nullptr || context == cb->context)) {
                i = _callbacks.erase(i);
            } else {
                i ++;
            }
            
        }
    }
    
    void EventEmitter::on(String name,EventFunction * func,void * context) {
        on(name,func, nullptr,nullptr, context);
    }
    
    void EventEmitter::off(String name,EventFunction * func,void * context) {
        off(name,func,nullptr,nullptr,context);
    }
    
    void EventEmitter::on(String name,EventCFunction  func,void * context) {
        on(name,nullptr,func,nullptr, context);
    }
    
    void EventEmitter::off(String name,EventCFunction func,void * context) {
        off(name,nullptr,func,nullptr,context);
    }
    
    kk::Boolean EventEmitter::has(String name) {
        
        std::list<Strong>::iterator i = _callbacks.begin();
        
        while(i != _callbacks.end()) {
            
            EventCallback * cb = (EventCallback *) (* i).get();
            
            if(CStringHasSuffix(cb->name.c_str(), "*")) {
                if(cb->name.length() == 1) {
                    return true;
                } else {
                    String prefix = cb->name.substr(0,cb->name.length() -1);
                    if(CStringHasPrefix(name.c_str(), prefix.c_str())) {
                        return true;
                    }
                }
            }
            else if(name == cb->name) {
                return true;
            }
            
            i ++;
            
        }
        
        return false;
    }
    
    void EventEmitter::emit(String name,Event * event) {
        
        std::list<Strong> cbs;
        
        std::list<Strong>::iterator i = _callbacks.begin();
        
        while(i != _callbacks.end()) {
            
            EventCallback * cb = (EventCallback *) (* i).get();
            
            if(CStringHasSuffix(cb->name.c_str(), "*")) {
                if(cb->name.length() == 1) {
                    cbs.push_back(cb);
                } else {
                    String prefix = cb->name.substr(0,cb->name.length() -1);
                    if(CStringHasPrefix(name.c_str(), prefix.c_str())) {
                        cbs.push_back(cb);
                    }
                }
            }
            else if(name == cb->name) {
                cbs.push_back(cb);
            }
            
            i ++;
            
        }
        
        i = cbs.begin();
        
        while(i != cbs.end()) {
            
            EventCallback * cb = (EventCallback *) (* i).get();
            
            {
                EventFunction * func = (EventFunction *) cb->func.get();
                
                if(func) {
                    func->call(this,name, event, cb->context);
                }
            }
            
            {
                EventCFunction func = cb->cfunc;
                
                if(func) {
                    (*func)(this, name.c_str(),event, cb->context);
                }
            }
            
            {
                kk::script::Object * v = (kk::script::Object *) cb->jsfunc.get();
                
                if(v && v->heapptr() && v->jsContext()) {
                    
                    duk_context * ctx = v->jsContext();
                    duk_push_heapptr(ctx, v->heapptr());
                    
                    if(duk_is_function(ctx, -1)) {
                        
                        kk::script::PushObject(ctx, event);
                        duk_push_string(ctx, name.c_str());
                        
                        if(duk_pcall(ctx, 2) != DUK_EXEC_SUCCESS) {
                            kk::script::Error(ctx, -1);
                        }
                        
                        duk_pop(ctx);
                        
                    } else {
                        duk_pop(ctx);
                    }
                }
            }
            
            i ++;
        }
        
    }
    
    duk_ret_t EventEmitter::duk_on(duk_context * ctx) {
        
        CString name = nullptr;
        Strong func;
        
        int top = duk_get_top(ctx);
        
        if(top > 0 && duk_is_string(ctx, -top) ) {
            name = duk_to_string(ctx, - top);
        }
        
        if(top > 1 && duk_is_function(ctx, -top + 1) ) {
            func = new kk::script::Object(kk::script::GetContext(ctx), - top + 1);
        }
        
        if(name && func.get()) {
            on(name, nullptr, nullptr, (kk::script::Object *) func.get(), nullptr);
        }
        
        return 0;
    }
    
    duk_ret_t EventEmitter::duk_off(duk_context * ctx) {
        
        CString name = nullptr;
        Strong func;
        
        int top = duk_get_top(ctx);
        
        if(top > 0 && duk_is_string(ctx, -top) ) {
            name = duk_to_string(ctx, - top);
        }
        
        if(top > 1 && duk_is_function(ctx, -top + 1) ) {
            func = new kk::script::Object(kk::script::GetContext(ctx), - top + 1);
        }
        
        off(name, nullptr, nullptr, (kk::script::Object *) func.get(), nullptr);
        
        return 0;
        
    }
    
    duk_ret_t EventEmitter::duk_emit(duk_context * ctx) {
        
        CString name = nullptr;
        Event * e = nullptr;
        
        int top = duk_get_top(ctx);
        
        if(top > 0 && duk_is_string(ctx, -top) ) {
            name = duk_to_string(ctx, - top);
        }
        
        if(top > 1 && duk_is_object(ctx, -top + 1) ) {
            e = dynamic_cast<Event *>(kk::script::GetObject(ctx, - top + 1));
        }
        
        if(name && e) {
            emit(name, e);
        }
        
        return 0;
    }
    
    duk_ret_t EventEmitter::duk_has(duk_context * ctx) {
        
        CString name = nullptr;
        
        int top = duk_get_top(ctx);
        
        if(top > 0 && duk_is_string(ctx, -top) ) {
            name = duk_to_string(ctx, - top);
        }
    
        if(name) {
            duk_push_boolean(ctx, has(name));
            return 1;
        }
        
        return 0;
    }
    
}
