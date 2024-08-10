#include<algorithm>
#include <cmath>
#include <iostream>
#include <raylib.h>

// Constants
const bool DEBUG_MODE = 0;
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

const int FPS_LIMIT = 60;
const int VECTOR_LENGHT = 60;
const float POSITIONAL_CORRECTION_PERCENT = 0.2f;
const float POSITIONAL_CORRECTION_SLOP = 0.01f;
const float LINE_THICKNESS = 2.0f;
const float GIZMOS_SIZE = 2.0f;
const float VELOCITY_THRESHOLD = 0.01f;


// Structs
struct AABBS {
	Vector2 min;
	Vector2 max;
};
struct PhysicsObject {
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
		float InverseMass;
	} RigidBody;

	AABBS AABB;

	void toAABB() {
		AABB.min = Transform.Position;
		AABB.max = Vector2{ Transform.Size.x + Transform.Position.x, Transform.Size.y + Transform.Position.y };
	}
};
struct Manifold {
	PhysicsObject* A;
	PhysicsObject* B;
	float penetration;
	Vector2 normal;
};

// Functions
//Usability
double clamp(double x, double upper, double lower)
{
	return std::min(upper, std::max(x, lower));
}
Vector2 normalize(const Vector2& v)
{
	float length = std::sqrt(v.x * v.x + v.y * v.y);
	if (length != 0) {
		return Vector2{ v.x / length, v.y / length };
	}
	return Vector2{ 0, 0 }; // If the length is zero, return a zero vector to avoid division by zero.
}
// Debugging function
void DebugAABB(PhysicsObject Obj) {
	std::cout << Obj.AABB.min.x << " " << Obj.AABB.min.y << " " << Obj.AABB.max.x << " " << Obj.AABB.max.y;
}

void ObjectInteraction(Manifold* M) {

	PhysicsObject* A = M->A;
	PhysicsObject* B = M->B;
	Vector2 n = Vector2{ B->Transform.Position.x - A->Transform.Position.x, B->Transform.Position.y - A->Transform.Position.y };
	AABBS abox = A->AABB;
	AABBS bbox = B->AABB;
	float a_extent = (abox.max.x - abox.min.x) / 2;
	float b_extent = (bbox.max.x - bbox.min.x) / 2;
	float x_overlap = a_extent + b_extent - std::abs(n.x);
	if (x_overlap > 0)
	{
		float a_extent = (abox.max.y - abox.min.y) / 2;
		float b_extent = (bbox.max.y - bbox.min.y) / 2;
		float y_overlap = a_extent + b_extent - std::abs(n.y);
		if (y_overlap > 0)
		{
			if (x_overlap > y_overlap)
			{
				if (n.x < 0)
				{
					M->normal = Vector2{ -1,0 };
				}
				else {
					M->normal = Vector2{ 0,0 };
				}
				M->penetration = x_overlap;
				A->Collided = B->Collided = true;
			}
			else {
				if (n.y < 0)
				{
					M->normal = Vector2{ 0,-1 };
				}
				else {
					M->normal = Vector2{ 0,1 };
				}
				M->penetration = y_overlap;
				A->Collided = B->Collided = true;
			}
		}
	}
}

void ObjectInteractionCircles(Manifold* M) {
	// Setup a couple of pointers to each object 
	PhysicsObject* A = M->A;
	PhysicsObject* B = M->B;

	// Vector from A to B 
	Vector2 n = Vector2{ B->Transform.Position.x - A->Transform.Position.x, B->Transform.Position.y - A->Transform.Position.y };

	// Sum of both radii squared
	float r = A->Transform.Size.x + B->Transform.Size.x; // Assuming Size.x is the radius for circles
	r *= r;

	// Check if the distance squared between the centers is greater than the squared radius sum
	float lenSq = n.x * n.x + n.y * n.y;
	if (lenSq > r) {
		A->Collided = B->Collided = false;
		return;
	}

	// Circles have collided, now compute manifold 
	float d = std::sqrt(lenSq);  // perform actual sqrt to get distance

	// If distance between circles is not zero 
	if (d != 0) {
		// Distance is the difference between the combined radii and the distance between the circles' centers
		M->penetration = (A->Transform.Size.x + B->Transform.Size.x) - d;

		// Normalize the vector n and set it as the collision normal
		M->normal = Vector2{ n.x / d, n.y / d };
	}
	// Circles are on the same position
	else {
		// Choose a random (but consistent) direction for the normal
		M->penetration = A->Transform.Size.x;  // penetration is the radius of A (could also use B)
		M->normal = Vector2{ 1, 0 };  // Arbitrary direction
	}

	A->Collided = B->Collided = true;
}
void ObjectInteractionMixed(Manifold* M)
{
	PhysicsObject* A = M->A;
	PhysicsObject* B = M->B;
	Vector2 n = Vector2{ B->Transform.Position.x - A->Transform.Position.x, B->Transform.Position.y - A->Transform.Position.y };
	Vector2 closestPoint = n;

	float x_extent = (A->AABB.max.x - A->AABB.min.x) / 2;
	float y_extent = (A->AABB.max.y - A->AABB.min.y) / 2;

	closestPoint.x = clamp(-x_extent, x_extent, closestPoint.x);
	closestPoint.y = clamp(-y_extent, y_extent, closestPoint.y);

	bool inside = (n.x == closestPoint.x && n.y == closestPoint.y);

	if (inside)
	{
		if (std::abs(n.x) > std::abs(n.y))
		{
			closestPoint.x = (closestPoint.x > 0) ? x_extent : -x_extent;
		}
		else {
			closestPoint.y = (closestPoint.y > 0) ? y_extent : -y_extent;
		}
	}

	Vector2 normal = Vector2{ n.x - closestPoint.x, n.y - closestPoint.y };
	float d = normal.x * normal.x + normal.y * normal.y;
	float r = B->Transform.Size.x;

	if (d > r * r && !inside) {
		A->Collided = B->Collided = false;
		return;
	}

	d = std::sqrt(d);
	normal = normalize(normal);

	if (inside)
	{
		M->normal = normalize(Vector2{ -n.x, -n.y });
		M->penetration = r - d;
	}
	else {
		M->normal = normal;
		M->penetration = r - d;
	}

	A->Collided = B->Collided = true;
}


