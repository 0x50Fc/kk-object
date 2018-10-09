//
//  kk-script.c
//  KKGame
//
//  Created by zhanghailong on 2018/2/5.
//  Copyright © 2018年 kkmofang.cn. All rights reserved.
//

#include "kk-config.h"
#include "kk-script.h"
#include "kk-string.h"

namespace kk {
    
    namespace script {
        
        static void Context_fatal_function (void *udata, const char *msg) {
            kk::Log("%s",msg);
        }
        
        static duk_ret_t Context_print_function (duk_context * ctx) {
            
            int top = duk_get_top(ctx);
            
            for(int i=0;i<top;i++) {
                
                if(duk_is_string(ctx, - top + i)) {
                    kk::Log("%s",duk_to_string(ctx, - top + i));
                } else if(duk_is_number(ctx, - top + i)) {
                    kk::Log("%g",duk_to_number(ctx, - top + i));
                } else if(duk_is_boolean(ctx, - top + i)) {
                    kk::Log("%s",duk_to_boolean(ctx, - top + i) ? "true":"false");
                } else if(duk_is_buffer_data(ctx, - top + i)) {
                    kk::Log("[bytes]:");
                    {
                        size_t n;
                        unsigned char * bytes = (unsigned char *) duk_get_buffer_data(ctx, - top + i, &n);
                        while(n >0) {
                            printf("%u",*bytes);
                            bytes ++;
                            n --;
                            if(n != 0) {
                                printf(",");
                            }
                        }
                        printf("\n");
                    }
                } else if(duk_is_function(ctx, - top + i)) {
                    kk::Log("[function]");
                } else if(duk_is_undefined(ctx, - top + i)) {
                    kk::Log("[undefined]");
                } else if(duk_is_null(ctx, - top + i)) {
                    kk::Log("[null]");
                } else {
                    kk::Log("%s",duk_json_encode(ctx, - top + i));
                    duk_pop(ctx);
                }
            
            }
            
            return 0;
        }
    
        Context::Context() {
            
            _jsContext = duk_create_heap(nullptr, nullptr, nullptr, nullptr, Context_fatal_function);
            
            duk_push_global_object(_jsContext);
            
            duk_push_string(_jsContext, "__jsContext");
            duk_push_pointer(_jsContext, this);
            duk_def_prop(_jsContext, -3,DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_CLEAR_WRITABLE | DUK_DEFPROP_CLEAR_CONFIGURABLE | DUK_DEFPROP_CLEAR_ENUMERABLE);
            
            duk_push_string(_jsContext, "print");
            duk_push_c_function(_jsContext, Context_print_function, DUK_VARARGS);
            duk_put_prop(_jsContext, -3);

            duk_pop(_jsContext);
            
            OpenlibWeakMap(_jsContext);
            OpenlibMap(_jsContext);
            
        }
        
        Context::~Context() {
            
            std::map<kk::String,kk::Object *>::iterator i = _objects.begin();
            
            while(i != _objects.end()) {
                i->second->release();
                i ++;
            }
            
            duk_destroy_heap(_jsContext);
            
        }
        
        void debugger(int port) {
            
        }
        
        duk_context * Context::jsContext() {
            return _jsContext;
        }
        
        kk::Object * Context::object(kk::CString key) {
            std::map<kk::String,kk::Object *>::iterator i = _objects.find(key);
            if(i != _objects.end()){
                return i->second;
            }
            return nullptr;
        }
        
        void Context::setObject(kk::CString key,kk::Object * object) {
            if(object == nullptr) {
                std::map<kk::String,kk::Object *>::iterator i = _objects.find(key);
                if(i != _objects.end()){
                    i->second->release();
                    _objects.erase(i);
                }
            } else {
                
                object->retain();
                
                std::map<kk::String,kk::Object *>::iterator i = _objects.find(key);
                
                if(i != _objects.end()){
                    i->second->release();
                }
                _objects[key] = object;
            }
        }
        
        Object::Object(Context * context,duk_idx_t idx){
            duk_context * ctx = context->jsContext();
            _context = context;
            _heapptr = duk_get_heapptr(ctx, idx);
            if(_heapptr) {
                duk_push_global_object(ctx);
                duk_push_sprintf(ctx, "0x%x",_heapptr);
                duk_push_heapptr(ctx, _heapptr);
                duk_put_prop(ctx, -3);
                duk_pop(ctx);
            }
        }
        
        Object::~Object() {
            
            Context * context = _context.as<Context>();
            
            if(context && _heapptr) {
                duk_context * ctx = context->jsContext();
                if(ctx) {
                    duk_push_global_object(ctx);
                    duk_push_sprintf(ctx, "0x%x",_heapptr);
                    duk_del_prop(ctx, -2);
                    duk_pop(ctx);
                }
            }

        }
        
        Context * Object::context() {
            return _context.as<Context>();
        }
        
        duk_context * Object::jsContext() {
            
            Context * context = _context.as<Context>();
            
            if(context) {
                return context->jsContext();
            }
            return nullptr;
        }
        
        void * Object::heapptr() {
            return _heapptr;
        }


        HeapObject::HeapObject() {

        }

        HeapObject::~HeapObject() {

        }

