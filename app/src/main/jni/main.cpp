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

#include "Includes/Logger.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"

#include "Menu/Register.h"
#include "hooks.h"

#include <Substrate/SubstrateHook.h>
#include <Substrate/CydiaSubstrate.h>

ESP espOverlay;

void DrawESP(ESP esp, int screenWidth, int screenHeight) {
   
    screenW = screenWidth;
    screenH = screenHeight;
   
    esp.DrawFilledCircle(Color(255,255,255,255), Vector2(screenWidth / 2, screenHeight / 9), 48.0f);
    std::string count;
    count += std::to_string(players.size());
    esp.DrawText(Color(0,0,0,255), count.c_str(), Vector2(screenWidth / 2, screenHeight / 8), 65.0f); 

    if (aimbot) {
        esp.DrawCircle(Color(255, 255, 255, 255), 1.0f, Vector2(screenWidth / 2, screenHeight / 2), fov);    
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
                         esp.DrawLine(Color(255,255,255,255), 1.0f, From, To);
                     }

if (EspBox) {
    float boxHeight = 200.0f;   // высота бокса (настрой себе)
    float boxWidth  = 80.0f;    // ширина бокса

    Rect rect;
    rect.x = Pos2d.x - boxWidth / 2;
    rect.y = screenHeight - Pos2d.y - boxHeight;
    rect.width = boxWidth;
    rect.height = boxHeight;

    esp.DrawBox(Color(255,255,255,255), 2.0f, rect);
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
    do {
        sleep(1);
    } while (!isLibraryLoaded(libName));
       
    // public sealed class Camera
    // public static Camera get_main()  
    get_current = (void *(*)())getAbsoluteAddress("libil2cpp.so", 0x2072AB0);
    
    // public class Component
    // public Transform get_transform()
    get_transform = (void *(*)(void *))getAbsoluteAddress("libil2cpp.so", 0x209D61C);
    
    // public class Transform
    // public Vector3 get_position() { }
    get_position = (Vector3 (*)(void *))getAbsoluteAddress("libil2cpp.so", 0x20AB558);
    
    // public sealed class Camera
    // public Vector3 WorldToScreenPoint(Vector3 position) { }
    worldtoscreen = (Vector3 (*)(void *, Vector3))getAbsoluteAddress("libil2cpp.so", 0x20727BC);
    
    // public class Object
    // private static bool IsNativeObjectAlive
    isAlive = (bool *(*)(void *))getAbsoluteAddress("libil2cpp.so", 0x20A4B6C);
    
    // public class NpcControl
    // private void Update() { }
    MSHookFunction((void *)getAbsoluteAddress("libil2cpp.so", 0x9B4D88), (void *)&_update, (void **)&old_update); // Accoring to your game . Hope You Find this offset 😁
    
    // public class PlayerControl
    // private void Update() { }
    MSHookFunction((void *)getAbsoluteAddress("libil2cpp.so", 0x9D2120), (void *)&_myPlayer, (void **)&old_myPlayer); // Accoring to your game . Hope You Find this offset 😁

    return NULL;
}

jobjectArray GetFeatureList(JNIEnv *env, jobject context) {
    jobjectArray ret;

    const char *features[] = {
            OBFUSCATE("Category_ESP"),//Not counted
            OBFUSCATE("0_Toggle_Esp"),
            OBFUSCATE("1_Toggle_Line"),
            OBFUSCATE("Category_Aimbot"),
            OBFUSCATE("2_Toggle_Aimbot"),
            OBFUSCATE("3_SeekBar_Fov_0_1000"),
            OBFUSCATE("4_Toggle_EspBox"),
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
       
        case 0: Esp = boolean; break;
        case 1: EspLine = boolean; break;
        case 2: aimbot = boolean; break;
        case 3: fov = value; break;
        case 4: EspBox = boolean; break;
       
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
            {OBFUSCATE("Icon"), OBFUSCATE("()Ljava/lang/String;"), reinterpret_cast<void *>(Icon)},
            {OBFUSCATE("IconWebViewData"),  OBFUSCATE("()Ljava/lang/String;"), reinterpret_cast<void *>(IconWebViewData)},
            {OBFUSCATE("IsGameLibLoaded"),  OBFUSCATE("()Z"), reinterpret_cast<void *>(isGameLibLoaded)},
            {OBFUSCATE("Init"),  OBFUSCATE("(Landroid/content/Context;Landroid/widget/TextView;Landroid/widget/TextView;)V"), reinterpret_cast<void *>(Init)},
            {OBFUSCATE("GetFeatureList"),  OBFUSCATE("()[Ljava/lang/String;"), reinterpret_cast<void *>(GetFeatureList)},
    };

    if (Register(env, "com/android/support/Menu", menuMethods, sizeof(menuMethods) / sizeof(JNINativeMethod)) != 0)
        return JNI_ERR;

    static const JNINativeMethod prefMethods[] = {
            { OBFUSCATE("Changes"), OBFUSCATE("(Landroid/content/Context;ILjava/lang/String;IZLjava/lang/String;)V"), reinterpret_cast<void *>(Changes)},
    };

    if (Register(env, "com/android/support/Preferences",
                 prefMethods, sizeof(prefMethods) / sizeof(JNINativeMethod)) != 0)
        return JNI_ERR;

    static const JNINativeMethod mainMethods[] = {
            { OBFUSCATE("CheckOverlayPermission"), OBFUSCATE("(Landroid/content/Context;)V"), reinterpret_cast<void *>(CheckOverlayPermission)},
    };

    if (Register(env, "com/android/support/Main", mainMethods, sizeof(mainMethods) / sizeof(JNINativeMethod)) != 0)
        return JNI_ERR;

    return JNI_VERSION_1_6;
}