void CalculateVelocity(PhysicsObject& ObjA) {
	float a = ObjA.RigidBody.EndPoint.x, x = ObjA.Transform.Position.x;
	float b = ObjA.RigidBody.EndPoint.y, y = ObjA.Transform.Position.y;

	Vector2 direction = Vector2{ a - x, b - y };

	float length = sqrt(direction.x * direction.x + direction.y * direction.y);
	if (length != 0) {
		direction = Vector2{ direction.x / length, direction.y / length };
	}
	ObjA.RigidBody.Velocity = Vector2{ direction.x, direction.y };
}
void PositionalCorrection(Manifold* M)
{
	PhysicsObject* ObjA = M->A;
	PhysicsObject* ObjB = M->B;
	float penetration = M->penetration;
	Vector2 normal = M->normal;
	float correctionMagnitude = fmax(penetration - POSITIONAL_CORRECTION_SLOP, 0.0f) / (ObjA->RigidBody.InverseMass + ObjB->RigidBody.InverseMass) * POSITIONAL_CORRECTION_PERCENT;
	Vector2 correction = Vector2{ correctionMagnitude * normal.x, correctionMagnitude * normal.y };
	ObjA->Transform.Position.x -= ObjA->RigidBody.InverseMass * correction.x;
	ObjA->Transform.Position.y -= ObjA->RigidBody.InverseMass * correction.y;

	ObjB->Transform.Position.x += ObjB->RigidBody.InverseMass * correction.x;
	ObjB->Transform.Position.y += ObjB->RigidBody.InverseMass * correction.y;
}
void ResolveCollision(Manifold* M) {
	PhysicsObject* ObjA = M->A;
	PhysicsObject* ObjB = M->B;
	Vector2 relativeVelocity = Vector2{ ObjB->RigidBody.Velocity.x - ObjA->RigidBody.Velocity.x,
									   ObjB->RigidBody.Velocity.y - ObjA->RigidBody.Velocity.y };

	float x = ObjB->Transform.Position.x - ObjA->Transform.Position.x;
	float y = ObjB->Transform.Position.y - ObjA->Transform.Position.y;
	float length = std::sqrt(x * x + y * y);

	Vector2 normalVector = M->normal;

	float velocityAlongNormal = relativeVelocity.x * normalVector.x + relativeVelocity.y * normalVector.y;

	if (velocityAlongNormal > 0) return;

	float e = std::min(ObjA->RigidBody.Restitution, ObjB->RigidBody.Restitution);
	float j = -(1 + e) * velocityAlongNormal;
	j /= ObjA->RigidBody.InverseMass + ObjB->RigidBody.InverseMass;

	Vector2 impulse = Vector2{ j * normalVector.x, j * normalVector.y };

	ObjA->RigidBody.Velocity = Vector2{ ObjA->RigidBody.Velocity.x - (impulse.x * ObjA->RigidBody.InverseMass),
									  ObjA->RigidBody.Velocity.y - (impulse.y * ObjA->RigidBody.InverseMass) };

	ObjB->RigidBody.Velocity = Vector2{ ObjB->RigidBody.Velocity.x + (impulse.x * ObjB->RigidBody.InverseMass),
									  ObjB->RigidBody.Velocity.y + (impulse.y * ObjB->RigidBody.InverseMass) };
	PositionalCorrection(M);
}

void CreateGameObject(int& OBJECT_NUMBER, PhysicsObject ObjList[], Vector2 Position = Vector2{ 0, 0 },
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
	ObjList[OBJECT_NUMBER].RigidBody.Restitution = Restitution;
	ObjList[OBJECT_NUMBER].Collided = false;
	ObjList[OBJECT_NUMBER].RigidBody.InverseMass = (Mass == 0) ? 0 : 1 / Mass;
	OBJECT_NUMBER++;
}

