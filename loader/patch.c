/*
 * patch.c
 *
 * Patching some of the .so internal functions or bridging them to native for
 * better compatibility.
 *
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include <so_util.h>
#include <psp2/kernel/clib.h>
#include <android/native_window.h>
#include "patch.h"
#include "main.h"
#include "utils/logger.h"
#include <kubridge.h>
#include <stdlib.h>
#include <malloc.h>
#include <psp2/kernel/threadmgr.h>
#include <string.h>
#include "reimpl/io.h"
#include "utils/glutil.h"

void android_app_read_cmd() {
    sceClibPrintf("ALABAMA KAKAKAK LLL\n");
}

void (*android_app_write_cmd)(struct android_app* app, int8_t cmd);

void xt_eatVariadicArguments(char* fmt, ...) {
    va_list list;
    char string[1024];

    va_start(list, fmt);
    sceClibVsnprintf(string, 1024, fmt, list);
    va_end(list);
    sceClibPrintf("xt_eatVariadicArguments: ");
    sceClibPrintf(string);
    sceClibPrintf("\n");
}


so_hook _ZN2xt5ArrayIhE7reserveEj_hook;
int _ZN2xt5ArrayIhE7reserveEj(void ** this, size_t len) {
    size_t * current_array_size = (size_t *)( (void*)this + 8);

    if (*current_array_size < len) {
        if (len <= *current_array_size * 2) {
            len = *current_array_size * 2;
        }

        if (len < 5) {
            len = 4;
        }

        void * new_buf = malloc(len);

        if (new_buf == NULL) {
            return 0;
        }

        if (*this) {
            memcpy(new_buf, *this, *(size_t *)(this + 4));
            free(*this);
        }

        *this = new_buf;
        *current_array_size = len;
    }
    return 1;
}

so_hook _ZN15AndroidInStreamC2EP6AAsset_hook;
void * _ZN15AndroidInStreamC2EP6AAsset(void *this,void *param_1) {
    sceClibPrintf("_ZN15AndroidInStreamC2EP6AAsset constructor\n");
    return SO_CONTINUE(void *, _ZN15AndroidInStreamC2EP6AAsset_hook, this, param_1);
}

void * _Znwj(uint size) {
    void * ret = memalign(0x10 ,size);
    if (ret != NULL) {
        return ret;
    }
    sceClibPrintf("operator.new failed with alloc size %i\n", size);
    return NULL;
}

so_hook FUN_001f2358_hook;
void FUN_001f2358(void*param_1) {
    sceClibPrintf("FUN_001f2358\n");
    SO_CONTINUE(void*, FUN_001f2358_hook, param_1);
}

void* _ZN2xt13MemoryManager11allocMemoryEj(size_t size) {
    size = (size + (4 - 1)) & -4; // round up to % 4

    void* ret = malloc(size);
    if (ret == NULL) {
        sceClibPrintf("failed malloc\n");
    }
    return ret;
}



#define undefined4 void *

so_hook _ZN6Shader7compileEPKcS1__hook;
void _ZN6Shader7compileEPKcS1_(void *this,char *param_1,char *param_2) {
    sceClibPrintf("_ZN6Shader7compileEPKcS1_\n");
    SO_CONTINUE(void*, _ZN6Shader7compileEPKcS1__hook, this, param_1, param_2);
}

so_hook FUN_001f2e5c_hook;
undefined4 * FUN_001f2e5c(undefined4 *param_1,char *param_2,undefined4 param_3) {
    sceClibPrintf("FUN_001f2e5c_hook\n");
    return SO_CONTINUE(void*, FUN_001f2e5c_hook, param_1, param_2, param_3);
}

so_hook _ZN2xt10FileSystem10getDefaultEv_hook;
void * _ZN2xt10FileSystem10getDefaultEv(void) {
    sceClibPrintf("_ZN2xt10FileSystem10getDefaultEv\n");
    return SO_CONTINUE(void*, _ZN2xt10FileSystem10getDefaultEv_hook);
}

so_hook _ZN2xt12FileInStreamD1Ev_hook;
void * _ZN2xt12FileInStreamD1Ev(void *this) {
    sceClibPrintf("_ZN2xt12FileInStreamD1Ev_hook: destructor\n");
    return SO_CONTINUE(void*, _ZN2xt12FileInStreamD1Ev_hook, this);
}
so_hook _ZN2xt5ArrayIhED2Ev_hook;
void * _ZN2xt5ArrayIhED2Ev(void **this) {
    return this;
    sceClibPrintf("_ZN2xt5ArrayIhED2Ev: destructor 1 (0x%x)\n", *(void**)this);
    free(*this);
    sceClibPrintf("_ZN2xt5ArrayIhED2Ev: destructor 2\n");
    *this = NULL;
    sceClibPrintf("_ZN2xt5ArrayIhED2Ev: destructor 3\n");
    return this;
}

//void _ZN2xt4HashINS_6StringEPNS_11ReflectTypeEE6rehashEi(void *this,int param_1)


#include <stdbool.h>
#include <vitaGL.h>
#include <psp2/kernel/processmgr.h>

so_hook map_op_brackets_hook;

typedef struct Shader Shader;
typedef void ** basic_string;

/* Shader::compile(char const*, char const*) */

