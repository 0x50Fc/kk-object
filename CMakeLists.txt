

cmake_minimum_required(VERSION 3.4.1)

add_library(kk-object
            STATIC
            src/kk-object.cc
            src/kk-script.cc
            src/kk-string.cc
            src/kk-binary.cc
            src/kk-bio.cc
            src/kk-block.cc
            src/kk-chan.cc
            src/kk-crypto.cc
            src/kk-dispatch.cc
            src/kk-ev.cc
            src/kk-event.cc
            src/kk-http.cc
            src/kk-wk.cc
            src/kk-ws.cc
            src/kk-atomic-pthread.cc
            lib/md5/md5.c)

include_directories(
            ./include
            ../kk-duktape/include
            ../kk-event/include
            lib/md5
            )

set_target_properties(kk-object PROPERTIES
    CXX_STANDARD 11
)


