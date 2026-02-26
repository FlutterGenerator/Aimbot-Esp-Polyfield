#include "Canvas/ESP.h"
#include "Canvas/Bools.h"
#include "Canvas/StructsCommon.h"

#include "PlayerSearcher.h"


void *(*get_current)();

void *(*get_transform)(void *obj);

Vector3 (*get_position)(void *obj);

Vector3 (*worldtoscreen)(void *cam, Vector3 obj);


bool isInsideFov(int x, int y) {
    int circle_x = screenW / 2;
    int circle_y = screenH / 2;
    int rad = fov;
    return (x - circle_x) * (x - circle_x) + (y - circle_y) * (y - circle_y) <= rad * rad;
}


void (*old_update)(void *p);

void _update(void *p) {
    if (p != nullptr) {
        if (Esp) {
            if (!playerFind(p)) players.push_back(p);
            if (players.size() > 50) {
                players.clear();
            }
        }
        clearPlayers();
    }
    return old_update(p);
}


void (*old_myPlayer)(void *player);

void _myPlayer(void *player) {
    if (player != NULL) {

        if (aimbot) {
            for (int i = 0; i < players.size(); i++) {
                auto Player = players[i];
                if (Player != nullptr) {
                    if (isAlive(Player)) {
                        Vector3 Me = get_position(get_transform(get_current()));
                        Vector3 Pos3d = get_position(get_transform(Player));
                        Vector3 Target = {Pos3d.x, Pos3d.y + 1.3, Pos3d.z};
                        Vector3 posxy = worldtoscreen(get_current(), Target);
                        if (posxy.z < 1.0f) continue;
                        if (isInsideFov((int) posxy.x, (int) posxy.y)) {
                            Angles angles = CalculateViewAngle(Me, Target);
                            float xx = angles.pitch;
                            float yy = angles.yaw;
                            // Find this Filed Name "_lookDir" In your dump.cs
                            //public Vector2 _lookDir; // 0xA8
                            *(Vector2 *) ((uintptr_t) player + 0xA8) = Vector2(xx, yy);
                        }
                    } else {
                        players.clear();
                        clearPlayers();
                    }
                }
            }
        }
    }
    old_myPlayer(player);
}
