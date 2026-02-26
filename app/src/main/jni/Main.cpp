#include <list>
#include <vector>
#include <string.h>
#include <pthread.h>
#include <cstring>
#include <jni.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <dlfcn.h>

#include "Includes/Chams.h"
#include "Includes/Logger.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"

#include "KittyMemory/MemoryPatch.h"
#include "And64InlineHook/And64InlineHook.hpp"
#include "Menu/Register.h"
#include "AutoHook/AutoHook.h"

#include <Substrate/SubstrateHook.h>
#include <Substrate/CydiaSubstrate.h>

#include "Hooks.h"

//Target lib here
#define targetLibName OBFUSCATE("libil2cpp.so")

#include "Includes/Macros.h"

struct My_Patches {

    MemoryPatch Fire, Reload, Grenade, Spawn;

} hexPatches;

bool Fire = false;
bool Reload = false;
bool Grenade = false;
bool Spawn = false;
int Ammo = false;
int Firerate = false;
int Gm = false;
float Fastresp = false;
int Redscore = false;
int Bluescore = false;
float Weaponfov;
float speedvalue = 0;

// Chams code
bool wireframe, glow, outline, chams, rainbow;

// Hooking examples. Assuming you know how to write hook


void (*old_FirerateUpdate)(void *instance);

void FirerateUpdateMod(void *instance) {
    if (instance != NULL) {
        if (Ammo) {
            *(int *) ((uint32_t) instance + 0x88) = 999;
        }
        if (Firerate) {
            *(int *) ((uint32_t) instance + 0x1C) = 999;
        }
    }
    return old_FirerateUpdate(instance);
}

void (*old_GmUpdate)(void *instance);

void GmUpdateMod(void *instance) {
    if (instance != NULL) {
        if (Gm) {
            *(int *) ((uint32_t) instance + 0x60) = 9999;
        }
        if (Fastresp) {
            *(float *) ((uint32_t) instance + 0x5C) = -1;
        }
        if (speedvalue) {
            *(float *) ((uint32_t) instance + 0x80) = speedvalue;
        }
    }
    return old_GmUpdate(instance);
}

void (*old_RedscoreUpdate)(void *instance);

void RedscoreUpdateMod(void *instance) {
    if (instance != NULL) {
        if (Redscore) {
            *(int *) ((uint32_t) instance + 0x94) = 999;
        }
        if (Bluescore) {
            *(int *) ((uint32_t) instance + 0x90) = 999;
        }
    }
    return old_RedscoreUpdate(instance);
}

float (*old_Weapon)(void *instance);

float Weapon(void *instance) {
    if (instance != NULL && Weaponfov) {
        return (float) Weaponfov;
    }
    return old_Weapon(instance);
}


ESP espOverlay;

void DrawESP(ESP esp, int screenWidth, int screenHeight) {

    screenW = screenWidth;
    screenH = screenHeight;

    esp.DrawFilledCircle(Color(255, 255, 255, 255), Vector2(screenWidth / 2, screenHeight / 9),
                         48.0f);
    std::string count;
    count += std::to_string(players.size());
    esp.DrawText(Color(0, 0, 0, 255), count.c_str(), Vector2(screenWidth / 2, screenHeight / 8),
                 65.0f);

    if (aimbot) {
        esp.DrawCircle(Color(255, 255, 255, 255), 1.0f, Vector2(screenWidth / 2, screenHeight / 2),
                       fov);
    }

    if (Esp) {

        for (int i = 0; i < players.size(); i++) {
            auto Player = players[i];
            if (Player != nullptr) {
                if (isAlive(Player)) {
                    Vector3 Pos3d = get_position(get_transform(Player));
                    Vector3 FakePos = {Pos3d.x, Pos3d.y + 1.3, Pos3d.z};
                    Vector3 Pos2d = worldtoscreen(get_current(), FakePos);
                    if (Pos2d.z < 1.0f) continue;

                    Vector2 From = Vector2(screenWidth / 2, screenHeight / 8.8);
                    Vector2 To = Vector2(Pos2d.x, screenHeight - Pos2d.y);

                    if (EspLine) {
                        esp.DrawLine(Color(255, 255, 255, 255), 1.0f, From, To);
                    }

                    if (EspBox) {
                        float boxHeight = 200.0f;   // высота бокса (настрой себе)
                        float boxWidth = 80.0f;    // ширина бокса

                        Rect rect;
                        rect.x = Pos2d.x - boxWidth / 2;
                        rect.y = screenHeight - Pos2d.y - boxHeight;
                        rect.width = boxWidth;
                        rect.height = boxHeight;

                        esp.DrawBox(Color(255, 255, 255, 255), 2.0f, rect);
                    }

                } else {
                    players.clear();
                    clearPlayers();
                }
            }
        }
    }

}