        void HeapObject::setHeapptr(void * heapptr,duk_context * ctx) {
            _heapptrs[ctx] = heapptr;
        }
        
        void * HeapObject::heapptr(duk_context * ctx) {
            std::map<duk_context *, void *>::iterator i = _heapptrs.find(ctx);
            if(i != _heapptrs.end()) {
                return i->second;
            }
            return nullptr;
        }
        
        void HeapObject::removeHeapptr(duk_context * ctx) {
            std::map<duk_context *, void *>::iterator i = _heapptrs.find(ctx);
            if(i != _heapptrs.end()) {
                _heapptrs.erase(i);
            }
        }
        
        Context * GetContext(duk_context * jsContext) {
            
            Context * v = nullptr;
            
            duk_push_global_object(jsContext);
            
            duk_push_string(jsContext, "__jsContext");
            duk_get_prop(jsContext, -2);
            
            if(duk_is_pointer(jsContext, -1)) {
                v = (Context *) duk_to_pointer(jsContext, -1);
            }
            
            duk_pop_n(jsContext, 2);
            
            return v;
        }
        
        static duk_ret_t ScriptObjectDeallocFunc(duk_context * ctx) {
            
            kk::Object * v = nullptr;
            
            duk_push_string(ctx, "__object");
            duk_get_prop(ctx, -2);
            
            if(duk_is_pointer(ctx, -1)) {
                v = (kk::Object *) duk_to_pointer(ctx, -1);
            }
            
            duk_pop(ctx);
            
            if(v != nullptr) {
                IHeapObject * vv = dynamic_cast<IHeapObject *>(v);
                if(vv) {
                    vv->removeHeapptr(ctx);
                }
                v->release();
            }
            
            return 0;
        }
        
        void PushObject(duk_context * ctx,kk::Object * object) {
            
            if(object == nullptr) {
                duk_push_null(ctx);
                return;
            }
            
            {
                Object * v = dynamic_cast<Object *>(object);
                if(v) {
                    duk_push_heapptr(ctx, v->heapptr());
                    return;
                }
            }
            
            {
                IHeapObject * v = dynamic_cast<IHeapObject *>(object);
                if(v) {
                    void * heapptr = v->heapptr(ctx);
                    if(heapptr == nullptr) {
                        duk_push_object(ctx);
                        heapptr = duk_get_heapptr(ctx, -1);
                        InitObject(ctx, -1, object);
                        v->setHeapptr(heapptr, ctx);
                    } else {
                        duk_push_heapptr(ctx, heapptr);
                    }
                    return;
                }
            }
            
            duk_push_object(ctx);
            InitObject(ctx, -1, object);
        }
        
        void InitObject(duk_context * ctx,duk_idx_t idx,kk::Object * object) {
            
            duk_push_string(ctx, "__object");
            duk_push_pointer(ctx, object);
            duk_def_prop(ctx, idx - 2, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_CLEAR_WRITABLE | DUK_DEFPROP_CLEAR_CONFIGURABLE | DUK_DEFPROP_CLEAR_ENUMERABLE);
            
            object->retain();
            
            if(GetPrototype(ctx, object)) {
                duk_set_prototype(ctx, idx - 1);
            }
            
            duk_push_c_function(ctx, ScriptObjectDeallocFunc, 1);
            duk_set_finalizer(ctx, idx - 1);
            
        }
        
        kk::Object * GetObject(duk_context * ctx,duk_idx_t idx) {
            
            kk::Object * v = nullptr;
            
            duk_push_string(ctx, "__object");
            duk_get_prop(ctx, idx -1 );
            
            if(duk_is_pointer(ctx, -1)) {
                v = (kk::Object *) duk_to_pointer(ctx, -1);
            }
            
            duk_pop(ctx);
            
            return v;
        }
        
        bool ObjectEqual(Object * a, Object * b) {
            if(a == b) {
                return true;
            }
            if(a == nullptr || b == nullptr) {
                return false;
            }
            return a->heapptr() == b->heapptr();
        }
        
        void SetPrototype(duk_context * ctx,Class * isa) {
            
            duk_push_global_object(ctx);
            
            duk_push_string(ctx, isa->name);
            duk_push_object(ctx);
            
            if(isa->alloc) {
                duk_push_string(ctx, "alloc");
                duk_push_c_function(ctx, isa->alloc, 0);
                duk_put_prop(ctx, -3);
            }
            
            if(isa->prototype) {
                (*isa->prototype)(ctx);
            }
            
            if(isa->isa) {
                if(GetPrototype(ctx, isa->isa)) {
                    duk_set_prototype(ctx, -2);
                }
            }
            
            duk_put_prop(ctx, -3);
            
            duk_pop(ctx);
            
        }
        
        bool GetPrototype(duk_context * ctx,Class * isa) {
            
            if(isa == nullptr) {
                return false;
            }
            
            duk_push_global_object(ctx);
            
            duk_push_string(ctx, isa->name);
            duk_get_prop(ctx, -2);
            
            if(duk_is_object(ctx, -1)) {
                duk_remove(ctx, -2);
                return true;
            }
            
            duk_pop(ctx);
            
            duk_push_object(ctx);
            
            if(isa->alloc) {
                duk_push_string(ctx, "alloc");
                duk_push_c_function(ctx, isa->alloc, 0);
                duk_put_prop(ctx, -3);
            }
            
            if(isa->prototype) {
                (*isa->prototype)(ctx);
            }
            
            if(isa->isa) {
                if(GetPrototype(ctx, isa->isa)) {
                    duk_set_prototype(ctx, -2);
                }
            }
            
            duk_push_string(ctx, isa->name);
            duk_dup(ctx, -2);
            
            duk_put_prop(ctx, -4);
            
            duk_remove(ctx, -2);
            return true;
        }
        
