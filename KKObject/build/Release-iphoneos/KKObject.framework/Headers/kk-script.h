//
//  kk-script.h
//  KKGame
//
//  Created by zhanghailong on 2018/2/5.
//  Copyright © 2018年 kkmofang.cn. All rights reserved.
//

#ifndef kk_script_h
#define kk_script_h

#if defined(KK_PLATFORM_IOS)

#include <KKObject/kk-object.h>
#include <KKDuktape/KKDuktape.h>

#else

#include "kk-object.h"
#include "duk_config.h"
#include "duktape.h"

#endif

namespace kk {
    
    namespace script {
        
        typedef duk_ret_t (kk::Object::*Function)(duk_context * jsContext);
        
        typedef void (*ClassPrototypeFunc)(duk_context * jsContext);
        
        struct Class {
            Class * isa;
            CString name;
            ClassPrototypeFunc prototype;
            duk_c_function alloc;
        };
        
        class Debugger : public kk::Object {
        public:
            Debugger(int port);
            virtual ~Debugger();
            virtual int port();
            virtual void debug(duk_context * ctx);
        protected:
            int _sock;
        };
        
        class Context : public kk::Object {
        public:
            Context();
            virtual ~Context();
            virtual duk_context * jsContext();
            virtual kk::Object * object(kk::CString key);
            virtual void setObject(kk::CString key,kk::Object * object);
        private:
            duk_context * _jsContext;
            std::map<kk::String,kk::Object *> _objects;
        };
        
        class IHeapObject {
        public:
            virtual void setHeapptr(void * heapptr,duk_context * ctx) = 0;
            virtual void * heapptr(duk_context * ctx) = 0;
            virtual void removeHeapptr(duk_context * ctx) = 0;
        };
        
        class HeapObject: public kk::Object, public IHeapObject {
        public:
            HeapObject();
            virtual ~HeapObject();
            virtual void setHeapptr(void * heapptr,duk_context * ctx);
            virtual void * heapptr(duk_context * ctx);
            virtual void removeHeapptr(duk_context * ctx);
        protected:
            std::map<duk_context *,void *> _heapptrs;
        };
        
        class IReflectObject {
        public:
            virtual void addReflect(duk_context * ctx,void * heapptr) = 0;
            virtual void * reflect(duk_context * ctx) = 0;
        };
        
        class Object : public kk::Object {
        public:
            Object(Context * context,duk_idx_t idx);
            virtual ~Object();
            virtual Context * context();
            virtual duk_context * jsContext();
            virtual void * heapptr();
        private:
            Weak _context;
            void * _heapptr;
        };
        
        class IObject  {
        public:
            virtual Class * getScriptClass() = 0;
        };
        
        Context * GetContext(duk_context * jsContext);
        
        void PushObject(duk_context * ctx,kk::Object * object);
        
        void InitObject(duk_context * ctx,duk_idx_t idx,kk::Object * object);
        
        kk::Object * GetObject(duk_context * ctx,duk_idx_t idx);
        
        bool ObjectEqual(Object * a, Object * b);
        
        void SetPrototype(duk_context * ctx,Class * isa);
        
        bool GetPrototype(duk_context * ctx,Class * isa);
        
        bool GetPrototype(duk_context * ctx,kk::Object * object);
        
        void Error(duk_context * ctx, duk_idx_t idx);
        
        void Error(duk_context * ctx, duk_idx_t idx,kk::CString prefix);
        
        struct Property {
            CString name;
            Function getter;
            Function setter;
        };
        
        void SetProperty(duk_context * ctx, duk_idx_t idx, Property * propertys, kk::Uint count);
        
        struct Method {
            CString name;
            Function invoke;
        };
        
        void SetMethod(duk_context * ctx, duk_idx_t idx, Method * methods, kk::Uint count);
        
        void compile(duk_context * ctx, kk::CString code , kk::CString filename);
        
        duk_int_t decodeJSON(duk_context * ctx, kk::CString text, size_t n);
        
        kk::String toString(duk_context * ctx, duk_idx_t idx);
        
        kk::CString toCString(duk_context * ctx, duk_idx_t idx);
        
        kk::Double toDouble(duk_context * ctx, duk_idx_t idx);
        
        kk::Int toInt(duk_context * ctx, duk_idx_t idx);
        
        kk::Uint toUint(duk_context * ctx, duk_idx_t idx);
        
        kk::Boolean toBoolean(duk_context * ctx, duk_idx_t idx);
        
        void * toBufferData(duk_context * ctx, duk_idx_t idx, duk_size_t * size);
        
        kk::Int toIntArgument(duk_context * ctx, duk_idx_t i,kk::Int defaultValue);
        
        kk::Uint toUintArgument(duk_context * ctx, duk_idx_t i,kk::Uint defaultValue);
        
        kk::Double toDoubleArgument(duk_context * ctx, duk_idx_t i,kk::Double defaultValue);
        
        kk::String toStringArgument(duk_context * ctx, duk_idx_t i,kk::CString defaultValue);
        
        kk::CString toCStringArgument(duk_context * ctx, duk_idx_t i,kk::CString defaultValue);
        