void *hack_thread(void *) {
    ProcMap il2cppMap;
    do {
        il2cppMap = KittyMemory::getLibraryMap("libil2cpp.so");
        sleep(1);
    } while (!isLibraryLoaded(targetLibName) && mlovinit());
    setShader("_MainTex");
    LogShaders();
    Wallhack();

#if defined(__aarch64__) //To compile this code for arm64 lib only. Do not worry about greyed out highlighting code, it still works


#else //To compile this code for armv7 lib only.

    // public sealed class Camera
    // public static Camera get_main()
    get_current = (void *(*)()) getAbsoluteAddress(targetLibName, 0x208E7D0);

    // public class Component
    // public Transform get_transform()
    get_transform = (void *(*)(void *)) getAbsoluteAddress(targetLibName, 0x20B925C);

    // public class Transform
    // public Vector3 get_position() { }
    get_position = (Vector3 (*)(void *)) getAbsoluteAddress(targetLibName, 0x20C7198);

    // public sealed class Camera
    // public Vector3 WorldToScreenPoint(Vector3 position) { }
    worldtoscreen = (Vector3 (*)(void *, Vector3)) getAbsoluteAddress(targetLibName, 0x208E4DC);

    // public class Object
    // private static bool IsNativeObjectAlive
    isAlive = (bool *(*)(void *)) getAbsoluteAddress(targetLibName, 0x20C07AC);

    // public class NpcControl
    // private void Update() { }
    MSHookFunction((void *) getAbsoluteAddress(targetLibName, 0x9B96D8), (void *) &_update,
                   (void **) &old_update); // Accoring to your game . Hope You Find this offset 😁

    // public class PlayerControl
    // private void Update() { }
    MSHookFunction((void *) getAbsoluteAddress(targetLibName, 0x9D6A6C), (void *) &_myPlayer,
                   (void **) &old_myPlayer); // Accoring to your game . Hope You Find this offset 😁

    hexPatches.Fire = MemoryPatch::createWithHex("libil2cpp.so", 0xA5966C, "1EFF2FE1");

    hexPatches.Reload = MemoryPatch::createWithHex("libil2cpp.so", 0xA5A040, "1EFF2FE1");

    hexPatches.Grenade = MemoryPatch::createWithHex("libil2cpp.so", 0xA574AC, "1EFF2FE1");

    hexPatches.Spawn = MemoryPatch::createWithHex("libil2cpp.so", 0x9C9408, "1EFF2FE1");

    MSHookFunction((void *) getAbsoluteAddress("libil2cpp.so", 0xA590E8),
                   (void *) FirerateUpdateMod, (void **) &old_FirerateUpdate);

    MSHookFunction((void *) getAbsoluteAddress("libil2cpp.so", 0x9DEA90), (void *) GmUpdateMod,
                   (void **) &old_GmUpdate);

    MSHookFunction((void *) getAbsoluteAddress("libil2cpp.so", 0x97B66C),
                   (void *) RedscoreUpdateMod, (void **) &old_RedscoreUpdate);

    MSHookFunction((void *) getAbsoluteAddress("libil2cpp.so", 0x208DECC), (void *) Weapon,
                   (void **) &old_Weapon);


    LOGI(OBFUSCATE("Done"));
#endif

    return NULL;
}