        bool GetPrototype(duk_context * ctx,kk::Object * object) {
            
            if(object == nullptr) {
                return false;
            }
            
            IObject * v = dynamic_cast<IObject*>(object);
            
            if(v == nullptr) {
                return false;
            }
            
            return GetPrototype(ctx,v->getScriptClass());
        }
        
        static duk_ret_t ScriptObjectGetterFunc(duk_context * ctx) {
            
            Property * p = nullptr;
            
            duk_push_current_function(ctx);
            
            duk_push_string(ctx, "__object");
            
            duk_get_prop(ctx, -2);
            
            if(duk_is_pointer(ctx, -1)) {
                p = (Property *) duk_to_pointer(ctx, -1);
            }
            
            duk_pop_n(ctx, 2);
            
            duk_push_this(ctx);
            
            kk::Object * object = GetObject(ctx, -1);
            
            duk_pop(ctx);
            
            if(object && p && p->getter) {
                return (object->*p->getter)(ctx);
            }
            
            return 0;
        }
        
        static duk_ret_t ScriptObjectSetterFunc(duk_context * ctx) {
            
            Property * p = nullptr;
            
            duk_push_current_function(ctx);
            
            duk_push_string(ctx, "__object");
            
            duk_get_prop(ctx, -2);
            
            if(duk_is_pointer(ctx, -1)) {
                p = (Property *) duk_to_pointer(ctx, -1);
            }
            
            duk_pop_n(ctx, 2);
            
            duk_push_this(ctx);
            
            kk::Object * object = GetObject(ctx, -1);
            
            duk_pop(ctx);
            
            if(object && p && p->setter) {
                return (object->*p->setter)(ctx);
            }
            
            return 0;
        }
        
        void SetProperty(duk_context * ctx, duk_idx_t idx, Property * propertys, kk::Uint count) {
            Property * p = propertys;
            kk::Uint c = count;
            while(c >0 && p) {
                
                duk_push_string(ctx, p->name);
                
                duk_push_c_function(ctx, ScriptObjectGetterFunc, 0);
                duk_push_string(ctx, "__object");
                duk_push_pointer(ctx, p);
                duk_def_prop(ctx, -3,
                             DUK_DEFPROP_HAVE_VALUE |
                             DUK_DEFPROP_HAVE_WRITABLE | DUK_DEFPROP_CLEAR_WRITABLE |
                             DUK_DEFPROP_HAVE_ENUMERABLE);
                
                duk_push_c_function(ctx, ScriptObjectSetterFunc, 1);
                duk_push_string(ctx, "__object");
                duk_push_pointer(ctx, p);
                duk_def_prop(ctx, -3,
                             DUK_DEFPROP_HAVE_VALUE |
                             DUK_DEFPROP_HAVE_WRITABLE | DUK_DEFPROP_CLEAR_WRITABLE |
                             DUK_DEFPROP_HAVE_ENUMERABLE);
                
                duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER |
                             DUK_DEFPROP_HAVE_ENUMERABLE | DUK_DEFPROP_SET_ENUMERABLE);
                
                c --;
                p ++;
            }
        }
        
        static duk_ret_t ScriptObjectInvokeFunc(duk_context * ctx) {
            
            Method * p = nullptr;
            
            duk_push_current_function(ctx);
            
            duk_push_string(ctx, "__object");
            
            duk_get_prop(ctx, -2);
            
            if(duk_is_pointer(ctx, -1)) {
                p = (Method *) duk_to_pointer(ctx, -1);
            }
            
            duk_pop_n(ctx, 2);
            
            duk_push_this(ctx);
            
            kk::Object * object= GetObject(ctx, -1);
            
            duk_pop(ctx);
            
            if(object && p && p->invoke) {
                kk::script::Function fn = p->invoke;
                return (object->*fn)(ctx);
            }
            
            return 0;
        }
        
        
        void SetMethod(duk_context * ctx, duk_idx_t idx, Method * methods, kk::Uint count) {
            
            Method * p = methods;
            Uint c = count;
            while(c >0 && p) {
                
                duk_push_string(ctx, p->name);
                
                duk_push_c_function(ctx, ScriptObjectInvokeFunc, DUK_VARARGS);
                duk_push_string(ctx, "__object");
                duk_push_pointer(ctx, p);
                duk_def_prop(ctx, -3,
                             DUK_DEFPROP_HAVE_VALUE |
                             DUK_DEFPROP_HAVE_WRITABLE | DUK_DEFPROP_CLEAR_WRITABLE |
                             DUK_DEFPROP_HAVE_ENUMERABLE);
                
                duk_def_prop(ctx, -3,
                             DUK_DEFPROP_HAVE_VALUE |
                             DUK_DEFPROP_HAVE_WRITABLE | DUK_DEFPROP_CLEAR_WRITABLE |
                             DUK_DEFPROP_HAVE_ENUMERABLE);
                
                c --;
                p ++;
            }
            
        }
        