basic_string * (*basic_string_constructor)(basic_string *cpp_string,char *c_string,void *unk);
void (*basic_string_destructor)(basic_string *param_1, void * unk);
void* (*Rb_tree__find)(void* this, basic_string *param_1);

void* (*map_op_brackets)(void* this, basic_string *param_1);

int * (*FUN_001f1e0c)(int *param_1,uint param_2,undefined4 *param_3);
int * (*FUN_001f0c98)(int *param_1,undefined4 *param_2,undefined4 param_3,undefined4 param_4);
void * m_cachedVertexShaders;
void * m_cachedFragmentShaders;
void * m_cachedShaders;

void Shader__compile(Shader *this,char *vert_shader,char *frag_shader)

{
    sceClibPrintf("SHADER COMPILE CALLED\n");

    uint cachedVertShader;
    uint cachedFragShader;
    int *local_3c;
    int *instream;
    basic_string str;
    basic_string str_2;
    basic_string str_3;
    bool loadSuccess;

    int * unused;
    int * unused_2;
    int * unused_3;


    basic_string_constructor(&str,vert_shader,&unused);dbg;
    void * find_ret = Rb_tree__find(m_cachedVertexShaders,&str);dbg;
    basic_string_destructor((void*)str - 0xc,&unused);dbg;
    if (find_ret == so_mod.text_base + 0x2e9f84) {
        cachedVertShader = 0;dbg;
    } else {
        basic_string_constructor(&str,vert_shader,&unused);dbg;
        void ** map_ele = map_op_brackets(m_cachedVertexShaders,&str);dbg;
        cachedVertShader = *map_ele;dbg;
        basic_string_destructor(str - 0xc,&unused);dbg;
    }
    dbg;
    basic_string_constructor(&str,frag_shader,&unused);dbg;
    find_ret = Rb_tree__find(m_cachedFragmentShaders,&str);dbg;
    basic_string_destructor(str - 0xc,&unused);dbg;
    if (find_ret == 0x2e9f9c) {
        cachedFragShader = 0;dbg;
    } else {
        basic_string_constructor(&str,frag_shader,&local_3c);dbg;
        void ** map_ele = map_op_brackets(m_cachedFragmentShaders,&str);dbg;
        cachedFragShader = *map_ele;dbg;
        basic_string_destructor(str - 0xc,&instream);dbg;
    }
    dbg;
    if (cachedVertShader == 0) {
        char vertShaderPathReal[1024];dbg;
        snprintf(vertShaderPathReal, sizeof(vertShaderPathReal), "%s%s", DATA_PATH_INT, vert_shader);dbg;

        if (!file_exists(vertShaderPathReal)) {
            log_error("Vertex shader not found! Check Project Properties, Configuration->Debugging->Workin gDirectory is \'TargetDir\'");
            cachedVertShader = 0;
            loadSuccess = false;
        } else {dbg;
            FILE * shaderf = fopen(vertShaderPathReal, "rb");
            void * shaderbuf;
            size_t shaderlen;

            ffullread(shaderf, &shaderbuf, &shaderlen, 512);

            cachedVertShader = glCreateShader(GL_VERTEX_SHADER);
            glShaderSourceHook(cachedVertShader, 1, (const char**)&shaderbuf, 0);
            glCompileShader(cachedVertShader);
            basic_string_constructor(&str_2, vert_shader, &unused);
            void ** map_ele = map_op_brackets(m_cachedVertexShaders,&str_2);
            *map_ele = cachedVertShader;
            basic_string_destructor(str_2 - 0xc, &unused);
            loadSuccess = true;

            free(shaderbuf);
        }

        if (!loadSuccess) return;
    }
    dbg;
    if (cachedFragShader == 0) {
        char fragShaderPathReal[1024];
        snprintf(fragShaderPathReal, sizeof(fragShaderPathReal), "%s%s", DATA_PATH_INT, frag_shader);

        if (!file_exists(fragShaderPathReal)) {
            log_error("Fragment shader not found!");
            cachedFragShader = 0;
            loadSuccess = false;
        } else {
            FILE * shaderf = fopen(fragShaderPathReal, "rb");
            void * shaderbuf;
            size_t shaderlen;

            ffullread(shaderf, &shaderbuf, &shaderlen, 512);

            cachedFragShader = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(cachedFragShader, 1, (const char**)&shaderbuf, 0);
            glCompileShader(cachedFragShader);
            basic_string_constructor(&str_2, frag_shader, &unused);
            void ** map_ele = map_op_brackets(m_cachedFragmentShaders,&str_2);
            *map_ele = cachedFragShader;
            basic_string_destructor(str_2 - 0xc, &unused);
            loadSuccess = true;

            free(shaderbuf);
        }

        if (!loadSuccess) return;
    }

    GLuint prog = glCreateProgram();
    *(int *)((int)this + 0x2c) = prog;

    if (prog == 0) {
        logv_error("Couldn't init %s", vert_shader);
        return;
    }

    glAttachShader(prog, cachedVertShader);
    glAttachShader(prog, cachedFragShader);
    glBindAttribLocation(prog,0,"position");
    glBindAttribLocation(prog,1,"texcoord");
    glBindAttribLocation(prog,2,"color");
    glBindAttribLocation(prog,3,"a_boneIndex");
    glBindAttribLocation(prog,4,"normal");

    GLint params;
    glGetShaderiv(cachedVertShader,GL_COMPILE_STATUS,&params);
    if (params == 0) {
        logv_error("Couldn\'t compile %s", vert_shader);
    }
    glGetShaderiv(cachedFragShader,GL_COMPILE_STATUS,&params);
    if (params == 0) {
        logv_error("Couldn\'t compile %s", frag_shader);
    }

    basic_string_constructor(&str_3, vert_shader, &unused);
    basic_string_constructor(&str_2, frag_shader, &unused_2);

    void ** ppiVar3;
    cachedVertShader = *(int *)((void*)str_2 - 0xc) + *(int *)((void*)str_3 + -0xc);
    if ((*(uint *)((int)str_3 + -8) < cachedVertShader) &&
        (cachedVertShader <= *(uint *)(str_2 - 8))) {
        ppiVar3 = (void **)FUN_001f1e0c(&str_2,0,&str_3);
    }
    else {
        ppiVar3 = (void **)FUN_001f0c98(&str_3,&str_2, NULL, NULL);
    }
    void* tmp = *ppiVar3;

    void ** map_ele = map_op_brackets(m_cachedShaders,&tmp);
    *map_ele = prog;

    basic_string_destructor(tmp - 0xc, &unused_3);
    basic_string_destructor(str_2 - 0xc, &unused_2);
    basic_string_destructor(str_3 - 0xc, &unused);
}

