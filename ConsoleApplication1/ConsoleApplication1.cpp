#include <cmath>
#include <iostream>
#include <raylib.h>
//Constants
const bool DEBUG_MODE = true;
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

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
		float Restitution;
		float Mass;
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
	return pow(R, 2) >= pow(ObjA.Transform.Position.x - ObjB.Transform.Position.x, 2) + pow(ObjA.Transform.Position.y - ObjB.Transform.Position.y, 2);
}
void CalculateVelocity(GameObject& ObjA) {
	float a = ObjA.RigidBody.EndPoint.x, x = ObjA.Transform.Position.x;
	float b = ObjA.RigidBody.EndPoint.y, y = ObjA.Transform.Position.y;

	// Calculate the direction vector (EndPoint - Position)
	Vector2 direction = Vector2{ a - x, b - y };

	// Normalize the direction vector
	float length = sqrt(direction.x * direction.x + direction.y * direction.y);
	if (length != 0) {
		direction = Vector2{ direction.x / length, direction.y / length };
	}
	// Set the velocity based on the normalized direction
	ObjA.RigidBody.Velocity = Vector2{ direction.x, direction.y };
}

void ResolveCollision(GameObject& ObjA, GameObject& ObjB)
{
	Vector2 relativeVelocity = Vector2{ ObjB.RigidBody.Velocity.x - ObjA.RigidBody.Velocity.x, ObjB.RigidBody.Velocity.y - ObjA.RigidBody.Velocity.y };

	float x = ObjB.Transform.Position.x - ObjA.Transform.Position.x;
	float y = ObjB.Transform.Position.y - ObjA.Transform.Position.y;
	float length = std::sqrt(x * x + y * y);

	// Normalize the collision normal
	Vector2 normalVector = Vector2{ x / length, y / length };

	// Calculate velocity along the normal
	float velocityAlongNormal = relativeVelocity.x * normalVector.x + relativeVelocity.y * normalVector.y;

	// If velocity is separating, no need to resolve
	if (velocityAlongNormal > 0)
		return;

	// Restitution (elasticity)
	float e = std::min(ObjA.RigidBody.Restitution, ObjB.RigidBody.Restitution);

	// Calculate impulse scalar
	float j = -(1 + e) * velocityAlongNormal;
	j /= (1 / ObjA.RigidBody.Mass) + (1 / ObjB.RigidBody.Mass);

	// Apply impulse to both objects in the opposite direction
	Vector2 impulse = Vector2{ j * normalVector.x, j * normalVector.y };

	ObjA.RigidBody.Velocity = Vector2{ ObjA.RigidBody.Velocity.x - (impulse.x / ObjA.RigidBody.Mass),
									   ObjA.RigidBody.Velocity.y - (impulse.y / ObjA.RigidBody.Mass) };

	ObjB.RigidBody.Velocity = Vector2{ ObjB.RigidBody.Velocity.x + (impulse.x / ObjB.RigidBody.Mass),
									   ObjB.RigidBody.Velocity.y + (impulse.y / ObjB.RigidBody.Mass) };
}

void CreateGameObject(int& OBJECT_NUMBER, GameObject ObjList[], Vector2 Position = Vector2{ 0,0 }, Vector3 Rotation = Vector3{ 0,0,0 }, Vector2 Size = Vector2{ 1,1 }, unsigned short Type = 0, float ObjectSpeed = 1, Vector2 EndPoint = Vector2{ -1,-1 }/*repeat the position otherwise it won't appear*/, float Mass = 1, float Restitution = 0.5f)
{
	ObjList[OBJECT_NUMBER].Transform.Position = Position;
	ObjList[OBJECT_NUMBER].Transform.Rotation = Rotation;
	if (Type == 0)
		ObjList[OBJECT_NUMBER].Transform.Size = Vector2{ Size.x,Size.x };
	else
		ObjList[OBJECT_NUMBER].Transform.Size = Size;
	ObjList[OBJECT_NUMBER].Type = Type;
	ObjList[OBJECT_NUMBER].RigidBody.ObjectSpeed = ObjectSpeed;
	ObjList[OBJECT_NUMBER].RigidBody.EndPoint = EndPoint;
	ObjList[OBJECT_NUMBER].RigidBody.Mass = Mass;
	ObjList[OBJECT_NUMBER].RigidBody.Restitution = Restitution;
	OBJECT_NUMBER++;
}
int main() {
	//Declarations
		//Variables
	bool InteractionCheck = false;
	int OBJECT_NUMBER = 0;
	//Objects
	GameObject Obj[10];
	CreateGameObject(OBJECT_NUMBER, Obj, Vector2{ 50, 50 }, Vector3{ 0,0,0 }, Vector2{ 25,25 }, 0, 2, Vector2{ 500, 500 }, 1, 0.5f);
	CreateGameObject(OBJECT_NUMBER, Obj, Vector2{ 100,180 }, Vector3{ 0,0,0 }, Vector2{ 25,25 }, 0, 2, Vector2{ 400,400 }, 1, 0.5f);
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
			if (InteractionCheck)
			{
				ResolveCollision(Obj[0], Obj[1]);
				Obj[counter].RigidBody.EndPoint = Vector2{ -1, -1 };
			}
			else if (Obj[counter].RigidBody.EndPoint.x != -1 && Obj[counter].RigidBody.EndPoint.y != -1) {
				float distanceToEndPoint = sqrt(pow(Obj[counter].RigidBody.EndPoint.x - Obj[counter].Transform.Position.x, 2) +
					pow(Obj[counter].RigidBody.EndPoint.y - Obj[counter].Transform.Position.y, 2));

				if (distanceToEndPoint < 1.0f) {
					Obj[counter].RigidBody.Velocity = Vector2{ 0, 0 };
					Obj[counter].RigidBody.EndPoint = Vector2{ -1, -1 };
				}
				else if (!InteractionCheck) {
					CalculateVelocity(Obj[counter]);
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
			if (varObj.RigidBody.EndPoint.x != -1) {
				DrawCircleV(varObj.RigidBody.EndPoint, GIZMOS_SIZE, YELLOW);
				DrawLineV(varObj.Transform.Position, varObj.RigidBody.EndPoint, YELLOW);
			}
		}
		EndDrawing();
	}

	CloseWindow();
	return 0;
}