        kk::Boolean toBooleanArgument(duk_context * ctx, duk_idx_t i,kk::Boolean defaultValue);
        
        kk::Object * toObjectArgument(duk_context * ctx, duk_idx_t i);
        
        void * toBufferDataArgument(duk_context * ctx, duk_idx_t i, duk_size_t * size);
        
        void OpenlibWeakMap(duk_context * ctx);
        
        class IWeakObject {
        public:
            virtual void recycle(duk_context * ctx, void * heapptr) = 0;
        };
        
        void duk_weak(duk_context * ctx, duk_idx_t idx,void ** heapptr);
        
        void duk_unweak(duk_context * ctx, void ** heapptr);
        
        void duk_weakObject(duk_context * ctx, duk_idx_t idx,IWeakObject * object);
        
        void duk_unweakObject(duk_context * ctx, duk_idx_t idx,IWeakObject * object);
        
        void OpenlibMap(duk_context * ctx);
        
        
        class ReflectObject: public kk::Object, public IReflectObject, public IWeakObject {
        public:
            ReflectObject();
            virtual ~ReflectObject();
            virtual void recycle(duk_context * ctx, void * heapptr);
            virtual void addReflect(duk_context * ctx,void * heapptr);
            virtual void * reflect(duk_context * ctx);
        protected:
            std::map<duk_context *,void *> _heapptrs;
        };
        
        typedef void (*OpenlibFunc)(duk_context * ctx);
        
        void addOpenlib(OpenlibFunc func, kk::CString target);
        
        void Openlib(duk_context * ctx,kk::CString target);
        
#define DEF_SCRIPT_CLASS \
    public: \
        static kk::script::Class ScriptClass; \
        static void ScriptClassPrototype(duk_context * ctx); \
        static duk_ret_t ScriptObjectAlloc(duk_context * ctx); \
        virtual kk::script::Class * getScriptClass();

#define DEF_SCRIPT_CLASS_NOALLOC \
public: \
static kk::script::Class ScriptClass; \
static void ScriptClassPrototype(duk_context * ctx); \
virtual kk::script::Class * getScriptClass();

#define IMP_SCRIPT_CLASS_BEGIN(isa,object,name) \
kk::script::Class object::ScriptClass = {((kk::script::Class *) isa),(#name),(kk::script::ClassPrototypeFunc)&object::ScriptClassPrototype,(duk_c_function) &object::ScriptObjectAlloc} ;\
duk_ret_t object::ScriptObjectAlloc(duk_context * ctx) { \
    kk::script::PushObject(ctx, new object()); \
    return 1; \
} \
kk::script::Class * object::getScriptClass() { return &object::ScriptClass; }  \
void object::ScriptClassPrototype(duk_context * ctx) {
   
#define IMP_SCRIPT_CLASS_BEGIN_NOALLOC(isa,object,name) \
kk::script::Class object::ScriptClass = {((kk::script::Class *) isa),(#name),(kk::script::ClassPrototypeFunc)&object::ScriptClassPrototype,(duk_c_function) nullptr} ;\
kk::script::Class * object::getScriptClass() { return &object::ScriptClass; }  \
void object::ScriptClassPrototype(duk_context * ctx) {
        
        
#define IMP_SCRIPT_CLASS_END \
}
 
#define DEF_SCRIPT_METHOD(name) virtual duk_ret_t duk_##name(duk_context * ctx);
#define IMP_SCRIPT_METHOD(object,name) {#name,(kk::script::Function) &object::duk_##name},
        
#define DEF_SCRIPT_PROPERTY_READONLY(name) duk_ret_t duk_##name(duk_context * ctx);
        
#define DEF_SCRIPT_PROPERTY(name,Name) duk_ret_t duk_##name(duk_context * ctx); \
duk_ret_t duk_set##Name(duk_context * ctx);
        
#define IMP_SCRIPT_PROPERTY_READONLY(object,name) {#name,(kk::script::Function) &object::duk_##name,(kk::script::Function) nullptr},
        
#define IMP_SCRIPT_PROPERTY(object,name,Name) {#name,(kk::script::Function) &object::duk_##name,(kk::script::Function) &object::duk_set##Name},

#define IMP_SCRIPT_CONST_STRING(name,value) \
{ \
    duk_push_string(ctx,#name); \
    duk_push_string(ctx,#value); \
    duk_def_prop(ctx,-3,DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_CLEAR_CONFIGURABLE |DUK_DEFPROP_CLEAR_WRITABLE); \
}
        
#define IMP_SCRIPT_CONST_INT(name,value) \
{ \
    duk_push_string(ctx,#name); \
    duk_push_int(ctx,value); \
    duk_def_prop(ctx,-3,DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_CLEAR_CONFIGURABLE |DUK_DEFPROP_CLEAR_WRITABLE); \
}
        
#define IMP_SCRIPT_CONST_UINT(name,value) \
{ \
duk_push_string(ctx,#name); \
duk_push_uint(ctx,value); \
duk_def_prop(ctx,-3,DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_CLEAR_CONFIGURABLE |DUK_DEFPROP_CLEAR_WRITABLE); \
}
        
    }
    
   
}

#endif /* kk_script_h */
