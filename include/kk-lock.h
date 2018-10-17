//
//  kk-lock.h
//  KKObject
//
//  Created by zhanghailong on 2018/10/11.
//  Copyright © 2018年 kkmofang.cn. All rights reserved.
//

#ifndef kk_lock_h
#define kk_lock_h


namespace kk {
    
    class Lock {
    public:
        virtual void lock() = 0;
        virtual void unlock() = 0;
    };
    
    class RWLock : public Lock {
    public:
        virtual void rlock() = 0;
        virtual void runlock() = 0;
    };
    
    extern Lock * CreateLock();
    extern RWLock * CreateRWLock();
}


#endif /* kk_lock_h */