        void Error(duk_context * ctx, duk_idx_t idx) {
            Error(ctx,idx,"");
        }
        
        void Error(duk_context * ctx, duk_idx_t idx, kk::CString prefix) {
            if(duk_is_error(ctx, idx)) {
                duk_get_prop_string(ctx, idx, "lineNumber");
                int lineNumber = duk_to_int(ctx, -1);
                duk_pop(ctx);
                duk_get_prop_string(ctx, idx, "stack");
                const char * error = duk_to_string(ctx, -1);
                duk_pop(ctx);
                duk_get_prop_string(ctx, idx, "fileName");
                const char * fileName = duk_to_string(ctx, -1);
                duk_pop(ctx);
                kk::Log("%s%s(%d): %s",prefix,fileName,lineNumber,error);
            } else {
                kk::Log("%s%s",prefix,duk_to_string(ctx, idx));
            }
        }
        
        static duk_ret_t decodeJSON_func (duk_context *ctx, void *udata) {
            duk_json_decode(ctx, -1);
            return 1;
        }
        
        duk_int_t decodeJSON(duk_context * ctx, kk::CString text, size_t n) {
            duk_push_lstring(ctx, text, n);
            return duk_safe_call(ctx, decodeJSON_func, nullptr, 1, 1);
        }
        
        static duk_ret_t compile_func (duk_context *ctx, void *udata) {
            duk_compile_string_filename(ctx, 0, (kk::CString) udata);
            return 1;
        }
        
        void compile(duk_context * ctx, kk::CString code , kk::CString filename) {
            duk_push_string(ctx, filename);
            duk_safe_call(ctx, compile_func, (void *) code, 1, 1);
        }
        
        kk::String toString(duk_context * ctx, duk_idx_t idx) {
            if(duk_is_string(ctx, idx)) {
                return duk_to_string(ctx, idx);
            } else if(duk_is_number(ctx, idx)) {
                char fmt[255];
                snprintf(fmt, sizeof(fmt), "%g",duk_to_number(ctx, idx));
                return fmt;
            } else if(duk_is_boolean(ctx, idx)) {
                return duk_to_boolean(ctx, idx) ? "true":"false";
            }
            return "";
        }
        
        kk::CString toCString(duk_context * ctx, duk_idx_t idx) {
            if(duk_is_string(ctx, idx)) {
                return duk_to_string(ctx, idx);
            }
            return nullptr;
        }
        
        kk::Double toDouble(duk_context * ctx, duk_idx_t idx) {
            if(duk_is_string(ctx, idx)) {
                return atof(duk_to_string(ctx, idx));
            } else if(duk_is_number(ctx, idx)) {
                return duk_to_number(ctx, idx);
            } else if(duk_is_boolean(ctx, idx)) {
                return duk_to_boolean(ctx, idx) ? 1 : 0;
            }
            return 0;
        }
        
        kk::Int toInt(duk_context * ctx, duk_idx_t idx) {
            if(duk_is_string(ctx, idx)) {
                return atoi(duk_to_string(ctx, idx));
            } else if(duk_is_number(ctx, idx)) {
                return duk_to_int(ctx, idx);
            } else if(duk_is_boolean(ctx, idx)) {
                return duk_to_boolean(ctx, idx) ? 1 : 0;
            }
            return 0;
        }
        
        kk::Uint toUint(duk_context * ctx, duk_idx_t idx) {
            if(duk_is_string(ctx, idx)) {
                return (kk::Uint) atol(duk_to_string(ctx, idx));
            } else if(duk_is_number(ctx, idx)) {
                return duk_to_uint(ctx, idx);
            } else if(duk_is_boolean(ctx, idx)) {
                return duk_to_boolean(ctx, idx) ? 1 : 0;
            }
            return 0;
        }
        
        kk::Boolean toBoolean(duk_context * ctx, duk_idx_t idx) {
            if(duk_is_string(ctx, idx)) {
                kk::CString v = duk_to_string(ctx, idx);
                return CStringEqual(v, "true");
            } else if(duk_is_number(ctx, idx)) {
                return duk_to_number(ctx, idx) != 0;
            } else if(duk_is_boolean(ctx, idx)) {
                return duk_to_boolean(ctx, idx) ? true : false;
            }
            return 0;
        }
        
        void * toBufferData(duk_context * ctx, duk_idx_t idx, duk_size_t * size) {
            if(duk_is_buffer(ctx, idx)) {
                return duk_get_buffer(ctx, idx, size);
            } else if(duk_is_buffer_data(ctx, idx)) {
                return duk_get_buffer_data(ctx, idx, size);
            }
            if(size) {
                *size = 0;
            }
            return nullptr;
        }
        
        kk::Int toIntArgument(duk_context * ctx, duk_idx_t i,kk::Int defaultValue) {
            int top = duk_get_top(ctx);
            if(i < top) {
                return toInt(ctx, -top + i);
            }
            return defaultValue;
        }
        
