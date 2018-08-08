//
//  kk-bio.h
//  KKObject
//
//  Created by zhanghailong on 2018/8/7.
//  Copyright © 2018年 kkmofang.cn. All rights reserved.
//

#ifndef kk_bio_h
#define kk_bio_h

#if defined(KK_PLATFORM_IOS)

#include <KKObject/kk-object.h>

#else

#include "kk-object.h"

#endif

namespace kk {
    
    class Bio {
    public:
        
        static size_t Byte_Size;
        static size_t Int32_Size;
        static size_t Int64_Size;
        static size_t Boolean_Size;
        static size_t Float_Size;
        static size_t Double_Size;
        
        static size_t encode(Byte value,Byte * data, size_t size);
        static size_t encode(Int32 value,Byte * data, size_t size);
        static size_t encode(Int64 value,Byte * data, size_t size);
        static size_t encode(Boolean value,Byte * data, size_t size);
        static size_t encode(Float value,Byte * data, size_t size);
        static size_t encode(Double value,Byte * data, size_t size);
        
        static size_t decode(Byte *value,Byte * data, size_t size);
        static size_t decode(Int32 *value,Byte * data, size_t size);
        static size_t decode(Int64 *value,Byte * data, size_t size);
        static size_t decode(Boolean *value,Byte * data, size_t size);
        static size_t decode(Float *value,Byte * data, size_t size);
        static size_t decode(Double *value,Byte * data, size_t size);
        
    };
    
}


#endif /* kk_bio_h */
