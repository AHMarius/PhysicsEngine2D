#include <cmath>
#include <iostream>
#include <raylib.h>
#include <algorithm> 
//
#define RADIUS Transform.Size.x
// Constants
const bool DEBUG_MODE = 1;
const unsigned short SCREEN_WIDTH = 800;
const unsigned short SCREEN_HEIGHT = 600;
const bool SPAWN_OBJECT_STATIC_TYPE = 0;
const bool SPAWN_OBJECT_VARIABLE_TYPE = 1;
const unsigned short FPS_LIMIT = 60;
const unsigned short VECTOR_LENGHT = 60;
const float CORRECTION_PERCENT = 0.2;
const float CORRECTION_SLOP = 0.01;
const float LINE_THICKNESS = 2.0f;
const float GIZMOS_SIZE = 2.0f;

template <typename T>
T Clamp(const T& value, const T& min, const T& max) {
	if (value < min) return min;
	if (value > max) return max;
	return value;
}
Vector2 VectorSum(Vector2 A, Vector2 B, bool sign) {
	if (sign == 1)
		return Vector2{
		A.x + B.x,
		A.y + B.y
	};
	else
		return Vector2{
		A.x - B.x,
		A.y - B.y
	};
}
Vector2 VectorProduct(Vector2 A, Vector2 B, bool sign) {
	if (!sign)
		return Vector2{
		A.x * B.x,
		A.y * B.y
	};
	else
		return Vector2{
		(B.x) ? A.x / B.x : 0,
		(B.y) ? A.y / B.y : 0
	};
}
float LenghtSquared(Vector2 A) {
	return A.x * A.x + A.y * A.y;
}
// Structs
struct AABBS {
	Vector2 min;
	Vector2 max;
};
struct PhysicsObject2D {
	unsigned short Type; // 0-circle, 1-rectangle
	bool Collided;
	struct {
		Vector2 Position;
		Vector2 CenterPosition;
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

	AABBS AABB;

