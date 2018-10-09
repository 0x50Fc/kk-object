//
//  kk-crypto.cpp
//  KKGame
//
//  Created by zhanghailong on 2018/2/7.
//  Copyright © 2018年 kkmofang.cn. All rights reserved.
//

#include "kk-config.h"
#include "kk-crypto.h"

#include <zlib.h>

#if defined(__APPLE__)

#include <CommonCrypto/CommonCrypto.h>

#elif defined(__ANDROID_API__)

#include "md5.h"

#else

#include <openssl/md5.h>

#endif

namespace kk {
    
#if defined(__APPLE__)
    
    String Crypto_MD5(CString string) {
        
        CC_MD5_CTX m;
        
        CC_MD5_Init(&m);
        
        if(string) {
            CC_MD5_Update(&m, string, (CC_LONG) strlen(string));
        }
        
        unsigned char md[16];
        
        CC_MD5_Final(md, &m);
        
        char s[40] = "";
        
        snprintf(s, sizeof(s), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
                 ,md[0],md[1],md[2],md[3],md[4],md[5],md[6],md[7]
                 ,md[8],md[9],md[10],md[11],md[12],md[13],md[14],md[15]);
        
        return s;
    }
#elif defined(__ANDROID_API__)

    String Crypto_MD5(CString string) {

        md5_state_t m;

        md5_init(&m);

        if(string) {
            md5_append(&m, (md5_byte_t *) string, (size_t) strlen(string));
        }

        md5_byte_t md[16];

        md5_finish(&m,md);

        char s[40] = "";

        snprintf(s, sizeof(s), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
                ,md[0],md[1],md[2],md[3],md[4],md[5],md[6],md[7]
                ,md[8],md[9],md[10],md[11],md[12],md[13],md[14],md[15]);

        return s;
    }

#else
   
    String Crypto_MD5(CString string) {
        
        MD5_CTX m;
        
        MD5_Init(&m);
        
        if(string) {
            MD5_Update(&m, string, (size_t) strlen(string));
        }
        
        unsigned char md[16];
        
        MD5_Final(md, &m);
        
        char s[40] = "";
        
        snprintf(s, sizeof(s), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
                 ,md[0],md[1],md[2],md[3],md[4],md[5],md[6],md[7]
                 ,md[8],md[9],md[10],md[11],md[12],md[13],md[14],md[15]);
        
        return s;
    }

#endif
    
    static duk_ret_t Crypto_MD5_func(duk_context * ctx) {
        int top = duk_get_top(ctx);
        
        if(top >0) {
            if(duk_is_string(ctx, -top)) {
                String v = Crypto_MD5(duk_to_string(ctx, -top));
                duk_push_string(ctx, v.c_str());
                return 1;
            }
        }
        
        return 0;
    }
    
    static duk_ret_t Crypto_zlib_deflate_func(duk_context * ctx) {
        
        size_t n = 0;
        void * data = kk::script::toBufferDataArgument(ctx, 0, &n);
        
        if(!data) {
            return 0;
        }
        
        size_t outlen = n * 1.5;
        void * out = duk_push_dynamic_buffer(ctx, outlen);
        
        int status = Z_OK;
        z_stream strm;
        
        strm.next_in = (Bytef *) data;
        strm.avail_in = (uInt) n;
        strm.avail_out = 0;
        strm.total_out = 0;
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
    
        if(deflateInit(&strm,Z_DEFAULT_COMPRESSION) == Z_OK){
            
            while(status == Z_OK){
                
                if(outlen - strm.total_out <= 0) {
                    outlen += MAX(2048, n);
                    out = duk_resize_buffer(ctx, -1, outlen);
                }
                
                strm.next_out = (Bytef *) out + strm.total_out;
                strm.avail_out = (uInt) (outlen - strm.total_out);
                
                status = deflate(&strm,Z_SYNC_FLUSH);
            }
            
            deflateEnd(&strm);
        }
        
        duk_push_buffer_object(ctx, -1, 0, strm.total_out, DUK_BUFOBJ_ARRAYBUFFER);
        
        duk_remove(ctx, -2);
        
        return 1;
    }
    
