#include <cmath>
#include <iostream>
#include <raylib.h>

// Constants
const bool DEBUG_MODE = 1;
const unsigned short SCREEN_WIDTH = 800;
const unsigned short SCREEN_HEIGHT = 600;
const bool SPAWN_OBJECT_STATIC_TYPE = 0;
const bool SPAWN_OBJECT_VARIABLE_TYPE = 0;
const unsigned short FPS_LIMIT = 60;
const unsigned short VECTOR_LENGHT = 60;
const float LINE_THICKNESS = 2.0f;
const float GIZMOS_SIZE = 2.0f;

// Structs
struct PhysicsObject2D {
	unsigned short Type; // 0-circle, 1-rectangle
	bool Collided;
	struct {
		Vector2 Position;
		Vector2 Size;
		Vector3 Rotation;
	} Transform;

	struct {
		float ObjectSpeed;
		Vector2 Velocity;
		Vector2 EndPoint;
		float Restitution;
		float Mass;
		float InvMass;
	} RigidBody;

	struct {
		Vector2 min;
		Vector2 max;
	} AABB;

	void toAABB() {
		AABB.min = Transform.Position;
		AABB.max = Vector2{ Transform.Size.x + Transform.Position.x, Transform.Size.y + Transform.Position.y };
	}
	void CalculateVelocity() {
		float a = RigidBody.EndPoint.x, x = Transform.Position.x;
		float b = RigidBody.EndPoint.y, y = Transform.Position.y;

		Vector2 direction = Vector2{ a - x, b - y };

		float length = sqrt(direction.x * direction.x + direction.y * direction.y);
		if (length != 0) {
			direction = Vector2{ direction.x / length, direction.y / length };
		}
		RigidBody.Velocity = Vector2{ direction.x, direction.y };
	}

	void MoveTowards()
	{
		if (RigidBody.EndPoint.x == -1 || RigidBody.EndPoint.y == -1) {
			return;
		}
		float distanceToEndPoint = sqrt(pow(RigidBody.EndPoint.x - Transform.Position.x, 2) +
			pow(RigidBody.EndPoint.y - Transform.Position.y, 2));

		if (distanceToEndPoint < 1.0f) {
			RigidBody.Velocity = Vector2{ 0, 0 };
			RigidBody.EndPoint = Vector2{ -1, -1 };
		}
		else if (!Collided) {
			CalculateVelocity();
		}

	}
};

void ObjectInteraction(PhysicsObject2D& ObjA, PhysicsObject2D& ObjB) {
	bool interaction = false;
	if (!((ObjA.AABB.max.x < ObjB.AABB.min.x || ObjA.AABB.min.x > ObjB.AABB.max.x) ||
		(ObjA.AABB.max.y < ObjB.AABB.min.y || ObjA.AABB.min.y > ObjB.AABB.max.y))) {
		interaction = true;
	}
	ObjA.Collided = ObjB.Collided = interaction;
}

void ObjectInteractionCircles(PhysicsObject2D& ObjA, PhysicsObject2D& ObjB) {
	float R = ObjA.Transform.Size.x + ObjB.Transform.Size.x;
	bool interaction = pow(R, 2) >= pow(ObjA.Transform.Position.x - ObjB.Transform.Position.x, 2) +
		pow(ObjA.Transform.Position.y - ObjB.Transform.Position.y, 2);
	ObjA.Collided = ObjB.Collided = interaction;
}


void ResolveCollision(PhysicsObject2D& ObjA, PhysicsObject2D& ObjB) {
	Vector2 relativeVelocity = Vector2{ ObjB.RigidBody.Velocity.x - ObjA.RigidBody.Velocity.x,
									   ObjB.RigidBody.Velocity.y - ObjA.RigidBody.Velocity.y };

	float x = ObjB.Transform.Position.x - ObjA.Transform.Position.x;
	float y = ObjB.Transform.Position.y - ObjA.Transform.Position.y;
	float length = std::sqrt(x * x + y * y);

	Vector2 normalVector = Vector2{ x / length, y / length };

	float velocityAlongNormal = relativeVelocity.x * normalVector.x + relativeVelocity.y * normalVector.y;

	if (velocityAlongNormal > 0) return;

	float e = std::min(ObjA.RigidBody.Restitution, ObjB.RigidBody.Restitution);

	float j = -(1 + e) * velocityAlongNormal;
	j /= ObjA.RigidBody.InvMass + ObjB.RigidBody.InvMass;

	Vector2 impulse = Vector2{ j * normalVector.x, j * normalVector.y };

	ObjA.RigidBody.Velocity = Vector2{ ObjA.RigidBody.Velocity.x - (impulse.x * ObjA.RigidBody.InvMass),
									  ObjA.RigidBody.Velocity.y - (impulse.y * ObjA.RigidBody.InvMass) };

	ObjB.RigidBody.Velocity = Vector2{ ObjB.RigidBody.Velocity.x + (impulse.x * ObjB.RigidBody.InvMass),
									  ObjB.RigidBody.Velocity.y + (impulse.y * ObjB.RigidBody.InvMass) };
}