        kk::Uint toUintArgument(duk_context * ctx, duk_idx_t i,kk::Uint defaultValue) {
            int top = duk_get_top(ctx);
            if(i < top) {
                return toUint(ctx, -top + i);
            }
            return defaultValue;
        }
        
        kk::Double toDoubleArgument(duk_context * ctx, duk_idx_t i,kk::Double defaultValue) {
            int top = duk_get_top(ctx);
            if(i < top) {
                return toDouble(ctx, -top + i);
            }
            return defaultValue;
        }
        
        kk::String toStringArgument(duk_context * ctx, duk_idx_t i,kk::CString defaultValue) {
            int top = duk_get_top(ctx);
            if(i < top) {
                return toString(ctx, -top + i);
            }
            return defaultValue == nullptr ? "" : defaultValue;
        }
        
        kk::CString toCStringArgument(duk_context * ctx, duk_idx_t i,kk::CString defaultValue) {
            int top = duk_get_top(ctx);
            if(i < top) {
                return toCString(ctx, -top + i);
            }
            return defaultValue;
        }
        
        kk::Boolean toBooleanArgument(duk_context * ctx, duk_idx_t i,kk::Boolean defaultValue) {
            int top = duk_get_top(ctx);
            if(i < top) {
                return toBoolean(ctx, -top + i);
            }
            return defaultValue;
        }
        
        kk::Object * toObjectArgument(duk_context * ctx, duk_idx_t i) {
            int top = duk_get_top(ctx);
            if(i < top && duk_is_object(ctx, -top + i)) {
                return GetObject(ctx, -top + i);
            }
            return nullptr;
        }
        
        void * toBufferDataArgument(duk_context * ctx, duk_idx_t i, duk_size_t * size) {
            int top = duk_get_top(ctx);
            if(i < top && duk_is_object(ctx, -top + i)) {
                return toBufferData(ctx, -top + i, size);
            } else if(size) {
                * size = 0;
            }
            return nullptr;
        }
        
        class WeakObject {
        public:
            
            WeakObject(void *heapptr):_heapptr(heapptr) {}
            
            virtual ~WeakObject() {
                
            }
            
            virtual void recycle(duk_context * ctx) {
                
                {
                    std::set<void **>::iterator i = _weaks.begin();
                    while(i != _weaks.end()) {
                        void ** p = * i;
                        *p = nullptr;
                        i ++;
                    }
                }
                
                {
                    std::set<IWeakObject *>::iterator i = _objects.begin();
                    while(i != _objects.end()) {
                        IWeakObject * v = * i;
                        v->recycle(ctx, _heapptr);
                        i ++;
                    }
                }
            }
            
            virtual void weak(void **heapptr) {
                _weaks.insert(heapptr);
            }
            
            virtual void unweak(void ** heapptr) {
                std::set<void **>::iterator i = _weaks.find(heapptr);
                if(i != _weaks.end()) {
                    _weaks.erase(i);
                }
            }
            
            virtual void weakObject(IWeakObject * object) {
                _objects.insert(object);
            }
            
            virtual void unweakObject(IWeakObject * object) {
                std::set<IWeakObject *>::iterator i = _objects.find(object);
                if(i != _objects.end()) {
                    _objects.erase(i);
                }
            }
            
        protected:
            std::set<void **> _weaks;
            std::set<IWeakObject *> _objects;
            void * _heapptr;
        };
        
        static duk_ret_t WeakObject_dealloc(duk_context * ctx) {
            
            WeakObject * v = nullptr;
            
            duk_get_prop_string(ctx, -1, "__object");
            
            if(duk_is_pointer(ctx, -1)) {
                v =  (WeakObject *) duk_to_pointer(ctx, -1);
            }
            
            duk_pop(ctx);
            
            if(v) {
                v->recycle(ctx);
                delete v;
            }
            
            return 0;
        }
        
        static WeakObject * duk_getWeakObject(duk_context * ctx, duk_idx_t idx, kk::Boolean isCreated) {
            
            if(duk_is_object(ctx, idx) || duk_is_function(ctx, idx)) {
                
                WeakObject * v = nullptr;
                
                duk_get_prop_string(ctx, idx, "__weakObject");
                
                if(duk_is_object(ctx, -1)) {
                    
                    duk_get_prop_string(ctx, -1, "__object");
                    
                    if(duk_is_pointer(ctx, -1)) {
                        v = (WeakObject *) duk_to_pointer(ctx, -1);
                    }
                    
                    duk_pop_2(ctx);
                    
                } else {
                    duk_pop(ctx);
                }
                
                if(v == nullptr && isCreated) {
                    
                    v = new WeakObject(duk_get_heapptr(ctx, idx));
                    
                    duk_push_string(ctx, "__weakObject");
                    
                    duk_push_object(ctx);
                    {
                        duk_push_string(ctx, "__object");
                        duk_push_pointer(ctx, v);
                        duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_CLEAR_WRITABLE | DUK_DEFPROP_CLEAR_ENUMERABLE | DUK_DEFPROP_CLEAR_CONFIGURABLE);
                        
                        duk_push_c_function(ctx, WeakObject_dealloc, 1);
                        duk_set_finalizer(ctx, -2);
                    }
                    duk_def_prop(ctx, idx - 2, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_CLEAR_WRITABLE | DUK_DEFPROP_CLEAR_ENUMERABLE | DUK_DEFPROP_CLEAR_CONFIGURABLE);
                }
                
                return v;
                
            }
            