uint32_t _ZN2xt4Time10getSecondsEv(void) {
    uint32_t ret = sceKernelGetProcessTimeLow();
    sceClibPrintf("requested time get seconds, ret %i\n", ret);
    return ret;
}


so_hook _Z9startGamev_hook;

void _Z9startGamev(void) {
    sceClibPrintf("_Z22loadLevelScriptPatternv called!\n");
    SO_CONTINUE(void*, _Z9startGamev_hook);
    sceClibPrintf("_Z22loadLevelScriptPatternv returned!\n");
}

so_hook _ZN5Music9playMusicEjb_hook;
float _ZN5Music9playMusicEjb(uint p1, bool p2) {
    sceClibPrintf("_ZN5Music9playMusicEjb called!\n");
    float ret = SO_CONTINUE(float, _ZN5Music9playMusicEjb_hook, p1, p2);
    //float ret = 1.0f;
    sceClibPrintf("_ZN5Music9playMusicEjb returned!\n");
    return ret;
}

so_hook _ZN2xt5Input17enqueueTouchEventEiNS_10TouchPhase4EnumERKNS_7Vector2IiEE_hook;
void _ZN2xt5Input17enqueueTouchEventEiNS_10TouchPhase4EnumERKNS_7Vector2IiEE(int param_1,void* param_2,void *param_3) {
    sceClibPrintf("_ZN2xt5Input17enqueueTouchEventEiNS_10TouchPhase4EnumERKNS_7Vector2IiEE called!\n");
    SO_CONTINUE(void*, _ZN2xt5Input17enqueueTouchEventEiNS_10TouchPhase4EnumERKNS_7Vector2IiEE_hook, param_1, param_2, param_3);
    sceClibPrintf("_ZN2xt5Input17enqueueTouchEventEiNS_10TouchPhase4EnumERKNS_7Vector2IiEE returned!\n");
}

