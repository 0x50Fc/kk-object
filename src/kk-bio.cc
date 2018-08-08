//
//  kk-bio.cc
//  KKObject
//
//  Created by zhanghailong on 2018/8/7.
//  Copyright © 2018年 kkmofang.cn. All rights reserved.
//

#include "kk-config.h"
#include "kk-bio.h"

#define BYTE_BITS       8
#define INT32_BYTES      4
#define INT64_BYTES      8
#define FLOAT_BYTES      4
#define DOUBLE_BYTES     8

namespace kk {
    
    size_t Bio::Byte_Size = 1;
    size_t Bio::Int32_Size = INT32_BYTES;
    size_t Bio::Int64_Size = INT64_BYTES;
    size_t Bio::Boolean_Size = 1;
    size_t Bio::Float_Size = FLOAT_BYTES;
    size_t Bio::Double_Size = DOUBLE_BYTES;
    
    size_t Bio::encode(Byte value,Byte * data, size_t size) {
        assert(size >= 1);
        * data = value;
        return 1;
    }
    
    size_t Bio::encode(Int32 value,Byte * data, size_t size) {
        assert(size >= INT32_BYTES);
        int b = INT32_BYTES;
        Ubyte *p = (Ubyte *) data;
        
        while(b--){
            *p = value & 0x0ff;
            value = value >> BYTE_BITS;
            p++;
        }
        return INT32_BYTES;
    }
    
    size_t Bio::encode(Int64 value,Byte * data, size_t size) {
        assert(size >= INT64_BYTES);
        int b = INT64_BYTES;
        Ubyte *p = (Ubyte *) data;
        
        while(b--){
            *p = value & 0x0ff;
            value = value >> BYTE_BITS;
            p++;
        }
        return INT64_BYTES;
    }
    
    size_t Bio::encode(Boolean value,Byte * data, size_t size) {
        assert(size >= 1);
        * data = value ? 1 : 0;
        return 1;
    }
    
    size_t Bio::encode(Float value,Byte * data, size_t size) {
        assert(size >= FLOAT_BYTES);
        memcpy(data, &value, FLOAT_BYTES);
        return FLOAT_BYTES;
    }
    
    size_t Bio::encode(Double value,Byte * data, size_t size) {
        assert(size >= DOUBLE_BYTES);
        memcpy(data, &value, DOUBLE_BYTES);
        return DOUBLE_BYTES;
    }
    
    size_t Bio::decode(Byte *value,Byte * data, size_t size) {
        assert(size >= 1);
        * value = * data;
        return 1;
    }
    
    size_t Bio::decode(Int32 *value,Byte * data, size_t size) {
        assert(size >= INT32_BYTES);
        Int32 v =0,cv;
        int c = INT32_BYTES;
        Ubyte *p = (Ubyte *) data;
        while(c){
            cv = *p;
            v = v | (cv << BYTE_BITS * (INT32_BYTES - c));
            p ++;
            c --;
        }
        * value = v;
        return INT32_BYTES;
    }
    
    size_t Bio::decode(Int64 *value,Byte * data, size_t size) {
        assert(size >= INT64_BYTES);
        Int64 v =0,cv;
        int c = INT64_BYTES;
        Ubyte *p = (Ubyte *) data;
        while(c){
            cv = *p;
            v = v | (cv << BYTE_BITS * (INT64_BYTES - c));
            p ++;
            c --;
        }
        * value = v;
        return INT64_BYTES;
    }
    
    size_t Bio::decode(Boolean *value,Byte * data, size_t size) {
        assert(size >= 1);
        * value = * data ? true : false;
        return 1;
    }
    
    size_t Bio::decode(Float *value,Byte * data, size_t size) {
        assert(size >= FLOAT_BYTES);
        memcpy(value, data, FLOAT_BYTES);
        return FLOAT_BYTES;
    }
    
    size_t Bio::decode(Double *value,Byte * data, size_t size) {
        assert(size >= DOUBLE_BYTES);
        memcpy(value, data, DOUBLE_BYTES);
        return DOUBLE_BYTES;
    }
    
}

