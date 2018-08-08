//
//  kk-chan.h
//  KKGame
//
//  Created by zhanghailong on 2018/7/30.
//  Copyright © 2018年 kkmofang.cn. All rights reserved.
//

#ifndef kk_chan_h
#define kk_chan_h

#if defined(KK_PLATFORM_IOS)

#include <KKObject/kk-object.h>
#include <pthread.h>
#include <queue>

#else

#include "kk-object.h"
#include <pthread.h>
#include <queue>

#endif



namespace kk {
    
    typedef void * ChanObject;
    
    class Chan;
    
    typedef void (*ChanObjectReleaseFunc)(Chan * chan,ChanObject object);
    
    void ChanObjectRelease(Chan * chan,ChanObject object);
    
    class Chan : public kk::Object {
    public:
        Chan();
        Chan(ChanObjectReleaseFunc release);
        virtual ~Chan();
        virtual void push(ChanObject object);
        virtual ChanObject pop();
        virtual void releaseObject(ChanObject object);
    protected:
        std::queue<ChanObject> _queue;
        pthread_mutex_t _lock;
        ChanObjectReleaseFunc _release;
    };
    
}


#endif /* kk_chan_h */
