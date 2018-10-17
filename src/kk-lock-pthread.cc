//
//  kk-lock-pthread.cc
//  KKObject
//
//  Created by zhanghailong on 2018/10/11.
//  Copyright © 2018年 kkmofang.cn. All rights reserved.
//


#include <pthread.h>

#include "kk-config.h"
#include "kk-lock.h"

namespace kk {
    
    class MutexLock : public Lock {
    public:
        MutexLock();
        virtual ~MutexLock();
        virtual void lock();
        virtual void unlock();
    protected:
        pthread_mutex_t _v;
    };
    
    MutexLock::MutexLock() {
        pthread_mutex_init(&_v, nullptr);
    }
    
    MutexLock::~MutexLock() {
        pthread_mutex_destroy(&_v);
    }
    
    void MutexLock::lock() {
        pthread_mutex_lock(&_v);
    }
    
    void MutexLock::unlock() {
        pthread_mutex_unlock(&_v);
    }
    
    class RWLockIMP : public RWLock {
    public:
        RWLockIMP();
        virtual ~RWLockIMP();
        virtual void lock();
        virtual void unlock();
        virtual void rlock();
        virtual void runlock();
    protected:
        pthread_rwlock_t _v;
    };
    
    RWLockIMP::RWLockIMP() {
        pthread_rwlock_init(&_v, nullptr);
    }
    
    RWLockIMP::~RWLockIMP() {
        pthread_rwlock_destroy(&_v);
    }
    
    void RWLockIMP::lock() {
        pthread_rwlock_wrlock(&_v);
    }
    
    void RWLockIMP::unlock() {
        pthread_rwlock_unlock(&_v);
    }
    
    void RWLockIMP::rlock() {
        pthread_rwlock_rdlock(&_v);
    }
    
    void RWLockIMP::runlock() {
        pthread_rwlock_unlock(&_v);
    }
    

    Lock * CreateLock() {
        return new MutexLock();
    }
    
    RWLock * CreateRWLock() {
        return new RWLockIMP();
    }
    
}