jobjectArray GetFeatureList(JNIEnv *env, jobject context) {
    jobjectArray ret;

    const char *features[] = {
            OBFUSCATE("Category_ESP"),//Not counted
            OBFUSCATE("0_Toggle_Esp"),//case 0
            OBFUSCATE("1_Toggle_Line"),//case 1
            OBFUSCATE("Category_Aimbot"),//Not counted
            OBFUSCATE("2_Toggle_Aimbot"),//case 2
            OBFUSCATE("3_SeekBar_Fov_0_1000"),//case 3
            OBFUSCATE("4_Toggle_EspBox"),//case 4

            OBFUSCATE("Category_MY HACKS"),//Not counted
            OBFUSCATE("5_Toggle_Can't Fire"),//case 5
            OBFUSCATE("6_Toggle_Can't Reload"),//case 6
            OBFUSCATE("7_Toggle_No Grenade Explosion"),//case 7
            OBFUSCATE("8_Toggle_Bots Can't Spawn"),//case 8
            OBFUSCATE("9_SeekBar_Weapon Fov_1_300"),//case 9
            OBFUSCATE("10_Toggle_Infinite Ammo"),//case10
            OBFUSCATE("11_Toggle_High FireRate"),//case11
            OBFUSCATE("12_Toggle_God Mode"),//case12
            OBFUSCATE("13_Toggle_No Respawn Cooldown"),//case13
            OBFUSCATE("14_Toggle_High Red Team Score"),//case14
            OBFUSCATE("15_Toggle_High Blue Team Score"),//case15
            OBFUSCATE("16_SeekBar_Speed_1_100"), //case16

            OBFUSCATE("Category_Chams"),//Not counted
            OBFUSCATE("17_Toggle_Default Chams"),//case 17
            OBFUSCATE("18_Toggle_Wireframe Chams"),//case 18
            OBFUSCATE("19_Toggle_Glow Chams"),//case 19
            OBFUSCATE("20_Toggle_Outline Chams"),//case 20
            OBFUSCATE("21_Toggle_Rainbow Chams"),//case 21
            OBFUSCATE("22_SeekBar_Line Width_0_10"),//case 22
            OBFUSCATE("23_SeekBar_Color Red_0_255"),//case 23
            OBFUSCATE("24_SeekBar_Color Green_0_255"),//case 24
            OBFUSCATE("25_SeekBar_Color Blue_0_255"),//case 25
    };

    int Total_Feature = (sizeof features / sizeof features[0]);
    ret = (jobjectArray)
            env->NewObjectArray(Total_Feature, env->FindClass(OBFUSCATE("java/lang/String")),
                                env->NewStringUTF(""));

    for (int i = 0; i < Total_Feature; i++)
        env->SetObjectArrayElement(ret, i, env->NewStringUTF(features[i]));

    return (ret);
}