            return nullptr;
        }
        
        void duk_weak(duk_context * ctx, duk_idx_t idx,void ** heapptr) {
            
            WeakObject * v = duk_getWeakObject(ctx, idx,true);
            
            if(v != nullptr) {
                * heapptr = duk_get_heapptr(ctx, idx);
                v->weak(heapptr);
            }
            
        }
        
        void duk_unweak(duk_context * ctx, void ** heapptr) {
            
            if(*heapptr != nullptr) {
                
                duk_push_heapptr(ctx, *heapptr);
                
                WeakObject * v = duk_getWeakObject(ctx, -1,false);
                
                if(v != nullptr) {
                    v->unweak(heapptr);
                }
                
                duk_pop(ctx);
                
            }
            
            * heapptr = nullptr;
        }
        
        void duk_weakObject(duk_context * ctx, duk_idx_t idx,IWeakObject * object) {
            
            WeakObject * v = duk_getWeakObject(ctx, idx,true);
            
            if(v != nullptr) {
                v->weakObject(object);
            }
            
        }
        
        void duk_unweakObject(duk_context * ctx, duk_idx_t idx,IWeakObject * object) {
            
            WeakObject * v = duk_getWeakObject(ctx, idx,false);
            
            if(v != nullptr) {
                v->unweakObject(object);
            }
            
        }
        
        class WeakMap : public IWeakObject {
        public:
            
            WeakMap(void * heapptr):_heapptr(heapptr) {
                
            }
            
            virtual void recycle(duk_context * ctx) {
                std::set<void *>::iterator i = _keys.begin();
                while(i != _keys.end()) {
                    void * p = *i;
                    duk_push_heapptr(ctx, p);
                    duk_unweakObject(ctx, -1, this);
                    duk_pop(ctx);
                    i++;
                }
            }
            
            virtual void recycle(duk_context * ctx,void * heapptr) {
                std::set<void *>::iterator i = _keys.find(heapptr);
                if(i != _keys.end()) {
                    _keys.erase(i);
                    duk_push_heapptr(ctx, _heapptr);
                    duk_push_sprintf(ctx, "__0x%x",(unsigned long) heapptr);
                    duk_del_prop(ctx, -2);
                    duk_pop(ctx);
                }
            }
            
            virtual duk_ret_t set(duk_context * ctx) {
                
                void * key = duk_get_heapptr(ctx, -2);

                std::set<void *>::iterator i = _keys.find(key);
                
                if(i == _keys.end()) {
                    duk_weakObject(ctx, -2, this);
                    _keys.insert(key);
                }
                
                duk_push_this(ctx);
                
                duk_push_sprintf(ctx, "__0x%x",(unsigned long) key);
                duk_dup(ctx, -3);
                duk_def_prop(ctx, -3,DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_CLEAR_WRITABLE | DUK_DEFPROP_CLEAR_ENUMERABLE | DUK_DEFPROP_SET_CONFIGURABLE);
                
                duk_pop(ctx);
                
                return 0;
            }
            
            virtual duk_ret_t has(duk_context * ctx) {
                void * key = duk_get_heapptr(ctx, -1);
                std::set<void *>::iterator i  = _keys.find(key);
                if(i != _keys.end()) {
                    duk_push_boolean(ctx, true);
                    return 1;
                }
                duk_push_boolean(ctx, false);
                return 1;
            }
            
            virtual duk_ret_t get(duk_context * ctx) {
                void * key = duk_get_heapptr(ctx, -1);
                std::set<void *>::iterator i  = _keys.find(key);
                
                if(i != _keys.end()) {
                    
                    duk_push_this(ctx);
                    
                    duk_push_sprintf(ctx, "__0x%x",(unsigned long) key);
                    duk_get_prop(ctx, -2);
                    
                    duk_remove(ctx, -2);
                    
                    return 1;
                }
                
                duk_push_undefined(ctx);
                
                return 1;
            }
            
            virtual duk_ret_t remove(duk_context * ctx) {
                
                void * key = duk_get_heapptr(ctx, -1);
                
                std::set<void *>::iterator i  = _keys.find(key);
                
                if(i != _keys.end()) {
                    
                    duk_push_this(ctx);
                    
                    duk_push_sprintf(ctx, "__0x%x",(unsigned long) key);
                    duk_del_prop(ctx, -2);
                    
                    duk_pop(ctx);
                    
                    duk_unweakObject(ctx, -1, this);
                    
                    _keys.erase(i);
                }
                
                return 0;
            }
            
        protected:
            std::set<void *> _keys;
            void * _heapptr;
        };
        
        
        static duk_ret_t WeakMap_dealloc(duk_context * ctx) {
            
            WeakMap * v = nullptr;
            
            duk_get_prop_string(ctx, -1, "__object");
            
            if(duk_is_pointer(ctx, -1)) {
                v = (WeakMap *) duk_to_pointer(ctx, -1);
            }
            
            duk_pop(ctx);
            
            if(v) {
                v->recycle(ctx);
                delete v;
            }
            
            return 0;
        }
        
        static duk_ret_t WeakMap_alloc(duk_context * ctx) {
            
            duk_push_this(ctx);
            
            WeakMap * object = new WeakMap(duk_get_heapptr(ctx, -1));
            
            duk_push_string(ctx, "__object");
            duk_push_pointer(ctx, object);
            duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_CLEAR_WRITABLE | DUK_DEFPROP_CLEAR_ENUMERABLE | DUK_DEFPROP_CLEAR_CONFIGURABLE);
            
            duk_push_current_function(ctx);
            duk_get_prototype(ctx, -1);
            duk_set_prototype(ctx, -3);
            duk_pop(ctx);
            
            duk_push_c_function(ctx, WeakMap_dealloc, 1);
            duk_set_finalizer(ctx, -2);
            
            duk_pop(ctx);
            
            int top = duk_get_top(ctx);
            
            if(top > 0 && duk_is_object(ctx, -top)) {
                
                duk_enum(ctx, -top, DUK_ENUM_INCLUDE_SYMBOLS);
                
                while(duk_next(ctx, -1, 1)) {
                    
                    if(duk_is_object(ctx, -1) || duk_is_function(ctx, -1)) {
                        
                        object->set(ctx);
                        
                    } else {
                        duk_pop_2(ctx);
                    }
                    
                }
                
                duk_pop(ctx);
                
            }
            
            return 0;
        }
        
        static duk_ret_t WeakMap_get(duk_context * ctx) {
            
            WeakMap * v = nullptr;
            
            duk_push_this(ctx);
            
            duk_get_prop_string(ctx, -1, "__object");
            
            if(duk_is_pointer(ctx, -1)) {
                v = (WeakMap *) duk_to_pointer(ctx, -1);
            }
            
            duk_pop_2(ctx);
        
            if(v && duk_is_object(ctx, -1) ) {
                return v->get(ctx);
            }
            
            return 0;
        }
        
        static duk_ret_t WeakMap_set(duk_context * ctx) {
            
            WeakMap * v = nullptr;
            
            duk_push_this(ctx);
            
            duk_get_prop_string(ctx, -1, "__object");
            
            if(duk_is_pointer(ctx, -1)) {
                v = (WeakMap *) duk_to_pointer(ctx, -1);
            }
            
            duk_pop_2(ctx);
            
            if(v && duk_is_object(ctx, -1) && duk_is_object(ctx, -2)) {
                return v->set(ctx);
            }
            
            return 0;
        }
        
        static duk_ret_t WeakMap_has(duk_context * ctx) {
            
            WeakMap * v = nullptr;
            
            duk_push_this(ctx);
            
            duk_get_prop_string(ctx, -1, "__object");
            
            if(duk_is_pointer(ctx, -1)) {
                v = (WeakMap *) duk_to_pointer(ctx, -1);
            }
            
            duk_pop_2(ctx);
            
            if(v && duk_is_object(ctx, -1) ) {
                return v->has(ctx);
            }
            
            return 0;
        }
        
        static duk_ret_t WeakMap_delete(duk_context * ctx) {
            
            WeakMap * v = nullptr;
            
            duk_push_this(ctx);
            
            duk_get_prop_string(ctx, -1, "__object");
            
            if(duk_is_pointer(ctx, -1)) {
                v = (WeakMap *) duk_to_pointer(ctx, -1);
            }
            
            duk_pop_2(ctx);
            
            if(v && duk_is_object(ctx, -1)) {
                return v->remove(ctx);
            }
            
            return 0;
        }
        
        void OpenlibWeakMap(duk_context * ctx) {
            
            duk_push_c_function(ctx, WeakMap_alloc, 1);
            
            duk_push_object(ctx);
            
            duk_push_c_function(ctx, WeakMap_get, 1);
            duk_put_prop_string(ctx,-2,"get");
            
            duk_push_c_function(ctx, WeakMap_set, 2);
            duk_put_prop_string(ctx,-2,"set");
            
            duk_push_c_function(ctx, WeakMap_has, 1);
            duk_put_prop_string(ctx,-2,"has");
            
            duk_push_c_function(ctx, WeakMap_delete, 1);
            duk_put_prop_string(ctx,-2,"delete");
            
            duk_set_prototype(ctx, -2);
            
            duk_put_global_string(ctx, "WeakMap");
            
        }
        
        static void MapPushKeyString(duk_context * ctx, duk_idx_t idx) {
            
            if(duk_is_undefined(ctx, idx)) {
                duk_push_string(ctx, "@undefined");
                return;
            }
            
            if(duk_is_null(ctx, idx)) {
                duk_push_string(ctx, "@null");
                return;
            }
            
            if(duk_is_nan(ctx, idx)) {
                duk_push_string(ctx, "@nan");
                return;
            }
            
            if(duk_is_boolean(ctx, idx)) {
                if(duk_to_boolean(ctx, idx)) {
                    duk_push_string(ctx, "@true");
                    return;
                } else {
                    duk_push_string(ctx, "@false");
                    return;
                }
            }
            
            if(duk_is_number(ctx, idx)) {
                duk_push_sprintf(ctx, "@%g",duk_to_number(ctx, idx));
                return;
            }
            
            if(duk_is_pointer(ctx, idx)) {
                duk_push_sprintf(ctx, "0x%lx",(unsigned long) duk_to_pointer(ctx, idx));
                return;
            }
            
            if(duk_is_string(ctx, idx)) {
                duk_dup(ctx, idx);
                return;
            }
            
            duk_push_sprintf(ctx, "0x%lx",(unsigned long) duk_get_heapptr(ctx, idx));

        }
        
        static duk_ret_t Map_set(duk_context * ctx);
        
        static duk_ret_t Map_alloc(duk_context * ctx) {
            
            duk_push_this(ctx);
            
            duk_push_current_function(ctx);
            duk_get_prototype(ctx, -1);
            duk_set_prototype(ctx, -3);
            duk_pop(ctx);
  
            duk_pop(ctx);
            
            int top = duk_get_top(ctx);
            
            if(top > 0 && duk_is_object(ctx, -top)) {
                
                duk_enum(ctx, -top, DUK_ENUM_INCLUDE_SYMBOLS);
                
                while(duk_next(ctx, -1, 1)) {
                    
                    Map_set(ctx);
                    
                }
                
                duk_pop(ctx);
                
            }
            
            return 0;
            
        }
            
        static duk_ret_t Map_get(duk_context * ctx) {
            
            duk_push_this(ctx);
            
            MapPushKeyString(ctx, -2);
            duk_get_prop(ctx, -2);
            
            if(duk_is_object(ctx, -1)) {
                duk_get_prop_string(ctx, -1, "value");
                duk_remove(ctx, -2);
                duk_remove(ctx, -2);
                return 1;
            }
            
            duk_pop_2(ctx);
            
            return 0;
        }
        
        static duk_ret_t Map_set(duk_context * ctx) {
            
            duk_push_this(ctx);
            
            MapPushKeyString(ctx, -3);
            duk_push_object(ctx);
            {
                duk_push_string(ctx, "type");
                duk_push_pointer(ctx, (void *) Map_alloc);
                duk_put_prop(ctx, -3);
                duk_push_string(ctx, "key");
                duk_dup(ctx, -6);
                duk_put_prop(ctx, -3);
                duk_push_string(ctx, "value");
                duk_dup(ctx, -5);
                duk_put_prop(ctx, -3);
            }
            duk_def_prop(ctx, -3,DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_CLEAR_WRITABLE | DUK_DEFPROP_CLEAR_ENUMERABLE | DUK_DEFPROP_SET_CONFIGURABLE);
            
            duk_pop(ctx);
            
            return 0;
        }
        
        static duk_ret_t Map_has(duk_context * ctx) {
            
            duk_bool_t v = 0;
            
            duk_push_this(ctx);
            
            MapPushKeyString(ctx, -2);
            duk_get_prop(ctx, -2);
            
            if(duk_is_object(ctx, -1)) {
                v = true;
            }
            
            duk_pop_2(ctx);
            
            duk_push_boolean(ctx, v);
            
            return 1;
            
        }
        
        static duk_ret_t Map_delete(duk_context * ctx) {
            
            duk_push_this(ctx);
            
            MapPushKeyString(ctx, -2);
            duk_del_prop(ctx, -2);
            
            duk_pop(ctx);
        
            return 0;
        }
        
        static duk_ret_t Map_forEach(duk_context * ctx) {
            
            duk_push_this(ctx);
            
            duk_enum(ctx, -1, DUK_ENUM_INCLUDE_NONENUMERABLE);
            
            while(duk_next(ctx, -1, 1)) {
                
                if(! duk_is_object(ctx, -1)) {
                    duk_pop_2(ctx);
                    continue;
                }
                
                duk_get_prop_string(ctx, -1, "type");
                
                if(duk_is_pointer(ctx, -1) && duk_to_pointer(ctx, -1) == Map_alloc) {
                    duk_pop(ctx);
                } else {
                    duk_pop_3(ctx);
                    continue;
                }
                
                duk_dup(ctx, -5);
                
                if(duk_is_function(ctx, -1)) {
                    
                    
                    
                    
                    duk_get_prop_string(ctx, -2, "value");
                    duk_get_prop_string(ctx, -3, "key");
                    
                    if(duk_pcall(ctx, 2) != DUK_EXEC_SUCCESS) {
                        Error(ctx, -1);
                    }
                    
                    duk_pop_3(ctx);
                    
                } else {
                    duk_pop_3(ctx);
                }
                
            }
            
            duk_pop_2(ctx);
            
            return 0;
        }
        
        
        void OpenlibMap(duk_context * ctx) {
            
            duk_push_c_function(ctx, Map_alloc, 1);
            
            duk_push_object(ctx);
            
            duk_push_c_function(ctx, Map_get, 1);
            duk_put_prop_string(ctx,-2,"get");
            
            duk_push_c_function(ctx, Map_set, 2);
            duk_put_prop_string(ctx,-2,"set");
            
            duk_push_c_function(ctx, Map_has, 1);
            duk_put_prop_string(ctx,-2,"has");
            
            duk_push_c_function(ctx, Map_delete, 1);
            duk_put_prop_string(ctx,-2,"delete");
            
            duk_push_c_function(ctx, Map_forEach, 1);
            duk_put_prop_string(ctx,-2,"forEach");
            
            duk_set_prototype(ctx, -2);
            
            duk_put_global_string(ctx, "Map");
            
        }
        
    }
}
