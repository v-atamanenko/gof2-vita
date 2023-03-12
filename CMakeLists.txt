cmake_minimum_required(VERSION 3.14)

if (NOT DEFINED ENV{VITASDK} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "Please define VITASDK env var to point to your SDK path "
          "or set CMAKE_TOOLCHAIN_FILE.")
endif ()

set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake"
    CACHE PATH "Toolchain file")

project(so_loader C CXX)

include("${VITASDK}/share/vita.cmake" REQUIRED)

# Vita application configuration
set(VITA_APP_NAME "Galaxy on Fire 2")
set(VITA_TITLEID "GALAXYOF2")
set(VITA_VPKNAME "GalaxyOnFire2")
set(VITA_VERSION "01.00")
set(VITA_MKSFOEX_FLAGS "-d ATTRIBUTE2=12")

# Optional: used for "send" and "dump" targets
set(PSVITAIP "192.168.1.98" CACHE STRING "PSVita IP (for FTP access)")
set(PARSECOREPATH "$ENV{PARSECOREPATH}" CACHE STRING "vita-parse-core main.py")

# Game-specific definitions
set(DATA_PATH "ux0:data/gof2/" CACHE STRING "Path to data (with trailing /)")
set(SO_PATH "lib/armeabi/libgof2.so" CACHE STRING "Path to .so")
set(APK_PATH "${DATA_PATH}GalaxyOnFire.apk" CACHE STRING "Path to .apk")

add_definitions(-DDATA_PATH="${DATA_PATH}"
                -DDATA_PATH_INT="${DATA_PATH_INT}"
                -DAPK_PATH="${APK_PATH}"
                -DSO_PATH="${SO_PATH}")

# makes sincos, sincosf, etc. visible
add_definitions(-D_GNU_SOURCE -D__POSIX_VISIBLE=999999)

# Unique debug definition for loader's logging to not interfere with anything
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-DDEBUG_SOLOADER)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -O3 -g -ffast-math -mfloat-abi=softfp")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=gnu++20 -Wno-write-strings -Wno-psabi")

add_executable(${CMAKE_PROJECT_NAME}
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
               loader/utils/init.c
               loader/utils/dialog.c
               loader/utils/glutil.c
               loader/utils/logger.c
               loader/utils/settings.c
               loader/utils/utils.c
               loader/AFakeNative/ALooper.cpp
               lib/FalsoJNI/FalsoJNI.c
               lib/FalsoJNI/FalsoJNI_ImplBridge.c
               lib/FalsoJNI/FalsoJNI_Logger.c
               lib/sha1/sha1.c
               lib/so_util/so_util.c
               lib/unzip/unzip.c
               lib/unzip/ioapi.c
               loader/AFakeNative/PseudoEpoll.cpp
               loader/AFakeNative/AFakeNative_Utils.cpp
               loader/AFakeNative/ANativeWindow.cpp
               loader/AFakeNative/ASensor.cpp
               loader/AFakeNative/AConfiguration.cpp
               loader/AFakeNative/AInput.cpp
               loader/AFakeNative/utils/controls.cpp
               loader/AFakeNative/utils/sensors.cpp
               loader/AFakeNative/ANativeActivity.cpp)

target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM
                           PUBLIC ${CMAKE_SOURCE_DIR}/lib
                           )

target_include_directories(${CMAKE_PROJECT_NAME}
                           PUBLIC ${CMAKE_SOURCE_DIR}/loader
                           )

target_link_libraries(${CMAKE_PROJECT_NAME}
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
                      SceCtrl_stub
                      SceGxm_stub
                      ScePower_stub
                      SceTouch_stub
                      SceVshBridge_stub
                      )

add_executable(configurator
               loader/configurator/main.cpp
               loader/utils/settings.c
               )

target_include_directories(configurator SYSTEM
                           PUBLIC ${CMAKE_SOURCE_DIR}/lib
                           )

target_link_libraries(configurator
                      imgui
                      vitaGL
                      vitashark
                      SceShaccCgExt
                      m
                      mathneon
                      taihen_stub
                      SceAppMgr_stub
                      SceAudio_stub
                      SceCtrl_stub
                      SceCommonDialog_stub
                      SceDisplay_stub
                      SceKernelDmacMgr_stub
                      SceGxm_stub
                      SceShaccCg_stub
                      SceSysmodule_stub
                      SceTouch_stub
                      SceVshBridge_stub
                      )

vita_create_self(configurator.bin configurator UNSAFE)

vita_create_self(eboot.bin ${CMAKE_PROJECT_NAME} UNSAFE)
vita_create_vpk(${VITA_VPKNAME}.vpk ${VITA_TITLEID} eboot.bin
                VERSION ${VITA_VERSION}
                NAME ${VITA_APP_NAME}
                FILE ${CMAKE_SOURCE_DIR}/livearea/icon0.png sce_sys/icon0.png
                FILE ${CMAKE_SOURCE_DIR}/livearea/pic0.png sce_sys/pic0.png
                ${CMAKE_SOURCE_DIR}/livearea/mkktbkkt.png sce_sys/livearea/contents/mkktbkkt.png
                ${CMAKE_SOURCE_DIR}/livearea/bg0.png sce_sys/livearea/contents/bg0.png
                ${CMAKE_SOURCE_DIR}/livearea/template.xml sce_sys/livearea/contents/template.xml
                ${CMAKE_SOURCE_DIR}/shaders/gxp shaders
                ${CMAKE_SOURCE_DIR}/loader/configurator/configurator-bg.png configurator-bg.png
                ${CMAKE_SOURCE_DIR}/loader/configurator/SourceSansPro-Regular.ttf SourceSansPro-Regular.ttf
                ${CMAKE_BINARY_DIR}/configurator.bin configurator.bin
                ${CMAKE_SOURCE_DIR}/patch patch
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
                  COMMAND ${CMAKE_SOURCE_DIR}/scripts/get_dump.sh ${PSVITAIP}
                  COMMAND python3.9 ${PARSECOREPATH} coredump ${CMAKE_PROJECT_NAME}
                  )

add_custom_target(reboot
                  COMMAND echo reboot | nc ${PSVITAIP} 1338
                  )
