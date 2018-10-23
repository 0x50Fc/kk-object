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
    
    
    class EventCallback : public Object {
    public:
        EventCallback(String &name,EventFunction func,BK_DEF_ARG):name(name),func(func),ctx(__BK_CTX){}
        kk::Strong ctx;
        EventFunction func;
        String name;
    };
    
    
    EventEmitter::~EventEmitter() {
        
    }
    
    void EventEmitter::on(String name,EventFunction func,BK_DEF_ARG) {
        _callbacks.push_back(new EventCallback(name,func,__BK_CTX));
        add(name);
    }
    
    void EventEmitter::off(String name,EventFunction func) {
        std::list<Strong>::iterator i = _callbacks.begin();
        while(i != _callbacks.end()) {
            EventCallback * cb = (EventCallback *) (* i).get();
            if((name == "" || name == cb->name)
               && ( func == nullptr || func == cb->func)) {
                i = _callbacks.erase(i);
                remove(cb->name);
            } else {
                i ++;
            }
        }
    }
    
    void EventEmitter::add(String & name) {
        std::map<kk::String,kk::Int>::iterator i = _keyCounts.find(name);
        if(i == _keyCounts.end()) {
            _keyCounts[name] = 1;
            if(kk::CStringHasSuffix(name.c_str(), "*")) {
                _prefixs.insert(name.substr(0,name.length() - 1));
            } else {
                _keys.insert(name);
            }
        } else {
            _keyCounts[name] = i->second + 1;
        }
    }
    
    void EventEmitter::remove(String & name) {
        std::map<kk::String,kk::Int>::iterator i = _keyCounts.find(name);
        if(i != _keyCounts.end()) {
            if(i->second <= 1) {
                _keyCounts.erase(i);
                if(kk::CStringHasSuffix(name.c_str(), "*")) {
                    std::set<kk::String>::iterator n = _prefixs.find(name.substr(0,name.length() - 1));
                    if(n != _prefixs.end()) {
                        _prefixs.erase(n);
                    }
                } else {
                    std::set<kk::String>::iterator n = _keys.find(name);
                    if(n != _keys.end()) {
                        _keys.erase(n);
                    }
                }
            }
        }
    }
    
    kk::Boolean EventEmitter::has(String name) {
        
        if(_keys.find(name) != _keys.end()) {
            return true;
        }
        
        {
            std::set<kk::String>::iterator i = _prefixs.begin();
            
            while(i != _prefixs.end()) {
                
                kk::String n = * i;
                
                if(n.length() == 0) {
                    return true;
                }
                
                if(CStringHasPrefix(name.c_str(), n.c_str())) {
                    return true;
                }
                
                i ++;
                
            }
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
            
            EventFunction func = cb->func;
            
            if(func) {
                (*func)(this, name.c_str(),event, cb->ctx.as<BlockContext>());
            }
        
            i ++;
        }
        
        {
            std::map<duk_context *,void *>::iterator i = _heapptrs.begin();
            
            while(i != _heapptrs.end()) {
                
                duk_context * ctx = i->first;
                void * heapptr = i->second;
                
                duk_push_array(ctx);
                
                duk_uarridx_t idx = 0;
                
                duk_push_heapptr(ctx, heapptr);
                
                duk_get_prop_string(ctx, -1, "__events");
                
                if(duk_is_array(ctx, -1)) {
                    
                    duk_enum(ctx, -1, DUK_ENUM_ARRAY_INDICES_ONLY);
                    
                    while(duk_next(ctx, -1, 1)) {
                        
                        duk_get_prop_index(ctx, -1, 0);
                        kk::CString n = duk_to_string(ctx, -1);
                        duk_pop(ctx);
                        
                        kk::Boolean b = false;
                        
                        if(CStringHasSuffix(n, "*")) {
                            size_t len = CStringLength(n);
                            if(len == 1) {
                                b = true;
                            } else {
                                String prefix(n,0,len -1);
                                if(CStringHasPrefix(name.c_str(), prefix.c_str())) {
                                    b = true;
                                }
                            }
                        }
                        else if(CStringEqual(name.c_str(), n)) {
                            b = true;
                        }
                        
                        if(b) {
                            duk_get_prop_index(ctx, -1, 1);
                            duk_put_prop_index(ctx, -7, idx ++);
                        }
                        
                        duk_pop_2(ctx);
                    }
                    
                    duk_pop(ctx);
                }
                
                duk_pop_2(ctx);
                
                duk_enum(ctx, -1, DUK_ENUM_ARRAY_INDICES_ONLY);
                
                while(duk_next(ctx, -1, 1)) {
                    
                    kk::script::PushObject(ctx, event);
                    duk_push_string(ctx, name.c_str());
                    
                    if(duk_pcall(ctx, 2) != DUK_EXEC_SUCCESS) {
                        kk::script::Error(ctx, -1);
                    }
                    
                    duk_pop_2(ctx);
                }
                
                duk_pop_2(ctx);
                
                i ++;
            }
        }
    }
    
    duk_ret_t EventEmitter::duk_on(duk_context * ctx) {
        
        CString name = nullptr;
        void * func = nullptr;
        
        int top = duk_get_top(ctx);
        
        if(top > 0 && duk_is_string(ctx, -top) ) {
            name = duk_to_string(ctx, - top);
        }
        
        if(top > 1 && duk_is_function(ctx, -top + 1) ) {
            func = duk_get_heapptr(ctx, -top + 1);
        }
        
        if(name && func) {
            
            {
                String n = name;
                add(n);
            }
            
            duk_push_this(ctx);
            
            if(!duk_is_object(ctx, -1)) {
                duk_pop(ctx);
                std::map<duk_context *,void *>::iterator i = _heapptrs.find(ctx);
                if(i == _heapptrs.end()) {
                    return 0;
                }
                duk_push_heapptr(ctx, i->second);
            }
            
            duk_get_prop_string(ctx, -1, "__events");
            
            if(!duk_is_array(ctx, -1)) {
                duk_pop(ctx);
                duk_push_array(ctx);
                duk_push_string(ctx, "__events");
                duk_dup(ctx, -2);
                duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_CLEAR_ENUMERABLE | DUK_DEFPROP_CLEAR_WRITABLE | DUK_DEFPROP_CLEAR_CONFIGURABLE);
            }
            
            duk_uarridx_t n = (duk_uarridx_t) duk_get_length(ctx, -1);
            
            duk_push_array(ctx);
            
            {
                duk_push_string(ctx, name);
                duk_put_prop_index(ctx, -2, 0);
                duk_push_heapptr(ctx, func);
                duk_put_prop_index(ctx, -2, 1);
            }
            
            duk_put_prop_index(ctx, -2, n);
            
            duk_pop_2(ctx);
            
        }
        
        return 0;
    }
    
    duk_ret_t EventEmitter::duk_off(duk_context * ctx) {
        
        CString name = nullptr;
        void * func = nullptr;
        
        int top = duk_get_top(ctx);
        
        if(top > 0 && duk_is_string(ctx, -top) ) {
            name = duk_to_string(ctx, - top);
        }
        
        if(top > 1 && duk_is_function(ctx, -top + 1) ) {
            func = duk_get_heapptr(ctx, -top + 1);
        }
        
        duk_push_this(ctx);
        
        if(!duk_is_object(ctx, -1)) {
            duk_pop(ctx);
            std::map<duk_context *,void *>::iterator i = _heapptrs.find(ctx);
            if(i == _heapptrs.end()) {
                return 0;
            }
            duk_push_heapptr(ctx, i->second);
        }
        
        duk_get_prop_string(ctx, -1, "__events");
        
        if(duk_is_array(ctx, -1)) {
            
            duk_uarridx_t n = (duk_uarridx_t) duk_get_length(ctx, -1) ;
            
            while(n > 0) {
                
                n --;
                
                duk_get_prop_index(ctx, -1, n);
                
                duk_get_prop_index(ctx, -1, 0);
                kk::CString na = duk_to_string(ctx, -1);
                duk_pop(ctx);
                
                if(name != nullptr && !CStringEqual(name, na)) {
                    duk_pop(ctx);
                    continue;
                }
                
                if(func != nullptr) {
                    duk_get_prop_index(ctx, -1, 1);
                    if(func != duk_get_heapptr(ctx, -1)) {
                        duk_pop_2(ctx);
                        continue;
                    }
                }
                
                duk_pop(ctx);
                duk_del_prop_index(ctx, -1, n);
                
                {
                    String sn = na;
                    remove(sn);
                }
                
            }
            
        }
        
        duk_pop_2(ctx);
        
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