so_hook _ZN2xt10EventQueue15tryDequeueEventERNS_14AlignedStorageILi256ELi4EEE_hook;

void * _ZN2xt10EventQueue15tryDequeueEventERNS_14AlignedStorageILi256ELi4EEE(void* p) {
    sceClibPrintf("tryDequeueEventERNS_14AlignedStorageILi256ELi4EEE called\n");
    void* ret = SO_CONTINUE(void*, _ZN2xt10EventQueue15tryDequeueEventERNS_14AlignedStorageILi256ELi4EEE_hook, p);
    sceClibPrintf("tryDequeueEventERNS_14AlignedStorageILi256ELi4EEE returned\n");
    return ret;
}


so_hook _ZN2xt5Input12processEventERNS_10InputEventE_hook;

void * _ZN2xt5Input12processEventERNS_10InputEventE(void* p) {
    sceClibPrintf("processEventERNS_10InputEventE called\n");
    void* ret = SO_CONTINUE(void*, _ZN2xt5Input12processEventERNS_10InputEventE_hook, p);
    sceClibPrintf("processEventERNS_10InputEventE returned\n");
    return ret;
}

so_hook _ZN2xt5Input9endUpdateEv_hook;
double _ZN2xt5Input9endUpdateEv(void) {
    sceClibPrintf("_ZN2xt5Input9endUpdateEv called \n");
    double ret = SO_CONTINUE(double, _ZN2xt5Input9endUpdateEv_hook);
    sceClibPrintf("_ZN2xt5Input9endUpdateEv returned \n");
    return ret;
}

so_hook _ZN11AppMinigore6updateEf_hook;
int _ZN11AppMinigore6updateEf(void* v, float f, int i) {
    sceClibPrintf("_ZN11AppMinigore6updateEf called \n");
    int ret = SO_CONTINUE(int, _ZN11AppMinigore6updateEf_hook, v, f, i);
    sceClibPrintf("_ZN11AppMinigore6updateEf returned %i \n", ret);
    return ret;
}

so_hook _ZN15AnimationSystem6updateEf_hook;
float _ZN15AnimationSystem6updateEf(float f) {
    sceClibPrintf("_ZN15AnimationSystem6updateEf called \n");
    float ret = SO_CONTINUE(float, _ZN15AnimationSystem6updateEf_hook, f);
    sceClibPrintf("_ZN15AnimationSystem6updateEf returned %f \n", ret);
    return ret;
}


so_hook _ZN2xt5Imgui10beginFrameEf_hook;
void _ZN2xt5Imgui10beginFrameEf(float f) {
    sceClibPrintf("_ZN2xt5Imgui10beginFrameEf called \n");
    SO_CONTINUE(void*, _ZN2xt5Imgui10beginFrameEf_hook, f);
    sceClibPrintf("_ZN2xt5Imgui10beginFrameEf returned\n");
}

so_hook _ZN5Voice6updateEf_hook;
void _ZN5Voice6updateEf(void* v, float f) {
    sceClibPrintf("_ZN5Voice6updateEf called \n");
    SO_CONTINUE(void*, _ZN5Voice6updateEf_hook, v, f);
    sceClibPrintf("_ZN5Voice6updateEf returned\n");
}

so_hook _ZN2xt5Imgui8endFrameEv_hook;
void _ZN2xt5Imgui8endFrameEv(void) {
    sceClibPrintf("_ZN2xt5Imgui8endFrameEv called \n");
    SO_CONTINUE(void*, _ZN2xt5Imgui8endFrameEv_hook);
    sceClibPrintf("_ZN2xt5Imgui8endFrameEv returned\n");
}


so_hook _ZN2xt7Channel4stopEv_hook;
void _ZN2xt7Channel4stopEv(void* this) {
    sceClibPrintf("_ZN2xt7Channel4stopEv called \n");
    SO_CONTINUE(void*, _ZN2xt7Channel4stopEv_hook, this);
    sceClibPrintf("_ZN2xt7Channel4stopEv returned\n");
}

