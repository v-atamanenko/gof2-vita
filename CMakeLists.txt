cmake_minimum_required(VERSION 3.0)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

if(DEFINED ENV{PSVITAIP})
  set(PSVITAIP "$ENV{PSVITAIP}")
else()
  set(PSVITAIP "192.168.1.98")
  message(WARNING "PSVITAIP environment variable is not set, using default IP ${PSVITAIP}")
endif()

if(DEFINED ENV{PARSECOREPATH})
  set(PARSECOREPATH "$ENV{PARSECOREPATH}")
else()
  set(PARSECOREPATH "/Users/v/Repos/vitasdk/vita-parse-core/main.py")
  message(WARNING "PARSECOREPATH environment variable is not set, using default path ${PARSECOREPATH}")
endif()

project(so_loader C CXX)

include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Galaxy on Fire 2")
set(VITA_TITLEID  "GALAXYOF2")
set(VITA_VPKNAME  "GalaxyOnFire2")
set(VITA_VERSION  "01.00")
set(VITA_MKSFOEX_FLAGS "-d ATTRIBUTE2=12")

option(DEBUG "Print debug information to stdout" OFF)
option(DEBUG_GL "Print (very verbose) debug logs of VitaGL to stdout" OFF)

if (DEBUG)
  add_definitions(-DDEBUG_SOLOADER)
endif()
if (DEBUG_GL)
  add_definitions(-DDEBUG_GL)
endif()

SET(DATA_PATH "ux0:data/gof2/" CACHE STRING "Path to data files") # WITH trailing slash!
SET(DATA_PATH_INT "${DATA_PATH}assets/" CACHE STRING "Path to assets folder") # WITH trailing slash!
SET(SO_PATH "${DATA_PATH}lib/armeabi/libgof2.so" CACHE STRING "Path to .so")
SET(APK_PATH "${DATA_PATH}GalaxyOnFire.apk" CACHE STRING "Path to .apk")

add_definitions(-DDATA_PATH="${DATA_PATH}")
add_definitions(-DDATA_PATH_INT="${DATA_PATH_INT}")
add_definitions(-DAPK_PATH="${APK_PATH}")
add_definitions(-DSO_PATH="${SO_PATH}")

# makes sincos, sincosf, etc. visible
add_definitions(-D_GNU_SOURCE)
add_definitions(-D__POSIX_VISIBLE=999999)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -O3 -g -ffast-math -mfloat-abi=softfp")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=gnu++20 -Wno-write-strings -Wno-psabi")

add_executable(so_loader
        loader/main.c
        loader/dynlib.c
        loader/falsojni_impl.c
        loader/patch.c
        loader/reimpl/ctype_patch.c
        loader/reimpl/env.c
        loader/reimpl/io.c
        loader/reimpl/log.c
        loader/reimpl/mem.c
        loader/reimpl/pthr.c
        loader/reimpl/sys.c
        loader/utils/dialog.c
        loader/utils/glutil.c
        loader/utils/logger.c
        loader/utils/utils.c
        loader/AFakeNative/ALooper.cpp
        lib/FalsoJNI/FalsoJNI.c
        lib/FalsoJNI/FalsoJNI_ImplBridge.c
        lib/FalsoJNI/FalsoJNI_Logger.c
        lib/sha1/sha1.c
        lib/so_util/so_util.c
        loader/AFakeNative/PseudoEpoll.cpp
        loader/AFakeNative/AFakeNative_Utils.cpp
        loader/AFakeNative/ANativeWindow.cpp
        loader/AFakeNative/ASensor.cpp
        loader/AFakeNative/AConfiguration.cpp
        loader/AFakeNative/AInput.cpp
        loader/AFakeNative/utils/controls.cpp
        loader/AFakeNative/utils/sensors.cpp
        loader/AFakeNative/ANativeActivity.cpp)

