

cmake_minimum_required(VERSION 3.4.1)

add_library(duktape
            SHARED
            IMPORTED)

set_target_properties(duktape
            PROPERTIES IMPORTED_LOCATION
            ${CMAKE_SOURCE_DIR}/lib/${ANDROID_ABI}/libduktape.so)

add_library(event
            SHARED
            IMPORTED)

set_target_properties(event
            PROPERTIES IMPORTED_LOCATION
            ${CMAKE_SOURCE_DIR}/lib/${ANDROID_ABI}/libevent.so)

add_library(kk-object
            SHARED
            src/main/jni/src/kk-object.cc
            src/main/jni/src/kk-script.cc
            src/main/jni/src/kk-string.cc
            src/main/jni/src/kk-binary.cc
            src/main/jni/src/kk-bio.cc
            src/main/jni/src/kk-block.cc
            src/main/jni/src/kk-chan.cc
            src/main/jni/src/kk-crypto.cc
            src/main/jni/src/kk-dispatch.cc
            src/main/jni/src/kk-ev.cc
            src/main/jni/src/kk-event.cc
            src/main/jni/src/kk-http.cc
            src/main/jni/src/kk-wk.cc
            src/main/jni/src/kk-ws.cc
            src/main/jni/src/kk-atomic-pthread.cc
            src/main/jni/md5/md5.c)

include_directories(
            src/main/jni/include
            src/main/jni/kk-duktape/include
            src/main/jni/kk-event/include
            src/main/jni/md5
            )

set_target_properties(kk-object PROPERTIES
    CXX_STANDARD 11
)

target_link_libraries(
    kk-object
    duktape
    event
    log
    GLESv2
    EGL)
    
