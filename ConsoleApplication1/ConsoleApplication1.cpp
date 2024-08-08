#include <raylib.h>
#include <iostream>
//Constants
const int screenWidth = 800;
const int screenHeight = 600;
//Objects
struct GameObject {
    struct {
        Vector2 Position;
        Vector2 Size;
        Vector3 Rotation;
    }Transform;
    //Axis Aligned Bounding Boxes
    struct {
        Vector2 min;
        Vector2 max;
    }AABB;
    void toAABB()
    {
        AABB.min = Transform.Position;
        AABB.max = Vector2{ Transform.Size.x - Transform.Position.x, Transform.Size.y - Transform.Position.y };
    }
};
//Functions
    //Debug
void DebugAABB(GameObject Obj)
{
    std::cout << Obj.AABB.min.x << " " << Obj.AABB.min.y << " " << Obj.AABB.max.x << " " << Obj.AABB.max.y;
}
    //Returnable
int main() {
    //Declarations
    GameObject Obj;
    Obj.Transform.Position = Vector2{ 100, 0 };
    Obj.Transform.Size = Vector2{ 100,100 };
    //Raylib loop
    InitWindow(screenWidth, screenHeight, "Raylib Program");
    SetTargetFPS(60);
    
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(LIGHTGRAY); // Dark Green
        DrawRectangleV(Obj.Transform.Position, Obj.Transform.Size, GREEN);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