so_hook _ZN2xt7Channel5clearEv_hook;
void _ZN2xt7Channel5clearEv(void* this) {

    uint uVar1;
    dbg;
    if (*(int *)this != 2) {
        dbg;
        int (*fun)(void);
        fun = (int (*)(void)) (((**(int **) (this + 0xc)) + 4));
        sceClibPrintf("fun addr a: 0x%x!\n", fun);
        sceClibPrintf("fun addr b: 0x%x!\n", this + 0xc);
        sceClibPrintf("fun addr c: 0x%x!\n", *(int*)(this + 0xc));
        sceClibPrintf("fun addr d: 0x%x!\n", **(int**)(this + 0xc));
        uVar1 = fun();
        if (uVar1 < 0x11) {
            return;
        }
        sceClibPrintf("6: xtAndroidSoundSystem: xtAndroidSoundSystem: Invalid OpenSLES error code!\n");
    }
    return;

    sceClibPrintf("_ZN2xt7Channel5clearEv called \n");
    SO_CONTINUE(void*, _ZN2xt7Channel5clearEv_hook, this);
    sceClibPrintf("_ZN2xt7Channel5clearEv returned\n");
}

so_hook _ZN2xt11SoundSystem4Impl14destroyChannelEPNS_7ChannelE_hook;
void _ZN2xt11SoundSystem4Impl14destroyChannelEPNS_7ChannelE(void* this, void * p) {
    sceClibPrintf("_ZN2xt11SoundSystem4Impl14destroyChannelEPNS_7ChannelE called \n");
    SO_CONTINUE(void*, _ZN2xt11SoundSystem4Impl14destroyChannelEPNS_7ChannelE_hook, this, p);
    sceClibPrintf("_ZN2xt11SoundSystem4Impl14destroyChannelEPNS_7ChannelE returned\n");
}
so_hook _Znaj_hook;
void* _Znaj(uint sz) {
    sceClibPrintf("_Znaj called \n");
    void* ret = memalign(0x10, sz);
    sceClibPrintf("_Znaj returned\n");
    return ret;
}

so_hook _ZN2xt11SoundSystem4Impl22createChannelFromAssetEPNS_7ChannelEPKc_hook;
void _ZN2xt11SoundSystem4Impl22createChannelFromAssetEPNS_7ChannelEPKc(void* p1, void* p2) {
    sceClibPrintf("_ZN2xt11SoundSystem4Impl22createChannelFromAssetEPNS_7ChannelEPKc called \n");
    SO_CONTINUE(void*, _ZN2xt11SoundSystem4Impl22createChannelFromAssetEPNS_7ChannelEPKc_hook, p1, p2);
    sceClibPrintf("_ZN2xt11SoundSystem4Impl22createChannelFromAssetEPNS_7ChannelEPKc returned\n");
}

so_hook _ZdaPv_hook;
void _ZdaPv(void* p1) {
    sceClibPrintf("_ZdaPv called \n");
    free(p1);
    sceClibPrintf("_ZdaPv returned\n");
}

so_hook _ZN2xt7Channel4playEv_hook;
void _ZN2xt7Channel4playEv(void* p1) {
    sceClibPrintf("_ZN2xt7Channel4playEv called \n");
    SO_CONTINUE(void*, _ZN2xt7Channel4playEv_hook, p1);
    sceClibPrintf("_ZN2xt7Channel4playEv returned\n");
}


typedef void* Channel;