void CreateGameObject(int& OBJECT_NUMBER, PhysicsObject2D ObjList[], Vector2 Position = Vector2{ 0, 0 },
	Vector3 Rotation = Vector3{ 0, 0, 0 }, Vector2 Size = Vector2{ 1, 1 }, unsigned short Type = 0,
	float ObjectSpeed = 1, Vector2 EndPoint = Vector2{ -1, -1 }, float Mass = 1, float Restitution = 0.5f) {
	ObjList[OBJECT_NUMBER].Transform.Position = Position;
	ObjList[OBJECT_NUMBER].Transform.Rotation = Rotation;
	if (Type == 0)
		ObjList[OBJECT_NUMBER].Transform.Size = Vector2{ Size.x, Size.x };
	else
		ObjList[OBJECT_NUMBER].Transform.Size = Size;
	ObjList[OBJECT_NUMBER].Type = Type;
	ObjList[OBJECT_NUMBER].RigidBody.ObjectSpeed = ObjectSpeed;
	ObjList[OBJECT_NUMBER].RigidBody.EndPoint = EndPoint;
	ObjList[OBJECT_NUMBER].RigidBody.Mass = Mass;
	ObjList[OBJECT_NUMBER].RigidBody.InvMass = (Mass == 0) ? Mass : 1 / Mass;
	ObjList[OBJECT_NUMBER].RigidBody.Restitution = Restitution;
	ObjList[OBJECT_NUMBER].Collided = false;

	OBJECT_NUMBER++;
}
void SetupLogic(int& OBJECT_NUMBER, PhysicsObject2D Obj[10])
{
	Vector2 StartingPoints[100], EndPoints[100], BasicPoints[100];
	int StartPointNumber = 0, EndPointNumber = 0, BasicPointNumber = 0;
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib Program");
	SetTargetFPS(FPS_LIMIT);
	while (IsKeyUp(KEY_SPACE))
	{
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
		{
			StartingPoints[StartPointNumber] = GetMousePosition();
			StartPointNumber++;
		}
		else if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
		{
			EndPoints[EndPointNumber] = GetMousePosition();
			EndPointNumber++;
		}
		else if (IsKeyPressed(KEY_D))
		{
			BasicPoints[BasicPointNumber] = GetMousePosition();
			BasicPointNumber++;
		}

		BeginDrawing();
		ClearBackground(BLACK);
		for (int i = 0; i < StartPointNumber; i++)
		{
			DrawCircleV(StartingPoints[i], GIZMOS_SIZE, RED);
		}
		for (int i = 0; i < EndPointNumber; i++)
		{
			DrawCircleV(EndPoints[i], GIZMOS_SIZE, YELLOW);
		}
		for (int i = 0; i < BasicPointNumber; i++)
		{
			DrawCircleV(BasicPoints[i], GIZMOS_SIZE, DARKPURPLE);
		}
		for (int i = 0; i < std::min(EndPointNumber, StartPointNumber); i++)
		{
			DrawLineV(StartingPoints[i], EndPoints[i], YELLOW);
		}
		EndDrawing();
	}
	EndDrawing();
	for (int i = 0; i < EndPointNumber; i++)
	{
		CreateGameObject(OBJECT_NUMBER, Obj, StartingPoints[i], Vector3{ 0, 0, 0 }, Vector2{ 25, 50 }, SPAWN_OBJECT_VARIABLE_TYPE, 3, EndPoints[i], 1, 0.5f);
	}
	for (int i = EndPointNumber; i < StartPointNumber; i++)
	{
		CreateGameObject(OBJECT_NUMBER, Obj, StartingPoints[i], Vector3{ 0, 0, 0 }, Vector2{ 25, 50 }, SPAWN_OBJECT_VARIABLE_TYPE, 3, StartingPoints[i], 1, 0.5f);
	}
	for (int i = 0; i < BasicPointNumber; i++)
	{
		CreateGameObject(OBJECT_NUMBER, Obj, BasicPoints[i], Vector3{ 0, 0, 0 }, Vector2{ 25, 25 }, SPAWN_OBJECT_STATIC_TYPE, 3, BasicPoints[i], 1, 0.5f);
	}
	CloseWindow();
}

