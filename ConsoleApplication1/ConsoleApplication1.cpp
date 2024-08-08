#include <raylib.h>
#include <iostream>
//Constants
const bool DEBUG_MODE = true;
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int OBJECT_NUMBER = 2;
const float LINE_THICKNESS = 5.0f;

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
        AABB.max = Vector2{ Transform.Size.x + Transform.Position.x, Transform.Size.y + Transform.Position.y };
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
    GameObject Obj[10];
        //First Object
    Obj[0].Transform.Position = Vector2{100, 0};
    Obj[0].Transform.Size = Vector2{100,100};
        //First Object
    Obj[1].Transform.Position = Vector2{ 200, 50 };
    Obj[1].Transform.Size = Vector2{ 100,100 };

    //AABB Calculations
    for(int counter = 0; counter < OBJECT_NUMBER; counter++)
    {
        Obj[counter].toAABB();
    }
    //Debugging
    for (int counter = 0; counter < OBJECT_NUMBER && DEBUG_MODE; counter++)
    {
        DebugAABB(Obj[counter]);
    }
    //Raylib Loop
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib Program");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(LIGHTGRAY); 
        //Draw Objects
        for (int counter = 0; counter < OBJECT_NUMBER; counter++)
        {
            DrawRectangleV(Obj[counter].Transform.Position, Obj[counter].Transform.Size, GREEN);

        }
        //Draw Hitboxes
        for (int counter = 0; counter < OBJECT_NUMBER; counter++)
        {
            DrawLineEx(Obj[counter].AABB.min, Obj[counter].AABB.max, LINE_THICKNESS, RED);
        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