so_hook _ZN2xt11SoundSystem9playMusicEjb_hook;
void* _ZN2xt11SoundSystem9playMusicEjb(uint param_1, uint param_2, uint param_3) {
    Channel *pCVar1;
    undefined4 *****pppppuVar2;
    int *piVar3;
    uint uVar4;
    int iVar5;
    uint uVar6;
    int *this;
    int iVar7;
    undefined4 *****local_44 [7];

    dbg;

    uVar4 = (uint)param_2;
    if (uVar4 != 0) {
        if (uVar4 < 0x11) {
            iVar5 = *(int *)param_1;
            if (*(int *)(iVar5 + (uVar4 - 1) * 0x48 + 0x3c) == 0) {
                sceClibPrintf("SoundSystem::playMusic: track isn\'t loaded (%d)\n",uVar4,(uint)param_3);
            }
            else {
                if (*(int *)(iVar5 + 8) != 0) {
                    pCVar1 = (Channel *)(so_mod.text_base + 0x364c + iVar5);
                    _ZN2xt7Channel4stopEv(pCVar1);
                    //_ZN2xt7Channel5clearEv(pCVar1);
                    iVar5 = *(int *)param_1;
                    *(undefined4 *)(iVar5 + 8) = 0;
                }
                iVar7 = iVar5 + (uVar4 - 1) * 0x48;
                *(undefined4 *)(iVar7 + 0x50) = 0;
                if ((*(int *)(iVar5 + 0x365c) != 0) &&
                    ((this = (int *)(iVar5 + 0x364c), *(int *)(iVar5 + 0x3658) != 0 ||
                                                             (*this == 2)))) {
                    _ZN2xt7Channel4stopEv((Channel *)this);
                    //_ZN2xt7Channel5clearEv((Channel *)this);
                    _ZN2xt11SoundSystem4Impl14destroyChannelEPNS_7ChannelE(*(void **)param_1,(Channel *)this);
                    uVar6 = *(uint *)(iVar7 + 0x18);
                    pCVar1 = *(Channel **)param_1;
                    if (uVar6 < 0x1c) {
                        pppppuVar2 = local_44;
                    }
                    else {
                        pppppuVar2 = (undefined4 *****)_Znaj(uVar6 + 1);
                        local_44[0] = pppppuVar2;
                    }
                    piVar3 = (int *)*(int *)(iVar7 + 0x1c);
                    if (uVar6 < 0x1c) {
                        piVar3 = (int *)(iVar7 + 0x1c);
                    }
                    memcpy(pppppuVar2,piVar3,uVar6 + 1);
                    _ZN2xt11SoundSystem4Impl22createChannelFromAssetEPNS_7ChannelEPKc(pCVar1,(char *)this);
                    if ((0x1b < uVar6) && (local_44[0] != (undefined4 *****)0x0)) {
                        _ZdaPv(local_44[0]);
                    }
                    if (*this == 2) {
                        log_error("            (**(code **)(**(int **)(&UNK_00003668 + iVar5) + 4))\n"
                                  "                      (*(int **)(&UNK_00003668 + iVar5),1,0,0xffffffff);");
                        //             (**(code **)(**(int **)(&UNK_00003668 + iVar5) + 4))
                        //                      (*(int **)(&UNK_00003668 + iVar5),1,0,0xffffffff);
                    }
                    else {
                        *(undefined4 *)(iVar5 + 0x3654) = 0xffffffff;
                    }
                    _ZN2xt7Channel4playEv((Channel *)this);
                    *(uint *)(*(int *)param_1 + 8) = uVar4;
                }
            }
        }
        else {
            sceClibPrintf("SoundSystem::playMusic: track out of range (%d)\n",uVar4,(uint)param_3);
        }
    }

    return NULL;
}



