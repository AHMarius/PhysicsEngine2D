#include<algorithm>
#include <cmath>
#include <iostream>
#include <raylib.h>

// Constants
const bool DEBUG_MODE = 1;
const int SCREEN_WIDTH = 900;
const int SCREEN_HEIGHT = 900;

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
	std::cout <<"AABB min:\n x=" << Obj.AABB.min.x << "\n y=" << Obj.AABB.min.y << "\nAABB max \n x= " << Obj.AABB.max.x << "\n y= " << Obj.AABB.max.y;
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
void ObjectInteractionMixed(Manifold* M) {
	PhysicsObject* Circle = M->B; // Circle
	PhysicsObject* Rectangle = M->A; // Rectangle
	//Vector from the Rectangle to the Circle
	Vector2 n = Vector2{
		Circle->Transform.Position.x - (Rectangle->AABB.max.x + Rectangle->AABB.min.x) / 2,
		Circle->Transform.Position.y - (Rectangle->AABB.max.y + Rectangle->AABB.min.y) / 2
	};
	//Closest point on the rectangle to the center
	Vector2 closest = n;
	//Calculate half extents for each axis
	Vector2 halfExtent = Vector2{
		(float)(Rectangle->AABB.max.x - Rectangle->AABB.min.x) / 2,
		(float)(Rectangle->AABB.max.y - Rectangle->AABB.min.y) / 2
	};
	//Clamping the points to the edge of the rectangle
	closest = Vector2{
		(float)clamp(closest.x,halfExtent.x,-halfExtent.x),
		(float)clamp(closest.y,halfExtent.y,-halfExtent.y)
	};
	bool inside = false;
	// Circle is inside the AABB, so we need to clamp the circle's center 
	// to the closest edge
	if (n.x == closest.x && n.y == closest.y)
	{
		inside = true;
		//Find the closest axis
		if (std::abs(n.x) > std::abs(n.y))
		{
			//Clamp to closest extent
			if (closest.x > 0)
				closest.x = halfExtent.x;
			else
				closest.x = -halfExtent.x;
		}
		//y axis is shorter
		else {
			//Clamp to closest extent
			if (closest.y > 0)
				closest.y = halfExtent.y;
			else
				closest.y = -halfExtent.y;
		}
	}
	Vector2 normal = Vector2{
		n.x - closest.x,
		n.y - closest.y
	};
	float d = normal.x * normal.x + normal.y * normal.y;
	float r = Circle->Transform.Size.x;
	// Early out of the radius is shorter than distance to closest point and
	// Circle not inside the AABB
	if (d > r * r && !inside)
	{
		Circle->Collided = Rectangle->Collided = false;
		return;
	}
	d = sqrt(d);
	// Collision normal needs to be flipped to point outside if circle was 
	 // inside the AABB
	if (inside) {
		M->normal = Vector2{-n.x,-n.y};
		M->penetration = r - d;
	}
	else {
		M->normal = n;
		M->penetration = r - d;
	}
	Circle->Collided = Rectangle->Collided = true;
	return;
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

	float velocityAlongNormal = relativeVelocity.x * M->normal.x + relativeVelocity.y * M->normal.y;

	if (velocityAlongNormal > 0) return;

	float e = std::min(ObjA->RigidBody.Restitution, ObjB->RigidBody.Restitution);
	float j = -(1 + e) * velocityAlongNormal;
	j /= ObjA->RigidBody.InverseMass + ObjB->RigidBody.InverseMass;

	Vector2 impulse = Vector2{ j * M->normal.x, j * M->normal.y };

	ObjA->RigidBody.Velocity = Vector2{
		ObjA->RigidBody.Velocity.x - impulse.x * ObjA->RigidBody.InverseMass,
		ObjA->RigidBody.Velocity.y - impulse.y * ObjA->RigidBody.InverseMass
	};

	ObjB->RigidBody.Velocity = Vector2{
		ObjB->RigidBody.Velocity.x + impulse.x * ObjB->RigidBody.InverseMass,
		ObjB->RigidBody.Velocity.y + impulse.y * ObjB->RigidBody.InverseMass
	};
	std::cout << "Normal: (" << M->normal.x << ", " << M->normal.y << ")" << std::endl;
	std::cout << "Penetration: " << M->penetration << std::endl;
	std::cout << "Impulse: (" << impulse.x << ", " << impulse.y << ")" << std::endl;
	// Positional correction
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
		CreateGameObject(OBJECT_NUMBER, Obj, StartingPoints[i], Vector3{ 0, 0, 0 }, Vector2{ 100, 25 }, 1, 3, EndPoints[i], 1, 0.5f);
	}
	for (int i = EndPointNumber; i < StartPointNumber; i++)
	{
		CreateGameObject(OBJECT_NUMBER, Obj, StartingPoints[i], Vector3{ 0, 0, 0 }, Vector2{ 25, 25 }, 1, 3, StartingPoints[i], 1, 0.5f);
	}
	for (int i = 0; i < BasicPointNumber; i++)
	{
		CreateGameObject(OBJECT_NUMBER, Obj, BasicPoints[i], Vector3{ 0, 0, 0 }, Vector2{ 50, 50 }, 0, 3, BasicPoints[i], 1, 0.5f);
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
		/*for (int i = 0; i < OBJECT_NUMBER; i++) {
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
		*/


		for (int i = 0; i < OBJECT_NUMBER; i++) {
			// Reset the collision status
			Obj[i].Collided = false;

			// Check for collisions and resolve them
			for (int j = 0; j < OBJECT_NUMBER; j++) {
				std::cout << "Circle Position: (" << Obj[i].Transform.Position.x << ", " << Obj[i].Transform.Position.y << ")" << std::endl;
				std::cout << "Rectangle Position: (" << Obj[j].Transform.Position.x << ", " << Obj[j].Transform.Position.y << ")" << std::endl;
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
				if (Obj[i].Collided && Obj[j].Collided) {
					std::cout << "Before Collision Resolution:" << std::endl;
					std::cout << "Circle Position: (" << Obj[i].Transform.Position.x << ", " << Obj[i].Transform.Position.y << ")" << std::endl;
					std::cout << "Rectangle Position: (" << Obj[j].Transform.Position.x << ", " << Obj[j].Transform.Position.y << ")" << std::endl;
					Obj[i].RigidBody.EndPoint = Vector2{ -1, -1 };
					Obj[j].RigidBody.EndPoint = Vector2{ -1, -1 };
					ResolveCollision(&m);

					std::cout << "After Collision Resolution:" << std::endl;
					std::cout << "Circle Position: (" << Obj[i].Transform.Position.x << ", " << Obj[i].Transform.Position.y << ")" << std::endl;
					std::cout << "Rectangle Position: (" << Obj[j].Transform.Position.x << ", " << Obj[j].Transform.Position.y << ")" << std::endl;
				std::cout << "-----------------------------------------------------------------------------------------------------------------";

				}
			}

			 if (Obj[i].RigidBody.EndPoint.x != -1 && Obj[i].RigidBody.EndPoint.y != -1) {
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
			
			if (Obj[counter].Type == 0) {
				Vector2 velocityEndPoint = Vector2{
				Obj[counter].Transform.Position.x + Obj[counter].RigidBody.Velocity.x * VECTOR_LENGHT,
				Obj[counter].Transform.Position.y + Obj[counter].RigidBody.Velocity.y * VECTOR_LENGHT
				};
				DrawLineEx(Obj[counter].Transform.Position, velocityEndPoint, LINE_THICKNESS, BLUE);
			}
			else
			{
				Vector2 velocityEndPoint = Vector2{
				(Obj[counter].AABB.max.x + Obj[counter].AABB.min.x) / 2 + Obj[counter].RigidBody.Velocity.x * VECTOR_LENGHT,
				(Obj[counter].AABB.max.y + Obj[counter].AABB.min.y) / 2 + Obj[counter].RigidBody.Velocity.y * VECTOR_LENGHT
				};
				DrawLineEx(Vector2{ (Obj[counter].AABB.max.x + Obj[counter].AABB.min.x) / 2,(Obj[counter].AABB.max.y + Obj[counter].AABB.min.y) / 2} , velocityEndPoint, LINE_THICKNESS, BLUE);
			}
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
				DrawLineV(Vector2{ (varObj.AABB.max.x + varObj.AABB.min.x) / 2,(varObj.AABB.max.y + varObj.AABB.min.y) / 2 }, varObj.RigidBody.EndPoint, YELLOW);
			}
		}

		EndDrawing();
	}
	CloseWindow();
	return 0;
}