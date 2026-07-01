#include <list>
#include <vector>
#include <string.h>
#include <pthread.h>
#include <thread>
#include <cstring>
#include <jni.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <dlfcn.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>
#include "Includes/Chams.h"
#include "Includes/Logger.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "KittyMemory/MemoryPatch.h"
#include "Menu/Setup.h"
// #include "And64InlineHook/And64InlineHook.hpp"

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
bool chams = false;
bool wireframe = false;
bool glow = false;
bool outline = false;
bool rainbow = false;

// Hooking examples. Assuming you know how to write hook


void (*old_FirerateUpdate)(void *instance);

void FirerateUpdateMod(void *instance) {
    if (instance != NULL) {
        if (Ammo) {
            // Namespace: public class Weapon : MonoBehaviour public int ammo; // 0x8C
            *(int *)((uint32_t)instance + 0x8C) = 999;
        }
        if (Firerate) {
            // Namespace: public class Weapon : MonoBehaviour public int fireRate; // 0x20
            *(int *)((uint32_t)instance + 0x20) = 999;
        }
    }
    return old_FirerateUpdate(instance);
}

void (*old_GmUpdate)(void *instance);

void GmUpdateMod(void *instance) {
    if (instance != NULL) {
        if (Gm) {
            // Namespace: public class PlayerControl : NetworkBehaviour private int health; // 0x60
            *(int *)((uint32_t)instance + 0x60) = 9999;
        }
        if (Fastresp) {
            // Namespace: public class PlayerControl : NetworkBehaviour public float respawnTimer; // 0x5C
            *(float *)((uint32_t)instance + 0x5C) = -1;
        }
        if (speedvalue) {
            // Namespace: public class PlayerControl : NetworkBehaviour public float playerSpeed; // 0x80
            *(float *)((uint32_t)instance + 0x80) = speedvalue;
        }
    }
    return old_GmUpdate(instance);
}

void (*old_RedscoreUpdate)(void *instance);