void so_patch(void) {


    android_app_write_cmd = (void *)(so_mod.text_base + 0x001c3494 + 1);


    /*
    basic_string_constructor = (void *)(so_mod.text_base + 0x001f2e5c + 1);
    basic_string_destructor = (void *)(so_mod.text_base + 0x001f2358 + 1);
    Rb_tree__find = (void *)so_symbol(&so_mod, "_ZNSt8_Rb_treeISsSt4pairIKSsjESt10_Select1stIS2_ESt4lessISsESaIS2_EE4findERS1_");
    map_op_brackets = (void *)so_symbol(&so_mod, "_ZNSt3mapISsjSt4lessISsESaISt4pairIKSsjEEEixEOSs");

    FUN_001f1e0c = (void *)(so_mod.text_base + 0x001f1e0c + 1);
    FUN_001f0c98 = (void *)(so_mod.text_base + 0x001f0c98 + 1);
    m_cachedVertexShaders = (void *)so_symbol(&so_mod, "_ZN6Shader21m_cachedVertexShadersE");
    m_cachedFragmentShaders = (void *)so_symbol(&so_mod, "_ZN6Shader23m_cachedFragmentShadersE");
    m_cachedShaders = (void *)so_symbol(&so_mod, "_ZN6Shader15m_cachedShadersE");

    hook_addr(so_mod.text_base + 0x001285c8 + 1, (uintptr_t)&Shader__compile);*/


    //hook_addr(so_symbol(&so_mod, "_ZN2xt13MemoryManager11allocMemoryEj"), (uintptr_t)&_ZN2xt13MemoryManager11allocMemoryEj);
/*

    //hook_addr(so_symbol(&so_mod, "android_app_read_cmd"), (uintptr_t)&android_app_read_cmd);


    hook_addr(so_symbol(&so_mod, "_Znwj"), (uintptr_t)&_Znwj);
    _ZN2xt5ArrayIhED2Ev_hook = hook_addr(so_symbol(&so_mod, "_ZN2xt5ArrayIhED2Ev"), (uintptr_t)&_ZN2xt5ArrayIhED2Ev);
    //_ZN2xt5ArrayIiE7reserveEj_hook = hook_addr(so_symbol(&so_mod, "_ZN2xt5ArrayIiE7reserveEj"), (uintptr_t)&_ZN2xt5ArrayIiE7reserveEj);
    FUN_001f2358_hook = hook_addr(so_mod.text_base + 0x001f2358 + 1, (uintptr_t)&FUN_001f2358);
    //hook_addr(so_mod.text_base + 0x000e71d8, (uintptr_t)&android_app_read_cmd);
    */

    { // Logging
        hook_addr(so_symbol(&so_mod, "_ZN2xt20eatVariadicArgumentsEz"), (uintptr_t)&xt_eatVariadicArguments);
        hook_addr(so_symbol(&so_mod, "_ZN2xt3LOGEPKcz"), (uintptr_t)&xt_eatVariadicArguments);
    }

    { // Fixing audio issues
        /*
         _ZN5Music9playMusicEjb_hook = hook_addr(so_symbol(&so_mod, "_ZN5Music9playMusicEjb"), (uintptr_t)&_ZN5Music9playMusicEjb);

        _ZN2xt7Channel4stopEv_hook = hook_addr(so_symbol(&so_mod, "_ZN2xt7Channel4stopEv"), (uintptr_t)&_ZN2xt7Channel4stopEv);
        _ZN2xt7Channel5clearEv_hook = hook_addr(so_symbol(&so_mod, "_ZN2xt7Channel5clearEv"), (uintptr_t)&_ZN2xt7Channel5clearEv);
        _ZN2xt11SoundSystem4Impl14destroyChannelEPNS_7ChannelE_hook = hook_addr(so_symbol(&so_mod, "_ZN2xt11SoundSystem4Impl14destroyChannelEPNS_7ChannelE"), (uintptr_t)&_ZN2xt11SoundSystem4Impl14destroyChannelEPNS_7ChannelE);
        _ZN2xt11SoundSystem4Impl22createChannelFromAssetEPNS_7ChannelEPKc_hook = hook_addr(so_symbol(&so_mod, "_ZN2xt11SoundSystem4Impl22createChannelFromAssetEPNS_7ChannelEPKc"), (uintptr_t)&_ZN2xt11SoundSystem4Impl22createChannelFromAssetEPNS_7ChannelEPKc);
        _ZN2xt7Channel4playEv_hook = hook_addr(so_symbol(&so_mod, "_ZN2xt7Channel4playEv"), (uintptr_t)&_ZN2xt7Channel4playEv);
         */
        //_ZdaPv_hook = hook_addr(so_symbol(&so_mod, "_ZdaPv"), (uintptr_t)&_ZdaPv);
        //_Znaj_hook = hook_addr(so_symbol(&so_mod, "_Znaj"), (uintptr_t)&_Znaj);
        //_ZN2xt11SoundSystem9playMusicEjb_hook = hook_addr(so_symbol(&so_mod, "_ZN2xt11SoundSystem9playMusicEjbb"), (uintptr_t)&_ZN2xt11SoundSystem9playMusicEjb);
        //hook_addr(so_symbol(&so_mod, "_ZN2xt7Channel5clearEv"), (uintptr_t)&ret0);

    }

    sceClibPrintf("ababva22\n");


    //_ZN2xt12FileInStreamD1Ev_hook = hook_addr(so_symbol(&so_mod, "_ZN15AndroidInStreamD2Ev"), (uintptr_t)&_ZN2xt12FileInStreamD1Ev);
    _ZN2xt5ArrayIhED2Ev_hook = hook_addr(so_symbol(&so_mod, "_ZN2xt5ArrayIhED2Ev"), (uintptr_t)&_ZN2xt5ArrayIhED2Ev);
    /*_ZN2xt5ArrayIhE7reserveEj_hook = hook_addr(so_symbol(&so_mod, "_ZN2xt5ArrayIhE7reserveEj"), (uintptr_t)&_ZN2xt5ArrayIhE7reserveEj);
    _Z9startGamev_hook = hook_addr(so_symbol(&so_mod, "_Z22loadLevelScriptPatternv"), (uintptr_t)&_Z9startGamev);




    _ZN2xt5Input17enqueueTouchEventEiNS_10TouchPhase4EnumERKNS_7Vector2IiEE_hook = hook_addr(so_symbol(&so_mod, "_ZN2xt5Input17enqueueTouchEventEiNS_10TouchPhase4EnumERKNS_7Vector2IiEE"), (uintptr_t)&_ZN2xt5Input17enqueueTouchEventEiNS_10TouchPhase4EnumERKNS_7Vector2IiEE);
    _ZN2xt10EventQueue15tryDequeueEventERNS_14AlignedStorageILi256ELi4EEE_hook = hook_addr(so_symbol(&so_mod, "_ZN2xt10EventQueue15tryDequeueEventERNS_14AlignedStorageILi256ELi4EEE"), (uintptr_t)&_ZN2xt10EventQueue15tryDequeueEventERNS_14AlignedStorageILi256ELi4EEE);
    _ZN2xt5Input12processEventERNS_10InputEventE_hook = hook_addr(so_symbol(&so_mod, "_ZN2xt5Input12processEventERNS_10InputEventE"), (uintptr_t)&_ZN2xt5Input12processEventERNS_10InputEventE);
    _ZN2xt5Input9endUpdateEv_hook = hook_addr(so_symbol(&so_mod, "_ZN2xt5Input9endUpdateEv"), (uintptr_t)&_ZN2xt5Input9endUpdateEv);
    _ZN11AppMinigore6updateEf_hook = hook_addr(so_symbol(&so_mod, "_ZN11AppMinigore6updateEf"), (uintptr_t)&_ZN11AppMinigore6updateEf);
    _ZN15AnimationSystem6updateEf_hook = hook_addr(so_symbol(&so_mod, "_ZN15AnimationSystem6updateEf"), (uintptr_t)&_ZN15AnimationSystem6updateEf);
    _ZN2xt5Imgui10beginFrameEf_hook = hook_addr(so_symbol(&so_mod, "_ZN2xt5Imgui10beginFrameEf"), (uintptr_t)&_ZN2xt5Imgui10beginFrameEf);
    _ZN5Voice6updateEf_hook = hook_addr(so_symbol(&so_mod, "_ZN5Voice6updateEf"), (uintptr_t)&_ZN5Voice6updateEf);
    _ZN2xt5Imgui8endFrameEv_hook = hook_addr(so_symbol(&so_mod, "_ZN2xt5Imgui8endFrameEv"), (uintptr_t)&_ZN2xt5Imgui8endFrameEv);
*/

    //map_op_brackets_hook = hook_addr(so_symbol(&so_mod, "_ZNSt3mapISsjSt4lessISsESaISt4pairIKSsjEEEixEOSs"), (uintptr_t)&map_op_brackets);

    //_ZN6Shader7compileEPKcS1__hook = hook_addr(so_symbol(&so_mod, "_ZN6Shader7compileEPKcS1_"), (uintptr_t)&_ZN6Shader7compileEPKcS1_);
    //_ZN2xt10FileSystem10getDefaultEv_hook = hook_addr(so_symbol(&so_mod, "_ZN2xt10FileSystem10getDefaultEv"), (uintptr_t)&_ZN2xt10FileSystem10getDefaultEv);
    //_ZN15AndroidInStreamC2EP6AAsset_hook = hook_addr(so_symbol(&so_mod, "_ZN15AndroidInStreamC2EP6AAsset"), (uintptr_t)&_ZN15AndroidInStreamC2EP6AAsset);
    //FUN_001f2e5c_hook = hook_addr(so_mod.text_base + 0x001f2e5c + 1, (uintptr_t)&FUN_001f2e5c);

    //hook_addr(so_symbol(&so_mod, "_ZN2xt4Time10getSecondsEv"), (uintptr_t)&_ZN2xt4Time10getSecondsEv);


/*
    uint32_t * og = (int32_t *)(so_mod.text_base + 0x00139898);
    sceClibPrintf("og is 0x%x (should be 0xf8d0b151)\n", *og);

    uint32_t patch = 0xf8d0b951;
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x00139898), &patch, sizeof(patch));
    sceClibPrintf("after patch is 0x%x (should be 0xf8d0b951)\n", *og);
*/

/*
    uint16_t nop = 0xbf00;
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x00142968), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x0014296a), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x0014296c), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x0014296e), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x00142970), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x00142972), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x00142974), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x00142976), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x00142978), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x0014297a), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x0014297c), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x0014297e), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x00142980), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x00142982), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x00142984), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x00142986), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x00142988), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x0014298a), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x0014298c), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x0014298e), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x00142990), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x00142992), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x00142994), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x00142996), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x00142998), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x0014299a), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x0014299c), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x0014299e), &nop, sizeof(nop));
    kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x001429a0), &nop, sizeof(nop));*/

}
