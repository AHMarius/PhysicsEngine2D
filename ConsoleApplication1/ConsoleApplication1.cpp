#include <raylib.h>
#include <iostream>
#include <cmath>
//Constants
const bool DEBUG_MODE = true;
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int OBJECT_NUMBER = 2;
const int FPS_LIMIT = 60;
const int VECTOR_LENGHT = 60;
const float LINE_THICKNESS = 2.0f;
const float GIZMOS_SIZE = 2.0f;
//Objects
struct GameObject {
    unsigned short Type; //0-circle, 1 rectangle
    struct {
        Vector2 Position;
        Vector2 Size;
        Vector3 Rotation;
    }Transform;
    //Temporary
    struct {
        float ObjectSpeed;
        Vector2 Velocity;
        Vector2 EndPoint;
    }RigidBody;
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
bool ObjectIntersection(GameObject ObjA, GameObject ObjB)
{
    if (ObjA.AABB.max.x < ObjB.AABB.min.x || ObjA.AABB.min.x > ObjB.AABB.max.x) return false;
    if (ObjA.AABB.max.y < ObjB.AABB.min.y || ObjA.AABB.min.y > ObjB.AABB.max.y) return false;
    return true;
}
bool ObjectInteractionCircles(GameObject ObjA, GameObject ObjB)
{
    float R = ObjA.Transform.Size.x + ObjB.Transform.Size.x;
    return pow(R,2) >= pow(ObjA.Transform.Position.x - ObjB.Transform.Position.x, 2) + pow(ObjA.Transform.Position.y - ObjB.Transform.Position.y, 2);
}
void CalculateVelocity(GameObject &ObjA)
{
    float a = ObjA.RigidBody.EndPoint.x, x = ObjA.Transform.Position.x;
    float b = ObjA.RigidBody.EndPoint.y, y = ObjA.Transform.Position.y;
    // Vector Normalization
    /*float vectorLenght = pow((a - x), 2) + pow((b - x), 2);
    ObjA.RigidBody.Velocity = Vector2{ (float)pow((a-x),2)/vectorLenght
        ,(float)pow((b-y),2)/vectorLenght };
    */
    ObjA.RigidBody.Velocity = Vector2{ (float)((a - x>0)?1:-1),(float)((b - y > 0) ? 1 : -1) };

}
int main() {
    //Declarations
        //Variables
    bool InteractionCheck = false;
        //Objects
    GameObject Obj[10];
        //First Object
    Obj[0].Transform.Position = Vector2{50, 140};
    Obj[0].Transform.Size = Vector2{50,50};
    Obj[0].Type = 1;
    Obj[0].RigidBody.ObjectSpeed = 1;
    Obj[0].RigidBody.EndPoint = Vector2{550,640 };
        //First Object
    Obj[1].Transform.Position = Vector2{ 200, 50 };
    Obj[1].Transform.Size = Vector2{ 50,50 };
    Obj[1].Type = 1;
    Obj[1].RigidBody.EndPoint = Vector2{ 200,550 };
    Obj[1].RigidBody.ObjectSpeed = 2;
    //Debugging
    for (int counter = 0; counter < OBJECT_NUMBER && DEBUG_MODE; counter++)
    {
        DebugAABB(Obj[counter]);
    }
    //Raylib Loop
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib Program");
    SetTargetFPS(FPS_LIMIT);
    while (!WindowShouldClose()) {
        //Update
              
        //AABB && Velocity Calculations loop
        for (int counter = 0; counter < OBJECT_NUMBER; counter++)
        {
            if (Obj[counter].Type != 0)
                Obj[counter].toAABB();
            if ((Obj[counter].RigidBody.EndPoint.x != -1 && Obj[counter].RigidBody.EndPoint.y != -1)) {
                if (!InteractionCheck &&
                    (Obj[counter].RigidBody.EndPoint.x != Obj[counter].Transform.Position.x || Obj[counter].RigidBody.EndPoint.y != Obj[counter].Transform.Position.y)
                   )
                {
                    CalculateVelocity(Obj[counter]);
                }
                else {
                    Obj[counter].RigidBody.Velocity = Vector2{ 0,0 };
                    Obj[counter].RigidBody.EndPoint = Vector2{ -1,-1 };
                }
            }
        }
        //Position Change Loop
        for (int counter = 0; counter < OBJECT_NUMBER; counter++)
        {
            Obj[counter].Transform.Position = Vector2{
                Obj[counter].Transform.Position.x + Obj[counter].RigidBody.Velocity.x * Obj[counter].RigidBody.ObjectSpeed,
                Obj[counter].Transform.Position.y + Obj[counter].RigidBody.Velocity.y * Obj[counter].RigidBody.ObjectSpeed
            };
        }
        //Intersection Check
       
        if (Obj[0].Type == 0)
        {
            InteractionCheck = ObjectInteractionCircles(Obj[0], Obj[1]);
        }
        else {
            InteractionCheck = ObjectIntersection(Obj[0], Obj[1]);
        }
        BeginDrawing();
        ClearBackground(DARKGRAY); 
        //Draw Objects
        for (int counter = 0; counter < OBJECT_NUMBER; counter++)
        {
            if (Obj[counter].Type == 0)
            {
                DrawCircleV(Obj[counter].Transform.Position, Obj[counter].Transform.Size.x, (!InteractionCheck) ? GREEN : RED);
            }
            else {
                DrawRectangleV(Obj[counter].Transform.Position, Obj[counter].Transform.Size, (!InteractionCheck) ? GREEN : RED);
            }
        }
        //Draw Hitboxes
        for (int counter = 0; counter < OBJECT_NUMBER && DEBUG_MODE; counter++)
        {
            GameObject varObj = Obj[counter];
            if (varObj.Type == 0) {
                DrawCircleLinesV(varObj.Transform.Position, varObj.Transform.Size.x, RED);
                DrawLine(varObj.Transform.Position.x - varObj.Transform.Size.x, varObj.Transform.Position.y, varObj.Transform.Position.x + varObj.Transform.Size.x, varObj.Transform.Position.y, RED);
            }
            else {
                DrawLineEx(varObj.AABB.min, varObj.AABB.max, LINE_THICKNESS, RED);
                DrawRectangleLines(varObj.Transform.Position.x, varObj.Transform.Position.y, varObj.Transform.Size.x, varObj.Transform.Size.y, RED);
            }
            if (varObj.RigidBody.EndPoint.x!=-1) {
                DrawCircleV(varObj.RigidBody.EndPoint, GIZMOS_SIZE, YELLOW);
                DrawLineV(varObj.Transform.Position, varObj.RigidBody.EndPoint, YELLOW);
            }
        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
