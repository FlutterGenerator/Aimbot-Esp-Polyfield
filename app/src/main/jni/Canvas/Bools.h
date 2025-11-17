
#include <jni.h>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <sys/mman.h>
#include <android/log.h>

int screenH = 0;
int screenW = 0;

bool Esp = false;
bool EspLine = false;
bool EspBox = false;


bool aimbot;
float fov = 1.0f;

struct Angles {
   float yaw, pitch;
};

bool *(*isAlive)(void *);


Angles CalculateViewAngle(const Vector3& myPlayer, const Vector3& target) {
  
    Angles viewangl;
    //Here is the calculation of y 
    float centertotop = -60.0f; //change it according ro your real y (center to top) value;
    float centertobottom = 60.0f; //change it according to your real y (center to bottom) value;

    float directionX = target.x - myPlayer.x;
    float directionY = target.y - myPlayer.y;
    float directionZ = target.z - myPlayer.z;
    // Calculate the angle in radians
    float angleRadiansy = atan2f(directionY, sqrtf(directionX * directionX + directionZ * directionZ));
    // Convert the angle from radians to degrees
    float angleDegreesy = angleRadiansy * (180.0f / M_PI);
    // Adjust the angle to fit our desired range (-60 to 60) for polyfield its -90 to 90 max but -60 to 60 is seem good
    float yAngle = -angleDegreesy; // invert the angle since you want 0 at center, -60 at top, and 60 at bottom
    // Clamp the angle to the desired range
    yAngle = fmaxf(centertotop, fminf(centertobottom, yAngle));
    viewangl.pitch = yAngle;
    //Here is the calculation of x 
    // Calculate the angle in radians
    float angleRadiansx = atan2f(directionX, directionZ);
    // Convert the angle from radians to degrees
    float angleDegreesx = angleRadiansx * (180.0f / M_PI);
    // Adjust the angle to fit our desired range (0 to 360)
    float xAngle = fmodf((angleDegreesx + 360.0f), 360.0f); //its 360 like 0 - 360 center to right umm having whole rotation            
    viewangl.yaw = xAngle;
    
    return viewangl;
}



          