void CollisionLogic(PhysicsObject2D Obj[], int OBJECT_NUMBER)
{
	for (int counter = 0; counter < OBJECT_NUMBER; counter++) {
		Obj[counter].Collided = false;
		if (Obj[counter].Type != 0) {
			Obj[counter].toAABB();
		}
	}

	// Intersection Check
	for (int i = 0; i < OBJECT_NUMBER; i++) {
		for (int j = i + 1; j < OBJECT_NUMBER; j++) {
			if (Obj[i].Type == 0 && Obj[j].Type == 0) {
				ObjectInteractionCircles(Obj[i], Obj[j]);
			}
			else {
				ObjectInteraction(Obj[i], Obj[j]);
			}
		}
	}


	//Velocity Calculations loop
	for (int i = 0; i < OBJECT_NUMBER; i++) {
		// Check for collisions and resolve them
		for (int j = 0; j < OBJECT_NUMBER; j++) {
			if (i != j) {
				// Handle collisions between different object types
				if (Obj[i].Type == 0 && Obj[j].Type == 0) {
					// Both are circles
					ObjectInteractionCircles(Obj[i], Obj[j]);
				}
				else {
					// At least one is a rectangle
					ObjectInteraction(Obj[i], Obj[j]);
				}

				// Resolve collisions if there is any
				if (Obj[i].Collided && Obj[j].Collided) {
					Obj[i].RigidBody.EndPoint = Vector2{ -1, -1 };
					Obj[j].RigidBody.EndPoint = Vector2{ -1, -1 };
					ResolveCollision(Obj[i], Obj[j]);
				}
			}
		}
		Obj[i].MoveTowards();
	}
}
void PositionUpdate(PhysicsObject2D Obj[], int OBJECT_NUMBER) {

	// Position Change Loop
	for (int counter = 0; counter < OBJECT_NUMBER; counter++) {
		Obj[counter].Transform.Position = Vector2{
			Obj[counter].Transform.Position.x + Obj[counter].RigidBody.Velocity.x * Obj[counter].RigidBody.ObjectSpeed,
			Obj[counter].Transform.Position.y + Obj[counter].RigidBody.Velocity.y * Obj[counter].RigidBody.ObjectSpeed
		};
	}
}
void DrawScreenLogic(PhysicsObject2D Obj[], int OBJECT_NUMBER)
{
	// Draw
	BeginDrawing();
	ClearBackground(BLACK);
	// Draw Objects
	for (int counter = 0; counter < OBJECT_NUMBER; counter++) {
		if (Obj[counter].Type == 0) {
			DrawCircleV(Obj[counter].Transform.Position, Obj[counter].Transform.Size.x, (!Obj[counter].Collided) ? DARKGREEN : RED);
		}
		else {
			DrawRectangleV(Obj[counter].Transform.Position, Obj[counter].Transform.Size, (!Obj[counter].Collided) ? DARKGREEN : RED);
		}

		// Draw velocity vector
		Vector2 velocityEndPoint = Vector2{
			Obj[counter].Transform.Position.x + Obj[counter].RigidBody.Velocity.x * VECTOR_LENGHT,
			Obj[counter].Transform.Position.y + Obj[counter].RigidBody.Velocity.y * VECTOR_LENGHT
		};

		DrawLineEx(Obj[counter].Transform.Position, velocityEndPoint, LINE_THICKNESS, BLUE);
	}

	// Draw Hitboxes
	for (int counter = 0; counter < OBJECT_NUMBER && DEBUG_MODE; counter++) {
		PhysicsObject2D varObj = Obj[counter];
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
int main() {
	// Declarations
	int OBJECT_NUMBER = 0;
	PhysicsObject2D Obj[100];


	SetupLogic(OBJECT_NUMBER, Obj);
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib Program");
	SetTargetFPS(FPS_LIMIT);
	// Raylib Loop
	while (!WindowShouldClose()) {
		CollisionLogic(Obj, OBJECT_NUMBER);
		PositionUpdate(Obj, OBJECT_NUMBER);
		DrawScreenLogic(Obj, OBJECT_NUMBER);
	}
	CloseWindow();
	return 0;
}