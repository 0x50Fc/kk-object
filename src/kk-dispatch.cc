//
//  kk-dispatch.cpp
//  app
//
//  Created by zhanghailong on 2018/7/30.
//  Copyright © 2018年 kkmofang.cn. All rights reserved.
//

#include <pthread.h>

#include "kk-config.h"
#include "kk-dispatch.h"
#include "kk-ev.h"
#include <queue>

namespace kk {
    
    static struct timeval tv = {0,60000};
    
    void DispatchQueueCB(evutil_socket_t fd, short ev, void * ctx) {
        DispatchQueue * queue = (DispatchQueue *) ctx;
        queue->run();
    }
    
    
    class DispatchObject {
    public:
        
        DispatchObject(DispatchFunc func,BK_DEF_ARG,bool synced):_synced(synced) {
            if(_synced) {
                pthread_mutex_init(&_lock, nullptr);
                pthread_cond_init(&_cond, nullptr);
            }
            _context = __BK_CTX;
            if(_context) {
                _context->retain();
            }
            _func = func;
        }
        
        virtual ~DispatchObject() {
            if(_synced) {
                pthread_cond_destroy(&_cond);
                pthread_mutex_destroy(&_lock);
            }
            if(_context) {
                _context->release();
            }
        }
        
        virtual void wait(kk::Chan * chan) {
            if(_synced) {
                pthread_mutex_lock(&_lock);
                chan->push(this);
                pthread_cond_wait(&_cond, &_lock);
                pthread_mutex_unlock(&_lock);
            }
        }
        
        virtual void join() {
            if(_synced) {
                pthread_mutex_lock(&_lock);
                pthread_cond_broadcast(&_cond);
                pthread_mutex_unlock(&_lock);
            }
        }
        
        virtual void run(DispatchQueue * queue) {
            if(_func != nullptr) {
                _func(queue,_context);
            }
        }
        
    protected:
        pthread_cond_t _cond;
        pthread_mutex_t _lock;
        event * _event;
        bool _synced;
        BlockContext * _context;
        DispatchFunc _func;
    };
    
    void DispatchObjectRelease(kk::Chan * chan,kk::ChanObject object) {
        DispatchObject * v = (DispatchObject *) object;
        delete v;
    }
    
    static void DispatchQueueRunSIGPIPE(evutil_socket_t fd, short ev, void * ctx) {}

    static pthread_key_t gDispatchQueueKey = 0;
    
    void * DispatchQueueRun(void * data) {
        
        DispatchQueue * queue = (DispatchQueue *) data;
        
        pthread_setspecific(gDispatchQueueKey, queue);
        
#ifndef KK_PLATFORM_LINUX
        pthread_setname_np(queue->name());
#endif

        event_base * base = queue->base();
        
        struct event * s = evsignal_new(base, SIGPIPE, DispatchQueueRunSIGPIPE, NULL);
        
        evsignal_add(s, NULL);
        
        event_base_dispatch(base);
        
        evsignal_del(s);
        
        event_free(s);
        
        pthread_setspecific(gDispatchQueueKey, nullptr);
        
        pthread_exit(nullptr);
        
        return nullptr;
    }
    
    void DispatchQueue::run() {
        
        do {
            
            DispatchObject * object =  (DispatchObject *) _chan->pop();
            
            if(object == nullptr) {
                break;
            }
            
            object->run(this);
            
            object->join();
            
            _chan->releaseObject((ChanObject) object);
            
        } while (! _loopbreak);
        
        if(!_loopbreak) {
            evtimer_add(_event, &tv);
        }
        
    }
    
    static void DispatchQueueBreak(DispatchQueue * queue,BK_DEF_ARG) {
        queue->loopbreak();
    }
        
    DispatchQueue::DispatchQueue(kk::CString name):_name(name) {
        
        if(gDispatchQueueKey == 0) {
            pthread_key_create(&gDispatchQueueKey, nullptr);
        }
        
        _attach = false;
        _joined = false;
        _loopbreak = false;
        _chan = new kk::Chan(DispatchObjectRelease);
        _chan->retain();
        _base = event_base_new();
        _event = evtimer_new(_base, DispatchQueueCB, this);
        evtimer_add(_event, &tv);
        pthread_create(&_pid, nullptr, DispatchQueueRun, this);
    }
    
    DispatchQueue * DispatchQueue::current() {
        if(gDispatchQueueKey != 0) {
            return (DispatchQueue *) pthread_getspecific(gDispatchQueueKey);
        }
        return nullptr;
    }
    
    void DispatchQueue::setCurrent(DispatchQueue * queue) {
        pthread_setspecific(gDispatchQueueKey, queue);
    }
    
    DispatchQueue::DispatchQueue(kk::CString name,event_base * base):_name(name) {
        
        if(gDispatchQueueKey == 0) {
            pthread_key_create(&gDispatchQueueKey, nullptr);
        }
        
        _attach = true;
        _joined = false;
        _loopbreak = false;
        _chan = new kk::Chan(DispatchObjectRelease);
        _chan->retain();
        _base = base;
        _event = evtimer_new(_base, DispatchQueueCB, this);
        evtimer_add(_event, &tv);
        _pid = pthread_self();
    }
    
    
    DispatchQueue::~DispatchQueue() {
        
        if(_attach) {
            _chan->release();
            evtimer_del(_event);
            event_free(_event);
        } else {
            if(!_joined) {
                _joined = true;
                evtimer_del(_event);
                async(DispatchQueueBreak, nullptr);
                pthread_join(_pid, nullptr);
            }
            _chan->release();
            event_free(_event);
            event_base_free(_base);
        }
        
    }
    
    kk::CString DispatchQueue::name() {
        return _name.c_str();
    }
    
    void DispatchQueue::join() {
        if(!_attach && !_joined) {
            _joined = true;
            async(DispatchQueueBreak, nullptr);
            pthread_join(_pid, nullptr);
            evtimer_del(_event);
        }
    }
    
    void DispatchQueue::loopbreak() {
        if(!_loopbreak) {
            if(pthread_self() != _pid) {
                async(DispatchQueueBreak, nullptr);
            } else {
                _loopbreak = true;
                event_base_loopbreak(_base);
            }
        }
    }
    
    struct event_base * DispatchQueue::base() {
        return _base;
    }
    
    void DispatchQueue::async(DispatchFunc func,BK_DEF_ARG) {
        DispatchObject * v = new DispatchObject(func,__BK_CTX,false);
        _chan->push(v);
    }
    
    void DispatchQueue::sync(DispatchFunc func, BK_DEF_ARG) {
        DispatchObject * v = new DispatchObject(func,__BK_CTX,true);
        v->wait(_chan);
    }
    
    
    
}