	void toAABB() {
		AABB.min = Transform.Position;
		AABB.max = Vector2{ Transform.Size.x + Transform.Position.x, Transform.Size.y + Transform.Position.y };
		Transform.CenterPosition = VectorSum(Transform.Position, Vector2{ (AABB.max.x - AABB.min.x) / 2, (AABB.max.y - AABB.min.y) / 2 }, 1);
	}
	void CalculateVelocity() {
		float a = RigidBody.EndPoint.x, x = Transform.CenterPosition.x;
		float b = RigidBody.EndPoint.y, y = Transform.CenterPosition.y;

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
		float distanceToEndPoint = sqrt(pow(RigidBody.EndPoint.x - Transform.CenterPosition.x, 2) +
			pow(RigidBody.EndPoint.y - Transform.CenterPosition.y, 2));

		if (distanceToEndPoint < 2.0f) {
			RigidBody.Velocity = Vector2{ 0, 0 };
			RigidBody.EndPoint = Vector2{ -1, -1 };
		}
		else if (!Collided) {
			CalculateVelocity();
		}

	}
};

struct Manifold
{
	PhysicsObject2D* A;
	PhysicsObject2D* B;
	float penetration;
	Vector2 normal;
};

void PositionalCorrection(Manifold* m) {
	PhysicsObject2D* A = m->A;
	PhysicsObject2D* B = m->B;
	float penetration = m->penetration;
	Vector2 normal = m->normal;
	Vector2 correction = Vector2{
		(std::max(penetration - CORRECTION_SLOP,0.0f) / (A->RigidBody.InvMass + B->RigidBody.InvMass)) * CORRECTION_PERCENT * normal.x,
		(std::max(penetration - CORRECTION_SLOP,0.0f) / (A->RigidBody.InvMass + B->RigidBody.InvMass)) * CORRECTION_PERCENT * normal.y
	};
	A->Transform.Position = Vector2{
	A->Transform.Position.x - (A->RigidBody.InvMass * correction.x),
	A->Transform.Position.y - (A->RigidBody.InvMass * correction.y)
	};

	B->Transform.Position = Vector2{
		B->Transform.Position.x + (B->RigidBody.InvMass * correction.x),
		B->Transform.Position.y + (B->RigidBody.InvMass * correction.y)
	};
}

void ObjectInteraction(Manifold* m) {
	PhysicsObject2D* A = m->A;
	PhysicsObject2D* B = m->B;
	AABBS abox = A->AABB;
	AABBS bbox = B->AABB;
	//Vector from A to B
	Vector2 n = VectorSum(B->Transform.CenterPosition, A->Transform.CenterPosition, 0);


	//calculate hald extends along the x axis for each object
	float a_extent = (abox.max.x - abox.min.x) / 2;
	float b_extent = (bbox.max.x - bbox.min.x) / 2;
	//overlap on the x axis
	float x_overlap = a_extent + b_extent - std::abs(n.x);
	//SAT test on x axis
	if (x_overlap > 0) {
		//calculate half extents along y axis for each object
		float a_extent = (abox.max.y - abox.min.y) / 2;
		float b_extent = (bbox.max.y - bbox.min.y) / 2;
		// overlap on y axis 
		float y_overlap = a_extent + b_extent - std::abs(n.y);
		// SAT test on y axis 
		if (y_overlap > 0)
		{
			// Find out which axis is axis of least penetration 
			if (x_overlap < y_overlap)
			{
				// Point towards B knowing that n points from A to B 
				if (n.x < 0)
					m->normal = Vector2{ -1, 0 };
				else
					m->normal = Vector2{ 1, 0 };
				m->penetration = x_overlap;
				A->Collided = B->Collided = true;
				return;
			}
			else {
				if (n.y < 0)
					m->normal = Vector2{ 0,-1 };
				else
					m->normal = Vector2{ 0,1 };
				m->penetration = y_overlap;
				A->Collided = B->Collided = true;
				return;
			}
		}
	}
}
void  ObjectInteractionMixed(Manifold* m) {
	// Setup pointers to each object
	PhysicsObject2D* A = m->A;
	PhysicsObject2D* B = m->B;

	// Vector from A to B
	Vector2 n = VectorSum(B->Transform.Position, A->Transform.CenterPosition, 0);

	// Closest point on A to center of B
	Vector2 closest = n;

	// Calculate half extents along each axis
	float x_extent = (A->AABB.max.x - A->AABB.min.x) / 2;
	float y_extent = (A->AABB.max.y - A->AABB.min.y) / 2;

	// Clamp point to edges of the AABB
	closest.x = Clamp(closest.x, -x_extent, x_extent);
	closest.y = Clamp(closest.y, -y_extent, y_extent);

	bool inside = false;
	// Check if the circle's center is inside the AABB
	if (n.x == closest.x && n.y == closest.y) {
		inside = true;

		// Find the closest axis
		if (std::abs(n.x) > std::abs(n.y)) {
			closest.x = (closest.x > 0) ? x_extent : -x_extent;
		}
		else {
			closest.y = (closest.y > 0) ? y_extent : -y_extent;
		}
	}

	// Compute the distance squared from the circle's center to the closest point
	Vector2 normal = VectorSum(n, closest, 0);
	float d_squared = LenghtSquared(normal);
	float r_squared = B->Transform.Size.x * B->Transform.Size.x;
	std::cout << "R_sq=" << r_squared << " D_sq= " << d_squared << std::endl;
	// Early out if the distance squared is greater than the radius squared and the circle is not inside the AABB
	if (d_squared > r_squared && !inside) {
		A->Collided = B->Collided = false;
		return;
	}

	// Compute the actual distance
	float d = std::sqrt(d_squared);

	// Set the collision normal and penetration depth
	if (inside) {
		m->normal = Vector2{ -n.x, -n.y };
		m->penetration = B->Transform.Size.x - d;
	}
	else {
		m->normal = n;
		m->penetration = B->Transform.Size.x - d;
	}
	float length = std::sqrt(m->normal.x * m->normal.x + m->normal.y * m->normal.y);
	if (length != 0) {  // Avoid division by zero
		m->normal.x /= length;
		m->normal.y /= length;
	}
	std::cout << "m->normal x=" << m->normal.x << " m->normal y= " << m->normal.y << " penetration =" << m->penetration << std::endl;
	std::cout << "col";
	A->Collided = B->Collided = true;
	return;
}
void ObjectInteractionCircles(Manifold* m) {
	PhysicsObject2D* A = m->A;
	PhysicsObject2D* B = m->B;
	//Vector from A to B
	Vector2 n = VectorSum(B->Transform.Position, A->Transform.Position, 0);
	float r = B->Transform.Size.x + A->Transform.Size.x;
	r *= r;
	if (LenghtSquared(n) > r)
	{
		A->Collided = B->Collided = false;
		return;
	}
	//Computing manifolds after circle collide
	float d = std::sqrt(LenghtSquared(n));
	//Distance between circles is not 0
	if (d != 0) {
		//distance is the diffrence between radius and distance
		m->penetration = r - d;
		//points from A to B, unit vector
		m->normal = VectorProduct(n, Vector2{ d,d }, 1);
		A->Collided = B->Collided = true;
		return;
	}
	//circle are on the same position
	else {
		//random consistent values
		m->penetration = A->RADIUS;
		m->normal = Vector2{ 1,0 };
		A->Collided = B->Collided = true;
		return;
	}
}


void ResolveCollision(Manifold* m) {
	PhysicsObject2D* A = m->A;
	PhysicsObject2D* B = m->B;
	Vector2 relativeVelocity = Vector2{ B->RigidBody.Velocity.x - A->RigidBody.Velocity.x,
									   B->RigidBody.Velocity.y - A->RigidBody.Velocity.y };

	float x = B->Transform.Position.x - A->Transform.Position.x;
	float y = B->Transform.Position.y - A->Transform.Position.y;
	float length = std::sqrt(x * x + y * y);

	Vector2 normalVector = m->normal;

	float velocityAlongNormal = relativeVelocity.x * normalVector.x + relativeVelocity.y * normalVector.y;

	if (velocityAlongNormal > 0) return;

	float e = std::min(A->RigidBody.Restitution, B->RigidBody.Restitution);

	float j = -(1 + e) * velocityAlongNormal;
	j /= A->RigidBody.InvMass + B->RigidBody.InvMass;

	Vector2 impulse = Vector2{ j * normalVector.x, j * normalVector.y };

	A->RigidBody.Velocity = Vector2{ A->RigidBody.Velocity.x - (impulse.x * A->RigidBody.InvMass),
									  A->RigidBody.Velocity.y - (impulse.y * A->RigidBody.InvMass) };

	B->RigidBody.Velocity = Vector2{ B->RigidBody.Velocity.x + (impulse.x * B->RigidBody.InvMass),
									  B->RigidBody.Velocity.y + (impulse.y * B->RigidBody.InvMass) };
	PositionalCorrection(m);
}

void CreateGameObject(int& OBJECT_NUMBER, PhysicsObject2D ObjList[], Vector2 Position = Vector2{ 0, 0 },
	Vector3 Rotation = Vector3{ 0, 0, 0 }, Vector2 Size = Vector2{ 1, 1 }, unsigned short Type = 0,
	float ObjectSpeed = 1, Vector2 EndPoint = Vector2{ -1, -1 }, float Mass = 1, float Restitution = 0.5f) {
	ObjList[OBJECT_NUMBER].Transform.Position = Position;
	ObjList[OBJECT_NUMBER].Transform.CenterPosition = Position;
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
	CloseWindow();
	for (int i = 0; i < EndPointNumber; i++)
	{
		CreateGameObject(OBJECT_NUMBER, Obj, StartingPoints[i], Vector3{ 0, 0, 0 }, Vector2{ 255, 50 }, SPAWN_OBJECT_VARIABLE_TYPE, 3, EndPoints[i], 1, 0.95f);
	}
	for (int i = EndPointNumber; i < StartPointNumber; i++)
	{
		CreateGameObject(OBJECT_NUMBER, Obj, StartingPoints[i], Vector3{ 0, 0, 0 }, Vector2{ 255, 50 }, SPAWN_OBJECT_VARIABLE_TYPE, 3, StartingPoints[i], 1, 0.95f);
	}
	for (int i = 0; i < BasicPointNumber; i++)
	{
		CreateGameObject(OBJECT_NUMBER, Obj, BasicPoints[i], Vector3{ 0, 0, 0 }, Vector2{ 25, 25 }, SPAWN_OBJECT_STATIC_TYPE, 3, BasicPoints[i], 1, 0.95f);
	}
}

void CollisionLogic(PhysicsObject2D Obj[], int OBJECT_NUMBER)
{
	for (int i = 0; i < OBJECT_NUMBER; i++) {
		Obj[i].Collided = false;
		if (Obj[i].Type != 0) {
			Obj[i].toAABB();
		}
		else {
			Obj[i].Transform.CenterPosition = Obj[i].Transform.Position;
		}
	}

	// Velocity Calculations loop
	for (int i = 0; i < OBJECT_NUMBER; i++) {
		// Check for collisions and resolve them
		for (int j = i + 1; j < OBJECT_NUMBER; j++) {  // Change loop to j = i + 1 to avoid duplicate checks
			Manifold M{ &Obj[i], &Obj[j] };

			if (Obj[i].Type == 0 && Obj[j].Type == 0) {
				// Both are circles
				ObjectInteractionCircles(&M);
			}
			else if (Obj[i].Type == 0 || Obj[j].Type == 0) {
				// One is a circle and the other is a rectangle
				ObjectInteractionMixed(&M);
			}
			else {
				// Both are rectangles
				ObjectInteraction(&M);
			}

			// Resolve collisions if there is any
			if (Obj[i].Collided && Obj[j].Collided) {
				Obj[i].RigidBody.EndPoint = Vector2{ -1, -1 };
				Obj[j].RigidBody.EndPoint = Vector2{ -1, -1 };
				ResolveCollision(&M);
			}
		}

		Obj[i].MoveTowards();  // Move object after checking all potential collisions
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
	for (int i = 0; i < OBJECT_NUMBER; i++) {
		if (Obj[i].Type == 0) {
			DrawCircleLinesV(Obj[i].Transform.Position, Obj[i].Transform.Size.x, (!Obj[i].Collided) ? WHITE : RED);
		}
		else {
			Rectangle rec{ Obj[i].Transform.Position.x, Obj[i].Transform.Position.y, Obj[i].Transform.Size.x, Obj[i].Transform.Size.y };
			DrawRectangleLinesEx(rec, LINE_THICKNESS, (!Obj[i].Collided) ? WHITE : RED);
		}

		// Draw velocity vector
		Vector2 velocityEndPoint = Vector2{
			Obj[i].Transform.CenterPosition.x + Obj[i].RigidBody.Velocity.x * VECTOR_LENGHT,
			Obj[i].Transform.CenterPosition.y + Obj[i].RigidBody.Velocity.y * VECTOR_LENGHT
		};

		DrawLineEx(Obj[i].Transform.CenterPosition, velocityEndPoint, LINE_THICKNESS, BLUE);
	}

	// Draw Hitboxes
	for (int i = 0; i < OBJECT_NUMBER && DEBUG_MODE; i++) {
		PhysicsObject2D varObj = Obj[i];
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
			DrawLineV(varObj.Transform.CenterPosition, varObj.RigidBody.EndPoint, YELLOW);
		}
	}
	EndDrawing();
}
void DebugPrint(PhysicsObject2D Obj[], int OBJECT_NUMBER) {
	std::cout << "Debug Information:" << std::endl;

	for (int i = 0; i < OBJECT_NUMBER; i++) {
		PhysicsObject2D& obj = Obj[i];

		std::string type = (obj.Type == 0) ? "Circle" : "Rectangle";
		std::cout << "Object " << i << ":" << std::endl;
		std::cout << "  Type: " << type << std::endl;
		std::cout << "  Position: (" << obj.Transform.Position.x << ", " << obj.Transform.Position.y << ")" << std::endl;
		std::cout << "  Size: (" << obj.Transform.Size.x << ", " << obj.Transform.Size.y << ")" << std::endl;
		std::cout << "  Center Position: (" << obj.Transform.CenterPosition.x << ", " << obj.Transform.CenterPosition.y << ")" << std::endl;
		std::cout << "  Velocity: (" << obj.RigidBody.Velocity.x << ", " << obj.RigidBody.Velocity.y << ")" << std::endl;
		std::cout << "  End Point: (" << obj.RigidBody.EndPoint.x << ", " << obj.RigidBody.EndPoint.y << ")" << std::endl;
		std::cout << "  Collided: " << (obj.Collided ? "Yes" : "No") << std::endl;

		// Print AABB information for rectangles
		if (obj.Type != 0) {
			std::cout << "  AABB Min: (" << obj.AABB.min.x << ", " << obj.AABB.min.y << ")" << std::endl;
			std::cout << "  AABB Max: (" << obj.AABB.max.x << ", " << obj.AABB.max.y << ")" << std::endl;
		}

		std::cout << std::endl;
	}
}
int main() {
	// Declarations
	int OBJECT_NUMBER = 0;
	PhysicsObject2D Obj[100];

	//CreateGameObject(OBJECT_NUMBER, Obj, Vector2{ 250,1 }, Vector3{ 0, 0, 0 }, Vector2{ 255, 50 }, 1, 1, Vector2{ 250,300 }, 1, 0.95f);
	//CreateGameObject(OBJECT_NUMBER, Obj, Vector2{ 250,250 }, Vector3{ 0, 0, 0 }, Vector2{ 25, 50 }, 0, 1, Vector2{ 250,250 }, 1, 0.95f);

	SetupLogic(OBJECT_NUMBER, Obj);
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib Program");
	SetTargetFPS(FPS_LIMIT);
	// Raylib Loop
	while (!WindowShouldClose() && !IsKeyPressed(KEY_A)) {
		DebugPrint(Obj, OBJECT_NUMBER);
		CollisionLogic(Obj, OBJECT_NUMBER);
		PositionUpdate(Obj, OBJECT_NUMBER);
		DrawScreenLogic(Obj, OBJECT_NUMBER);
	}

	CloseWindow();
	return 0;
}