//
//  KKObject.m
//  KKObject
//
//  Created by 张海龙 on 2018/10/1.
//  Copyright © 2018年 kkmofang.cn. All rights reserved.
//

#import <Foundation/Foundation.h>
#include "kk-config.h"
#include "kk-object.h"

namespace kk {
    
    void LogV(const char * format, va_list va) {
        NSLogv([@"[KK] " stringByAppendingString:[NSString stringWithCString:format encoding:NSUTF8StringEncoding]], va);
    }
    
}

