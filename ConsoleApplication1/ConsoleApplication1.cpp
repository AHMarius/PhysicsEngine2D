#include <raylib.h>

int main() {
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

    //Declarations
    GameObject Ball;
    Ball.Transform.Position = Vector2{ 100, 0 };
    Ball.Transform.Size = Vector2{ 100,100 };
    //Raylib loop
    InitWindow(screenWidth, screenHeight, "Raylib Program");
    SetTargetFPS(60);
    
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(LIGHTGRAY); // Dark Green
        DrawRectangleV(Ball.Transform.Position, Ball.Transform.Size, GREEN);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