#set(EXCEPTION_SAFETY 1)
#add_subdirectory(lib/kubridge)
#add_dependencies(so_loader kubridge_all)

target_include_directories(so_loader SYSTEM
        PUBLIC ${CMAKE_SOURCE_DIR}/lib/libc_bridge
        PUBLIC ${CMAKE_SOURCE_DIR}/lib/sha1
        PUBLIC ${CMAKE_SOURCE_DIR}/lib/so_util
        PUBLIC ${CMAKE_SOURCE_DIR}/lib/TinyANA
        PUBLIC ${CMAKE_SOURCE_DIR}/lib/eventfd
        PUBLIC ${CMAKE_SOURCE_DIR}/lib
)

target_include_directories(so_loader
        PUBLIC ${CMAKE_SOURCE_DIR}/loader
)

target_link_libraries(so_loader
        -Wl,--whole-archive pthread -Wl,--no-whole-archive
        stdc++
        vitaGL
        vitashark
        SceShaccCgExt
        taihen_stub
        SceShaccCg_stub
        OpenSLES
        OpenAL
        sndfile
        vorbis
        ogg
        vorbisenc
        vorbisfile
        FLAC
        z
        m
        ScePgf_stub
        mathneon
        kubridge_stub
        SceHid_stub
        SceTouch_stub
        SceMotion_stub
        SceAppMgr_stub
        SceAppUtil_stub
        SceAudio_stub
        SceAudioIn_stub
        SceCommonDialog_stub
        SceDisplay_stub
        SceKernelDmacMgr_stub
        SceFios2_stub
        ${CMAKE_SOURCE_DIR}/lib/libc_bridge/out/libSceLibcBridge_stub.a
        SceCtrl_stub
        SceGxm_stub
        ScePower_stub
        SceTouch_stub
        SceVshBridge_stub
)

vita_create_self(eboot.bin so_loader UNSAFE)
vita_create_vpk(${VITA_VPKNAME}.vpk ${VITA_TITLEID} eboot.bin
        VERSION ${VITA_VERSION}
        NAME ${VITA_APP_NAME}
        FILE ${CMAKE_SOURCE_DIR}/livearea/icon0.png sce_sys/icon0.png
        ${CMAKE_SOURCE_DIR}/livearea/startup.png sce_sys/livearea/contents/startup.png
        ${CMAKE_SOURCE_DIR}/livearea/bg0.png sce_sys/livearea/contents/bg0.png
        ${CMAKE_SOURCE_DIR}/livearea/template.xml sce_sys/livearea/contents/template.xml
        ${CMAKE_SOURCE_DIR}/shaders/gxp shaders
)

add_custom_command(TARGET so_loader
        PRE_LINK
        COMMAND cd ${CMAKE_SOURCE_DIR} && make -C lib/libc_bridge
        COMMENT "Building stubs for libc_bridge"
        VERBATIM
)

add_custom_target(send
        COMMAND echo destroy | nc ${PSVITAIP} 1338
        COMMAND echo destroy | nc ${PSVITAIP} 1338
        COMMAND curl -T eboot.bin ftp://${PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/
        COMMAND echo launch ${VITA_TITLEID} | nc ${PSVITAIP} 1338
        DEPENDS ${VITA_VPKNAME}.vpk-vpk
)

add_custom_target(send_kvdb
        COMMAND echo destroy | nc ${PSVITAIP} 1338
        COMMAND curl -T eboot.bin ftp://${PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/
        COMMAND echo launch ux0:/app/${VITA_TITLEID}/eboot.bin | nc ${PSVITAIP} 1338
        DEPENDS ${VITA_VPKNAME}.vpk-vpk
)

add_custom_target(dump
        COMMAND ${CMAKE_SOURCE_DIR}/get_dump.sh ${PSVITAIP}
        COMMAND python3 ${PARSECOREPATH} coredump so_loader
)

add_custom_target(reboot
        COMMAND echo reboot | nc ${PSVITAIP} 1338
)