void RedscoreUpdateMod(void *instance) {
    if (instance != NULL) {
        if (Redscore) {
            // Namespace: public class GameManager : NetworkBehaviour public int redScore; // 0x9C
            *(int *)((uint32_t)instance + 0x9C) = 999;
        }
        if (Bluescore) {
            // Namespace: public class GameManager : NetworkBehaviour public int blueScore; // 0x98
            *(int *)((uint32_t)instance + 0x98) = 999;
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

int lineMode = 0;

ESP espOverlay;

void DrawESP(ESP esp, int screenWidth, int screenHeight) {

    int screenW = screenWidth;
    int screenH = screenHeight;

    // Верхний белый круг со счетчиком
    esp.DrawFilledCircle(Color(255, 255, 255, 255), Vector2(screenWidth / 2, screenHeight / 9), 48.0f);
    std::string count = std::to_string(players.size());
    esp.DrawText(Color(0, 0, 0, 255), count.c_str(), Vector2(screenWidth / 2, screenHeight / 8), 65.0f);

    if (aimbot) {
        esp.DrawCircle(Color(255, 255, 255, 255), 1.0f, Vector2(screenWidth / 2, screenHeight / 2), fov);
    }

    if (Esp) {
        for (int i = 0; i < players.size(); i++) {

            auto Player = players[i];
            if (Player == nullptr) continue;
            if (!isAlive(Player)) continue;

            // 1. Получаем базовую позицию (ноги)
            Vector3 BasePos = get_position(get_transform(Player));

            // 2. Позиция головы (+1.85f для Unity-ботов, чтобы бокс покрывал их полностью)
            Vector3 HeadPos = {BasePos.x, BasePos.y + 1.85f, BasePos.z};

            // Переводим в 2D
            Vector3 Base2D = worldtoscreen(get_current(), BasePos);
            Vector3 Head2D = worldtoscreen(get_current(), HeadPos);

            // ЗАЩИТА: Проверка глубины. Если игрок за спиной — игнорируем
            if (Base2D.z < 1.0f || Head2D.z < 1.0f) continue;

            // ИСПРАВЛЕНИЕ ОСИ Y (Инверсия под Android-оверлей)
            // Если в твоем оверлее 0 сверху, а W2S выдает 0 снизу, то переворачиваем:
            Base2D.y = (float)screenHeight - Base2D.y;
            Head2D.y = (float)screenHeight - Head2D.y;

            // ЗАЩИТА ОТ ДВИЖЕНИЯ НА НЕБО: Если координаты вылетели далеко за экран — не рисуем
            if (Base2D.x < -100 || Base2D.x > screenWidth + 100 ||
                Base2D.y < -100 || Base2D.y > screenHeight + 100) {
                continue;
            }

            // Настройка линий (Snaplines)
            Vector2 From;
            if (lineMode == 0)
                From = Vector2(screenWidth / 2, screenHeight * 0.1f);   // Топ
            else if (lineMode == 1)
                From = Vector2(screenWidth / 2, screenHeight * 0.5f);   // Центр
            else if (lineMode == 2)
                From = Vector2(screenWidth / 2, screenHeight * 0.9f);   // Снизу

            // Теперь линия пойдет ТОЧНО на голову конкретного бота, без улетов в центр экрана
            Vector2 To = Vector2(Head2D.x, Head2D.y);

            if (EspLine) {
                esp.DrawLine(Color(255, 255, 255, 255), 1.0f, From, To);
            }

            // РАСЧЕТ ИСПРАВЛЕННОГО БОКСА (Строго на боте)
            if (EspBox) {
                // Высота — это разница между инвертированными координатами ног и головы
                float boxHeight = fabs(Base2D.y - Head2D.y);
                float boxWidth = boxHeight * 0.55f; // Сделал чуть шире под квадратных ботов

                Rect rect;
                // Центрируем X координату бокса ровно по центру модели бота
                rect.x = Head2D.x - (boxWidth / 2.0f);

                // Верхняя точка бокса по оси Y
                rect.y = (Head2D.y < Base2D.y) ? Head2D.y : Base2D.y;

                rect.width = boxWidth;
                rect.height = boxHeight;

                // Рисуем коробку
                esp.DrawBox(Color(255, 255, 255, 255), 2.0f, rect);
            }
        }
    }
}

// we will run our hacks in a new thread so our while loop doesn't block process main thread
void *hack_thread(void *) {
    LOGI(OBFUSCATE("pthread created"));

    //Check if target lib is loaded
    ProcMap il2cppMap;
    do {
        il2cppMap = KittyMemory::getLibraryMap(targetLibName);
        sleep(1);
    } while (!il2cppMap.isValid() && mlovinit());
    setShader("_MainTex");
    LogShaders();
    Wallhack();

    //Anti-lib rename
    /*
    do {
        sleep(1);
    } while (!isLibraryLoaded("libYOURNAME.so"));*/

    LOGI(OBFUSCATE("%s has been loaded"), (const char *) targetLibName);

#if defined(__aarch64__) //To compile this code for arm64 lib only. Do not worry about greyed out highlighting code, it still works
    // Hook example. Comment out if you don't use hook
    // Strings in macros are automatically obfuscated. No need to obfuscate!

#else //To compile this code for armv7 lib only.

    // public sealed class Camera
    // public static Camera get_main()
    get_current = (void *(*)()) getAbsoluteAddress("libil2cpp.so", 0x20DBA78);

    // public class Component
    // public Transform get_transform()
    get_transform = (void *(*)(void *)) getAbsoluteAddress("libil2cpp.so", 0x2106754);

    // public class Transform
    // public Vector3 get_position() { }
    get_position = (Vector3 (*)(void *)) getAbsoluteAddress("libil2cpp.so", 0x2114690);

    // public sealed class Camera
    // public Vector3 WorldToScreenPoint(Vector3 position) { }
    worldtoscreen = (Vector3 (*)(void *, Vector3)) getAbsoluteAddress("libil2cpp.so", 0x20DB784);

    // public class Object
    // private static bool IsNativeObjectAlive
    isAlive = (bool *(*)(void *)) getAbsoluteAddress("libil2cpp.so", 0x210DCA4);

    // public class NpcControl
    // private void Update() { }
    MSHookFunction((void *) getAbsoluteAddress("libil2cpp.so", 0x9CEC58), (void *) &_update,
                   (void **) &old_update); // Accoring to your game . Hope You Find this offset 😁

    // public class PlayerControl
    // private void Update() { }
    MSHookFunction((void *) getAbsoluteAddress("libil2cpp.so", 0x9EC5DC), (void *) &_myPlayer,
                   (void **) &old_myPlayer); // Accoring to your game . Hope You Find this offset 😁

    // Namespace: public class Weapon : MonoBehaviour public void WeaponShoot() { }
    hexPatches.Fire = MemoryPatch::createWithHex("libil2cpp.so", 0xA783A8, "1EFF2FE1");

    // Namespace: public class Weapon : MonoBehaviour public void WeaponReload() { }
    hexPatches.Reload = MemoryPatch::createWithHex("libil2cpp.so", 0xA78D68, "1EFF2FE1");

    // Namespace: public class Grenade : MonoBehaviour private void Update() { }
    hexPatches.Grenade = MemoryPatch::createWithHex("libil2cpp.so", 0xA75ECC, "1EFF2FE1");

    // Namespace: public class NpcManager : NetworkBehaviour private void SpawnNpc(int _count, string _team) { }
    hexPatches.Spawn = MemoryPatch::createWithHex("libil2cpp.so", 0x9DEA38, "1EFF2FE1");

    // Namespace: public class Weapon : MonoBehaviour private void Update() { }
    MSHookFunction((void *) getAbsoluteAddress("libil2cpp.so", 0xA77C8C),
                   (void *) FirerateUpdateMod, (void **) &old_FirerateUpdate);

    // Namespace: public class PlayerControl : NetworkBehaviour private void LateUpdate() { }
    MSHookFunction((void *) getAbsoluteAddress("libil2cpp.so", 0x9F49BC), (void *) GmUpdateMod,
                   (void **) &old_GmUpdate);

    // Namespace: public class GameManager : NetworkBehaviour private void UpdateScores() { }
    MSHookFunction((void *) getAbsoluteAddress("libil2cpp.so", 0x98E5AC),
                   (void *) RedscoreUpdateMod, (void **) &old_RedscoreUpdate);

    // Namespace: UnityEngine public sealed class Camera : Behaviour public float get_fieldOfView() { }
    MSHookFunction((void *) getAbsoluteAddress("libil2cpp.so", 0x20DB174), (void *) Weapon,
                   (void **) &old_Weapon);


    LOGI(OBFUSCATE("Done"));
#endif

    //Anti-leech
    /*if (!iconValid || !initValid || !settingsValid) {
        //Bad function to make it crash
        sleep(5);
        int *p = 0;
        *p = 0;
    }*/

    return NULL;
}

// Do not change or translate the first text unless you know what you are doing
// Assigning feature numbers is optional. Without it, it will automatically count for you, starting from 0
// Assigned feature numbers can be like any numbers 1,3,200,10... instead in order 0,1,2,3,4,5...
// ButtonLink, Category, RichTextView and RichWebView is not counted. They can't have feature number assigned
// Toggle, ButtonOnOff and Checkbox can be switched on by default, if you add True_. Example: CheckBox_True_The Check Box
// To learn HTML, go to this page: https://www.w3schools.com/
extern "C"
JNIEXPORT jobjectArray JNICALL
Java_com_android_support_Menu_GetFeatureList(JNIEnv *env, jobject context) {
    jobjectArray ret;

    const char *features[] = {
            OBFUSCATE("Category_ESP"),//Not counted
            OBFUSCATE("0_Toggle_Esp"),//case 0
            OBFUSCATE("1_Toggle_Line"),//case 1
            OBFUSCATE("2_RadioButton_Line Edit_Top,Center,Down"), //2 Case
            OBFUSCATE("5_Toggle_EspBox"),//case 5
            OBFUSCATE("Category_Aimbot"),//Not counted
            OBFUSCATE("3_Toggle_Aimbot"),//case 3
            OBFUSCATE("4_SeekBar_Fov_0_1000"),//case 4


            OBFUSCATE("Category_MY HACKS"),//Not counted
            OBFUSCATE("6_Toggle_Can't Fire"),//case 6
            OBFUSCATE("7_Toggle_Can't Reload"),//case 7
            OBFUSCATE("8_Toggle_No Grenade Explosion"),//case 8
            OBFUSCATE("9_Toggle_Bots Can't Spawn"),//case 9
            OBFUSCATE("10_SeekBar_Weapon Fov_1_300"),//case 10
            OBFUSCATE("11_Toggle_Infinite Ammo"),//case11
            OBFUSCATE("12_Toggle_High FireRate"),//case12
            OBFUSCATE("13_Toggle_God Mode"),//case13
            OBFUSCATE("14_Toggle_No Respawn Cooldown"),//case14
            OBFUSCATE("15_Toggle_High Red Team Score"),//case15
            OBFUSCATE("16_Toggle_High Blue Team Score"),//case16
            OBFUSCATE("17_SeekBar_Speed_1_100"), //case17

            OBFUSCATE("Category_Chams Menu"), //Not Counted
            OBFUSCATE("18_CheckBox_Default Chams"), //18 Case
            OBFUSCATE("19_CheckBox_Wireframe Chams"), //19 Case
            OBFUSCATE("20_CheckBox_Glow Chams"), //20 Case
            OBFUSCATE("21_CheckBox_Outline Chams"), //21 Case
            OBFUSCATE("22_CheckBox_Rainbow Chams"), //22 Case
            OBFUSCATE("23_SeekBar_Line Width_0_10"), //23 Case
            OBFUSCATE("24_SeekBar_Color Red_0_255"), //24 Case
            OBFUSCATE("25_SeekBar_Color Green_0_255"), //25 Case
            OBFUSCATE("26_SeekBar_Color Blue_0_255"), //26 Case
    };

    //Now you dont have to manually update the number everytime;
    int Total_Feature = (sizeof features / sizeof features[0]);
    ret = (jobjectArray)
            env->NewObjectArray(Total_Feature, env->FindClass(OBFUSCATE("java/lang/String")),
                                env->NewStringUTF(""));

    for (int i = 0; i < Total_Feature; i++)
        env->SetObjectArrayElement(ret, i, env->NewStringUTF(features[i]));

    return (ret);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_android_support_Preferences_Changes(JNIEnv *env, jclass clazz, jobject obj, jint featNum, jstring featName, jint value,
                                             jboolean boolean, jstring str) {

    LOGD(OBFUSCATE("Feature name: %d - %s | Value: = %d | Bool: = %d | Text: = %s"), featNum,
         env->GetStringUTFChars(featName, 0), value,
         boolean, str != NULL ? env->GetStringUTFChars(str, 0) : "");

    //BE CAREFUL NOT TO ACCIDENTLY REMOVE break;

    switch (featNum) {
        // ESP case code
        case 0:
            Esp = boolean;
            break;
        case 1:
            EspLine = boolean;
            break;
        case  2:
            lineMode = value;
            break;
        case 3:
            aimbot = boolean;
            break;
        case 4:
            fov = value;
            break;
        case 5:
            EspBox = boolean;
            break;


            // MY HACKS case code
        case 6:
            Fire = boolean;
            if (Fire) {
                hexPatches.Fire.Modify();
            } else {
                hexPatches.Fire.Restore();
            }
            break;
        case 7:
            Reload = boolean;
            if (Reload) {
                hexPatches.Reload.Modify();
            } else {
                hexPatches.Reload.Restore();
            }
            break;
        case 8:
            Grenade = boolean;
            if (Grenade) {
                hexPatches.Grenade.Modify();
            } else {
                hexPatches.Grenade.Restore();
            }
            break;
        case 9:
            Spawn = boolean;
            if (Spawn) {
                hexPatches.Spawn.Modify();
            } else {
                hexPatches.Spawn.Restore();
            }
            break;
        case 10:
            Weaponfov = value;
            break;
        case 11:
            Ammo = boolean;
            break;
        case 12:
            Firerate = boolean;
            break;
        case 13:
            Gm = boolean;
            break;
        case 14:
            Fastresp = boolean;
            break;
        case 15:
            Redscore = boolean;
            break;
        case 16:
            Bluescore = boolean;
            break;
        case 17:
            speedvalue = value;
            break;

            // Chams case code
        case 18:
            chams = boolean;
            if (chams) {
                SetWallhack(boolean);
            }
            break;

        case 19:
            wireframe = boolean;
            if (wireframe) {
                SetWallhackW(boolean);
            }
            break;

        case 20:
            glow = boolean;
            if (glow) {
                SetWallhackG(boolean);
            }
            break;

        case 21:
            outline = boolean;
            if (outline) {
                SetWallhackO(boolean);
            }
            break;

        case 22:
            rainbow = boolean;
            if (rainbow) {
                SetRainbow1(boolean);
            }
            break;

        case 23:
            SetW(value);
            break;

        case 24:
            SetR(value);
            break;

        case 25:
            SetG(value);
            break;

        case 26:
            SetB(value);
            break;
    }
}

__attribute__((constructor))
void lib_main() {
    // Create a new thread so it does not block the main thread, means the game would not freeze
    pthread_t ptid;
    pthread_create(&ptid, NULL, hack_thread, NULL);
}

int RegisterMenu(JNIEnv *env) {
    JNINativeMethod methods[] = {
            {OBFUSCATE("Icon"), OBFUSCATE("()Ljava/lang/String;"), reinterpret_cast<void *>(Java_com_android_support_Menu_Icon)},
            {OBFUSCATE("IconWebViewData"),  OBFUSCATE("()Ljava/lang/String;"), reinterpret_cast<void *>(Java_com_android_support_Menu_IconWebViewData)},
            {OBFUSCATE("IsGameLibLoaded"),  OBFUSCATE("()Z"), reinterpret_cast<void *>(Java_com_android_support_Menu_IsGameLibLoaded)},
            {OBFUSCATE("Init"),  OBFUSCATE("(Landroid/content/Context;Landroid/widget/TextView;Landroid/widget/TextView;)V"), reinterpret_cast<void *>(Java_com_android_support_Menu_Init)},
            {OBFUSCATE("SettingsList"),  OBFUSCATE("()[Ljava/lang/String;"), reinterpret_cast<void *>(Java_com_android_support_Menu_SettingsList)},
            {OBFUSCATE("GetFeatureList"),  OBFUSCATE("()[Ljava/lang/String;"), reinterpret_cast<void *>(Java_com_android_support_Menu_GetFeatureList)},
    };

    jclass clazz = env->FindClass(OBFUSCATE("com/android/support/Menu"));
    if (!clazz)
        return JNI_ERR;
    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) != 0)
        return JNI_ERR;
    return JNI_OK;
}

int RegisterPreferences(JNIEnv *env) {
    JNINativeMethod methods[] = {
            {OBFUSCATE("Changes"), OBFUSCATE("(Landroid/content/Context;ILjava/lang/String;IZLjava/lang/String;)V"), reinterpret_cast<void *>(Java_com_android_support_Preferences_Changes)},
    };
    jclass clazz = env->FindClass(OBFUSCATE("com/android/support/Preferences"));
    if (!clazz)
        return JNI_ERR;
    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) != 0)
        return JNI_ERR;
    return JNI_OK;
}

int RegisterMain(JNIEnv *env) {
    JNINativeMethod methods[] = {
            {OBFUSCATE("CheckOverlayPermission"), OBFUSCATE("(Landroid/content/Context;)V"), reinterpret_cast<void *>(Java_com_android_support_Main_CheckOverlayPermission)},
    };
    jclass clazz = env->FindClass(OBFUSCATE("com/android/support/Main"));
    if (!clazz)
        return JNI_ERR;
    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) != 0)
        return JNI_ERR;

    return JNI_OK;
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
    if (RegisterMenu(env) != 0)
        return JNI_ERR;
    if (RegisterPreferences(env) != 0)
        return JNI_ERR;
    if (RegisterMain(env) != 0)
        return JNI_ERR;
    return JNI_VERSION_1_6;
}