void Changes(JNIEnv *env, jclass clazz, jobject obj,
             jint featNum, jstring featName, jint value,
             jboolean boolean, jstring str) {
    switch (featNum) {


        // ESP case code
        case 0:
            Esp = boolean;
            break;
        case 1:
            EspLine = boolean;
            break;
        case 2:
            aimbot = boolean;
            break;
        case 3:
            fov = value;
            break;
        case 4:
            EspBox = boolean;
            break;


            // MY HACKS case code
        case 5:
            Fire = boolean;
            if (Fire) {
                hexPatches.Fire.Modify();
            } else {
                hexPatches.Fire.Restore();
            }
            break;
        case 6:
            Reload = boolean;
            if (Reload) {
                hexPatches.Reload.Modify();
            } else {
                hexPatches.Reload.Restore();
            }
            break;
        case 7:
            Grenade = boolean;
            if (Grenade) {
                hexPatches.Grenade.Modify();
            } else {
                hexPatches.Grenade.Restore();
            }
            break;
        case 8:
            Spawn = boolean;
            if (Spawn) {
                hexPatches.Spawn.Modify();
            } else {
                hexPatches.Spawn.Restore();
            }
            break;
        case 9:
            Weaponfov = value;
            break;
        case 10:
            Ammo = boolean;
            break;
        case 11:
            Firerate = boolean;
            break;
        case 12:
            Gm = boolean;
            break;
        case 13:
            Fastresp = boolean;
            break;
        case 14:
            Redscore = boolean;
            break;
        case 15:
            Bluescore = boolean;
            break;
        case 16:
            speedvalue = value;
            break;

            // Chams case code
        case 17:
            chams = boolean;
            if (chams) {
                SetWallhack(true);
            } else {
                SetWallhack(false);
            }
            break;
        case 18:
            wireframe = boolean;
            if (wireframe) {
                SetWallhackW(true);
            } else {
                SetWallhackW(false);
            }
            break;
        case 19:
            glow = boolean;
            if (glow) {
                SetWallhackG(true);
            } else {
                SetWallhackG(false);
            }
            break;
        case 20:
            outline = boolean;
            if (outline) {
                SetWallhackO(true);
            } else {
                SetWallhackO(false);
            }
            break;
        case 21:
            rainbow = boolean;
            if (rainbow) {
                SetRainbow(true);
            } else {
                SetRainbow(false);
            }
            break;
        case 22:
            SetW(value);
            break;
        case 23:
            SetR(value);
            break;
        case 24:
            SetG(value);
            break;
        case 25:
            SetB(value);
            break;

    }
}

__attribute__((constructor))
void lib_main() {

    pthread_t ptid;
    pthread_create(&ptid, NULL, hack_thread, NULL);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_android_support_Menu_DrawOn(JNIEnv *env, jclass type, jobject espView, jobject canvas) {
    espOverlay = ESP(env, espView, canvas);
    if (espOverlay.isValid()) {
        DrawESP(espOverlay, espOverlay.getWidth(), espOverlay.getHeight());
    }
}

extern "C"
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;

    vm->GetEnv((void **) &env, JNI_VERSION_1_6);

    static const JNINativeMethod menuMethods[] = {
            {OBFUSCATE("Icon"),            OBFUSCATE(
                                                   "()Ljava/lang/String;"),                                                           reinterpret_cast<void *>(Icon)},
            {OBFUSCATE("IconWebViewData"), OBFUSCATE(
                                                   "()Ljava/lang/String;"),                                                           reinterpret_cast<void *>(IconWebViewData)},
            {OBFUSCATE("IsGameLibLoaded"), OBFUSCATE(
                                                   "()Z"),                                                                            reinterpret_cast<void *>(isGameLibLoaded)},
            {OBFUSCATE("Init"),            OBFUSCATE(
                                                   "(Landroid/content/Context;Landroid/widget/TextView;Landroid/widget/TextView;)V"), reinterpret_cast<void *>(Init)},
            {OBFUSCATE("GetFeatureList"),  OBFUSCATE(
                                                   "()[Ljava/lang/String;"),                                                          reinterpret_cast<void *>(GetFeatureList)},
    };

    if (Register(env, "com/android/support/Menu", menuMethods,
                 sizeof(menuMethods) / sizeof(JNINativeMethod)) != 0)
        return JNI_ERR;

    static const JNINativeMethod prefMethods[] = {
            {OBFUSCATE("Changes"),
             OBFUSCATE("(Landroid/content/Context;ILjava/lang/String;IZLjava/lang/String;)V"),
             reinterpret_cast<void *>(Changes)},
    };

    if (Register(env, "com/android/support/Preferences",
                 prefMethods, sizeof(prefMethods) / sizeof(JNINativeMethod)) != 0)
        return JNI_ERR;

    static const JNINativeMethod mainMethods[] = {
            {OBFUSCATE("CheckOverlayPermission"), OBFUSCATE("(Landroid/content/Context;)V"),
             reinterpret_cast<void *>(CheckOverlayPermission)},
    };

    if (Register(env, "com/android/support/Main", mainMethods,
                 sizeof(mainMethods) / sizeof(JNINativeMethod)) != 0)
        return JNI_ERR;

    return JNI_VERSION_1_6;
}