int main() {
	// Declarations
	int OBJECT_NUMBER = 0;

	// Objects
	PhysicsObject Obj[10];
	//CreateGameObject(OBJECT_NUMBER, Obj, Vector2{ 50, 50 }, Vector3{ 0, 0, 0 }, Vector2{ 25, 25 }, 0, 2, Vector2{ 500, 500 }, 1, 0.5f);
	//CreateGameObject(OBJECT_NUMBER, Obj, Vector2{ 100, 180 }, Vector3{ 0, 0, 0 }, Vector2{ 25, 25 }, 0, 2, Vector2{ 400, 400 }, 1, 0.5f);

	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib Program");
	SetTargetFPS(FPS_LIMIT);
	// Debugging
	for (int counter = 0; counter < OBJECT_NUMBER && DEBUG_MODE; counter++) {
		DebugAABB(Obj[counter]);
	}
	Vector2 StartingPoints[100], EndPoints[100], BasicPoints[100];
	int StartPointNumber = 0, EndPointNumber = 0, BasicPointNumber = 0;
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
		CreateGameObject(OBJECT_NUMBER, Obj, StartingPoints[i], Vector3{ 0, 0, 0 }, Vector2{ 25, 25 }, 0, 3, EndPoints[i], 1, 0.5f);
	}
	for (int i = EndPointNumber; i < StartPointNumber; i++)
	{
		CreateGameObject(OBJECT_NUMBER, Obj, StartingPoints[i], Vector3{ 0, 0, 0 }, Vector2{ 25, 25 }, 0, 3, StartingPoints[i], 1, 0.5f);
	}
	for (int i = 0; i < BasicPointNumber; i++)
	{
		CreateGameObject(OBJECT_NUMBER, Obj, BasicPoints[i], Vector3{ 0, 0, 0 }, Vector2{ 50, 50 }, 1, 3, BasicPoints[i], 0, 0.5f);
	}
	// Raylib Loop
	while (!WindowShouldClose()) {
		// Update
// Reset the collision check
		for (int counter = 0; counter < OBJECT_NUMBER; counter++) {
			Obj[counter].Collided = false;
			if (Obj[counter].Type != 0) {
				Obj[counter].toAABB();
			}
		}

		// Intersection Check
		for (int i = 0; i < OBJECT_NUMBER; i++) {
			for (int j = i + 1; j < OBJECT_NUMBER; j++) {
				Manifold m = { &Obj[i], &Obj[j] };
				if (Obj[i].Type == 0 && Obj[j].Type == 0) {

					ObjectInteractionCircles(&m);
				}
				else {
					ObjectInteraction(&m);
				}
			}
		}



		for (int i = 0; i < OBJECT_NUMBER; i++) {
			// Reset the collision status
			Obj[i].Collided = false;

			// Check for collisions and resolve them
			for (int j = 0; j < OBJECT_NUMBER; j++) {
				if (i == j) continue; // Skip checking an object with itself
				Manifold m;
				if (Obj[i].Type == 0 && Obj[j].Type == 0) {
					m = { &Obj[i], &Obj[j] };
					ObjectInteractionCircles(&m);
				}
				else if (Obj[i].Type != Obj[j].Type) {

					if ((Obj[i].Type != 0))
						m = { &Obj[i], &Obj[j] };
					else m = { &Obj[j], &Obj[i] };
					ObjectInteractionMixed(&m);
				}
				else {
					m = { &Obj[i], &Obj[j] };
					ObjectInteraction(&m);
				}

				// Resolve collisions if any occurred
				if (Obj[i].Collided || Obj[j].Collided) {
					ResolveCollision(&m);
				}
			}

			// Handle post-collision or movement logic
			if (Obj[i].Collided) {
				Obj[i].RigidBody.EndPoint = Vector2{ -1, -1 };
			}
			else if (Obj[i].RigidBody.EndPoint.x != -1 && Obj[i].RigidBody.EndPoint.y != -1) {
				float distanceToEndPoint = std::sqrt(
					std::pow(Obj[i].RigidBody.EndPoint.x - Obj[i].Transform.Position.x, 2) +
					std::pow(Obj[i].RigidBody.EndPoint.y - Obj[i].Transform.Position.y, 2)
				);

				if (distanceToEndPoint < 1.5f) {
					Obj[i].RigidBody.Velocity = Vector2{ 0, 0 };
					Obj[i].RigidBody.EndPoint = Vector2{ -1, -1 };
				}
				else {
					CalculateVelocity(Obj[i]);
				}
			}
		}




		// Position Change Loop
		for (int counter = 0; counter < OBJECT_NUMBER; counter++) {
			Obj[counter].Transform.Position = Vector2{
				Obj[counter].Transform.Position.x + Obj[counter].RigidBody.Velocity.x * Obj[counter].RigidBody.ObjectSpeed,
				Obj[counter].Transform.Position.y + Obj[counter].RigidBody.Velocity.y * Obj[counter].RigidBody.ObjectSpeed
			};
		}

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
			PhysicsObject varObj = Obj[counter];
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