
#include "kk-config.h"
#include "kk-object.h"
#include <strstream>
#include <typeinfo>
#include <pthread.h>
#include <queue>

#ifdef __ANDROID_API__

#include <android/log.h>

#endif

namespace kk {
    
    Ref::Ref():_object(nullptr) {
        
    }
    
    Object * Ref::get() {
        return _object;
    }

    Strong::Strong():Ref() {
        
    }
    
    Strong::Strong(Object * object):Ref() {
        set(object);
    }
    
    Strong::Strong(const Strong &ref):Ref() {
        set(ref._object);
    }
    
    Strong::~Strong() {
        if(this->_object) {
            this->_object->release();
        }
    }
    
    void Strong::set(Object * object) {
        if(this->_object != object) {
            if(object) {
                object->retain();
            }
            if(this->_object) {
                this->_object->release();
            }
            this->_object = object;
        }
    }
    
    Strong& Strong::operator=(Object * object) {
        set(object);
        return * this;
    }
    
    Strong& Strong::operator=(Ref& ref) {
        set(ref.get());
        return * this;
    }
    
    Strong& Strong::operator=(Strong& ref) {
        set(ref.get());
        return * this;
    }
    
    Weak::Weak():Ref() {
        
    }
    
    Weak::Weak(Object * object):Ref() {
        set(object);
    }
    
    Weak::Weak(const Weak & ref):Ref() {
        set(ref._object);
    }
    
    Weak::~Weak() {
        if(this->_object) {
            this->_object->unWeak(&this->_object);
        }
    }
    
    void Weak::set(Object * object) {
        if(_object != object) {
            if(object) {
                object->weak(&this->_object);
            }
            if(_object) {
                _object->unWeak(&this->_object);
            }
            _object = object;
        }
    }
    
    Weak& Weak::operator=(Object * object) {
        set(object);
        return * this;
    }
    
    Weak& Weak::operator=(Ref& ref) {
        set(ref.get());
        return * this;
    }
    
    Weak& Weak::operator=(Weak& ref) {
        set(ref.get());
        return * this;
    }
    
    Object::Object(): _retainCount(0) {
    }
    
    Object::~Object(){
        
        Atomic * a = atomic();
        
        if(a != nullptr) {
            a->lock();
        }
        
        std::set<Object **>::iterator i =_weakObjects.begin();
        
        while(i != _weakObjects.end()) {
            Object ** v = * i;
            if(v) {
                *v = NULL;
            }
            i ++;
        }
        
        if(a != nullptr) {
            a->unlock();
        }
        
    }
    
    std::string Object::toString() {
        return std::string(typeid(this).name());
    }
    
    void Object::release() {
        Atomic * a = atomic();
        if(a != nullptr) {
            a->lock();
        }
        _retainCount --;
        if(_retainCount == 0) {
            if(a != nullptr) {
                a->addObject(this);
            } else {
                delete this;
            }
        }
        if(a != nullptr) {
            a->unlock();
        }
    }
    
    void Object::retain() {
        Atomic * a = atomic();
        if(a != nullptr) {
            a->lock();
        }
        _retainCount ++;
        if(a != nullptr) {
            a->unlock();
        }
    }
    
    int Object::retainCount() {
        return _retainCount;
    }
    
    void Object::weak(Object ** ptr) {
        Atomic * a = atomic();
        if(a != nullptr) {
            a->lock();
        }
        _weakObjects.insert(ptr);
        if(a != nullptr) {
            a->unlock();
        }
    }
    
    void Object::unWeak(Object ** ptr) {
        Atomic * a = atomic();
        if(a != nullptr) {
            a->lock();
        }
        std::set<Object **>::iterator i = _weakObjects.find(ptr);
        if(i != _weakObjects.end()) {
            _weakObjects.erase(i);
        }
        if(a != nullptr) {
            a->unlock();
        }
    }
    
    static pthread_key_t kScopeCurrent = 0;
    
    Scope::Scope():_parent(Scope::current()) {
        pthread_setspecific(kScopeCurrent, this);
    }
    
    Scope::~Scope() {
        
        Object * v = nullptr;
        
        do {
            
            if(_objects.empty()) {
                break;
            }
            
            v = _objects.front();
            
            _objects.pop();
            
            v->release();
            
        } while(v != nullptr);
        
        pthread_setspecific(kScopeCurrent, _parent);
        
    }
    
    void Scope::addObject(Object * object) {
        _objects.push(object);
        object->retain();
    }
    
    Scope * Scope::parent() {
        return _parent;
    }
    
    Scope * Scope::current() {
        if(kScopeCurrent == 0) {
            pthread_key_create(&kScopeCurrent,nullptr);
        }
        return (Scope *) pthread_getspecific(kScopeCurrent);
    }
    
    class MutexAtomic : public Atomic {
    public:
        
        MutexAtomic() {
            pthread_mutex_init(&_lock, nullptr);
            pthread_mutex_init(&_objectLock, nullptr);
        }
        
        virtual ~MutexAtomic() {
            pthread_mutex_destroy(&_lock);
            pthread_mutex_destroy(&_objectLock);
        }
        
        virtual void lock() {
            pthread_mutex_lock(&_lock);
        }
        
        virtual void unlock() {
            pthread_mutex_unlock(&_lock);
            
            Object * v = nullptr;
            
            do {
                
                pthread_mutex_lock(&_objectLock);
                
                if(_objects.empty()) {
                    v = nullptr;
                } else {
                    v = _objects.front();
                    _objects.pop();
                }
                
                pthread_mutex_unlock(&_objectLock);
                
                if(v != nullptr && v->retainCount() == 0) {
                    delete v;
                }
                
            } while (v);
        }
        
        virtual void addObject(Object * object) {
            pthread_mutex_lock(&_objectLock);
            _objects.push(object);
            pthread_mutex_unlock(&_objectLock);
        }
        
    private:
        pthread_mutex_t _lock;
        pthread_mutex_t _objectLock;
        std::queue<Object *> _objects;
    };
    
    Atomic * atomic() {
        
        static Atomic * a = nullptr;
        
        if(a == nullptr) {
            a = new MutexAtomic();
        }
        
        return a;
        
    }
    
#ifdef __ANDROID_API__

    void LogV(const char * format, va_list va) {
        __android_log_vprint(ANDROID_LOG_DEBUG,"kk",format,va);
    }
    
#elif defined(KK_PLATFORM_IOS)
    
#else

    void LogV(const char * format, va_list va) {

        time_t now = time(NULL);
        
        struct tm * p = localtime(&now);
        
        printf("[KK] [%04d-%02d-%02d %02d:%02d:%02d] ",1900 + p->tm_year,p->tm_mon + 1,p->tm_mday,p->tm_hour,p->tm_min,p->tm_sec);
        vprintf(format, va);
        printf("\n");
    
    }
    
#endif
    
    void Log(const char * format, ...) {
        va_list va;
        va_start(va, format);
        LogV(format, va);
        va_end(va);
    }
    
}