    static duk_ret_t Crypto_zlib_inflate_func(duk_context * ctx) {
        
        size_t n = 0;
        void * data = kk::script::toBufferDataArgument(ctx, 0, &n);
        
        if(!data) {
            return 0;
        }
        
        size_t outlen = n * 1.5;
        void * out = duk_push_dynamic_buffer(ctx, outlen);
        
        int status = Z_OK;
        z_stream strm;
        
        strm.next_in = (Bytef *) data;
        strm.avail_in = (uInt) n;
        strm.avail_out = 0;
        strm.total_out = 0;
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        
        if(inflateInit(&strm) == Z_OK){
            
            while(status == Z_OK){
                
                if(outlen - strm.total_out <= 0) {
                    outlen += MAX(2048, n);
                    out = duk_resize_buffer(ctx, -1, outlen);
                }
                
                strm.next_out = (Bytef *) out + strm.total_out;
                strm.avail_out = (uInt) (outlen - strm.total_out);
                
                status = inflate(&strm,Z_SYNC_FLUSH);
            }
            
            inflateEnd(&strm);
        }
        
        duk_push_buffer_object(ctx, -1, 0, strm.total_out, DUK_BUFOBJ_ARRAYBUFFER);
        
        duk_remove(ctx, -2);
        
        return 1;
    }
    
    static duk_ret_t Crypto_zlib_gzip_func(duk_context * ctx) {
        
        size_t n = 0;
        void * data = kk::script::toBufferDataArgument(ctx, 0, &n);
        
        if(!data) {
            return 0;
        }
        
        size_t outlen = n * 1.5;
        void * out = duk_push_dynamic_buffer(ctx, outlen);
        
        int status = Z_OK;
        z_stream strm;
        
        strm.next_in = (Bytef *) data;
        strm.avail_in = (uInt) n;
        strm.avail_out = 0;
        strm.total_out = 0;
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        
        if(deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15+32, 8, Z_DEFAULT_STRATEGY)){
            
            while(status == Z_OK){
                
                if(outlen - strm.total_out <= 0) {
                    outlen += MAX(2048, n);
                    out = duk_resize_buffer(ctx, -1, outlen);
                }
                
                strm.next_out = (Bytef *) out + strm.total_out;
                strm.avail_out = (uInt) (outlen - strm.total_out);
                
                status = deflate(&strm,Z_SYNC_FLUSH);
            }
            
            deflateEnd(&strm);
        }
        
        duk_push_buffer_object(ctx, -1, 0, strm.total_out, DUK_BUFOBJ_ARRAYBUFFER);
        
        duk_remove(ctx, -2);
        
        return 1;
    }
    
    static duk_ret_t Crypto_zlib_gunzip_func(duk_context * ctx) {
        
        size_t n = 0;
        void * data = kk::script::toBufferDataArgument(ctx, 0, &n);
        
        if(!data) {
            return 0;
        }
        
        size_t outlen = n * 1.5;
        void * out = duk_push_dynamic_buffer(ctx, outlen);
        
        int status = Z_OK;
        z_stream strm;
        
        strm.next_in = (Bytef *) data;
        strm.avail_in = (uInt) n;
        strm.avail_out = 0;
        strm.total_out = 0;
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        
        if(inflateInit2(&strm,(15+32))){
            
            while(status == Z_OK){
                
                if(outlen - strm.total_out <= 0) {
                    outlen += MAX(2048, n);
                    out = duk_resize_buffer(ctx, -1, outlen);
                }
                
                strm.next_out = (Bytef *) out + strm.total_out;
                strm.avail_out = (uInt) (outlen - strm.total_out);
                
                status = inflate(&strm,Z_SYNC_FLUSH);
            }
            
            inflateEnd(&strm);
        }
        
        duk_push_buffer_object(ctx, -1, 0, strm.total_out, DUK_BUFOBJ_ARRAYBUFFER);
        
        duk_remove(ctx, -2);
        
        return 1;
    }
    
    void Crypto_openlibs(duk_context * ctx) {
        
        duk_push_global_object(ctx);
        
        duk_push_object(ctx);
        
        duk_push_c_function(ctx, Crypto_MD5_func, 1);
        duk_put_prop_string(ctx, -2, "md5");
        
        duk_push_c_function(ctx, Crypto_zlib_deflate_func, 1);
        duk_put_prop_string(ctx, -2, "deflate");
        
        duk_push_c_function(ctx, Crypto_zlib_inflate_func, 1);
        duk_put_prop_string(ctx, -2, "inflate");
        
        duk_push_c_function(ctx, Crypto_zlib_gzip_func, 1);
        duk_put_prop_string(ctx, -2, "gzip");
        
        duk_push_c_function(ctx, Crypto_zlib_gunzip_func, 1);
        duk_put_prop_string(ctx, -2, "gunzip");
        
        duk_put_prop_string(ctx, -2, "crypto");
        
        duk_pop(ctx);
    }
}
