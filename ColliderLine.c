/*!****************************************************************************
\file   ColliderLine.c
\author Austin Clark
\par    Course: CS230S22
\par    Copyright 2022 DigiPen (USA) Corporation

\brief
  An interface that allows for the creation, manipulation and checking of Line
  Collider objects. Line Colliders are Collider components that have been
  reinterpret cast to allow for storage of additional line segment data. During
  gameplay, a function call to ColliderLineIsCollidingWithCircle will check 
  if for collision between any line segment and a circle collider. If a
  collision is found, the appropriate physics and translation updates will be
  made.

******************************************************************************/

#include "stdafx.h"
#include "ColliderCircle.h"
#include "ColliderLine.h"
#include "Collider.h"
#include "Vector2D.h"
#include "Stream.h"
#include "GameObject.h"
#include "Transform.h"
#include "Physics.h"

#define cLineSegmentMax 50

/* Container for storing the beginner and end coordinates for a line segment */
typedef struct ColliderLineSegment
{
	/* A single line segment (P0 and P1) */
	Vector2D	point[2];

} ColliderLineSegment;

/* Used when Collider objects are reinterpreted as a Line Collider */
/* Allows for C++ class sudo-inheritence in C                      */
typedef struct ColliderLine
{
	/* Inherit the base collider structure. */
	Collider base;

	/* The number of line segments in the list. */
	unsigned int lineCount;

	/* The individual line segments. */
	ColliderLineSegment	lineSegments[cLineSegmentMax];

} ColliderLine, *ColliderLinePtr;

/*!****************************************************************************
\brief
  Dynamically allocates a new (line) collider component.
******************************************************************************/
ColliderPtr ColliderLineCreate(void)
{
	/* Allocates memory for new Line Collider component */
	ColliderLinePtr newColliderLine = calloc(1, sizeof(ColliderLine));

	if (newColliderLine)
	{
	/* Sets parameters to default values for a Line Collider */
		newColliderLine->base.type = ColliderTypeLine;
		newColliderLine->base.memorySize = sizeof(ColliderLine);
		newColliderLine->lineCount = 0;
		return (ColliderPtr)newColliderLine;
	}

	/* Catch case for if memory allocation fails */
	return NULL;
}

/*!****************************************************************************
\brief
  Reads the properties of a ColliderLine component from a file.

\param collider
  Pointer to the Collider component.

\param stream
  Pointer to the data stream used for reading.
******************************************************************************/
void ColliderLineRead(ColliderPtr collider, Stream stream)
{
	/* Reads in the number of line segments there will be in the file */
	int lineCount = StreamReadInt(stream);

	/* Adds all line segments from the filestream to the Line Collider */
	for (int i = 0; i < lineCount; i++)
	{
		Vector2D p0; /* Coodrinates for the beginning of the line */
		Vector2D p1; /* Coodrinates for the end  of the line */
		StreamReadVector2D(stream, &p0); /* Reads line beginning location */
		StreamReadVector2D(stream, &p1); /* Reads line end location */
		ColliderLineAddLineSegment(collider, &p0, &p1); /* Stores in collider */
	}
}

/*!****************************************************************************
\brief
  Adds a line segment to the Line Collider's line segment list.

\param collider
  Pointer to the Collider component.

\param p0
  The line segment's starting position.

\param p1
  The line segment's ending position.
******************************************************************************/
void ColliderLineAddLineSegment(ColliderPtr collider, const Vector2D* p0, const Vector2D* p1)
{
	if (collider && p0 && p1)
	{
		/* Redefines Collider object as a Line Collider for manipulation */
		ColliderLinePtr colliderLine = (ColliderLinePtr)collider;

		/* Adds a line segment to the Line Collider's line segment list. */
		colliderLine->lineSegments[colliderLine->lineCount].point[0] = *p0;
		colliderLine->lineSegments[colliderLine->lineCount].point[1] = *p1;
		colliderLine->lineCount++;
	}
}

/*!****************************************************************************
\brief
  Checks for collision between a Line Collider and a Circle Collider.

\param collider
  Pointer to a possible Line Collider component.

\param other
  Pointer to a possible Circle Collider component.
******************************************************************************/
bool ColliderLineIsCollidingWithCircle(ColliderPtr collider, ColliderPtr other)
{
	/* Only checks collision if objects are of correct type */
	if (collider->type == ColliderTypeLine && other->type == ColliderTypeCircle)
	{
		/* Reinterprets Collider as Line Collider for line segment access */
		ColliderLinePtr colliderLine = (ColliderLinePtr)collider;

		/* Retrieves prior coordinates of the parent object and the coodinates of */
		/* where it will be drawn next, used in collision resolution if needed    */
		Vector2D Bs = *PhysicsGetOldTranslation(GameObjectGetPhysics(other->parent));
		Vector2D Be = *TransformGetTranslation(GameObjectGetTransform(other->parent));
		Vector2D v; /* Container for result of distance calculation */

		/* Performs a matrixies subtraction, result stored in v */
		Vector2DSub(&v, &Be, &Bs);

		/* Performs calculation that determines if the circle collider is contacting */
		/* any of the line segments in the Line Collider                             */
		for (unsigned int j = 0; j < colliderLine->lineCount; j++)
		{
			/* Retrives the current line segment coordinates */
			Vector2D P0 = colliderLine->lineSegments[j].point[0];
			Vector2D P1 = colliderLine->lineSegments[j].point[1];
			Vector2D e; /* Container for result of distance calculation */

			/* Performs a matrixies subtraction, result stored in  e */
			Vector2DSub(&e, &P1, &P0);

			/* Container  used to store a unit vector of e */
			Vector2D n = { e.y, -e.x };
			Vector2DNormalize(&n, &n);

			/* Dot products used to determine if objects are on top of each other */
			if (Vector2DDotProduct(&n, &v) == 0)
			{
				continue;
			}

			/* Dot products used to determine if line segment is currently */
			/* being collided with                                         */
			if (Vector2DDotProduct(&n, &Bs) <= Vector2DDotProduct(&n, &P0) &&
					Vector2DDotProduct(&n, &Be) < Vector2DDotProduct(&n, &P0))
			{
				continue;
			}
			if (Vector2DDotProduct(&n, &Bs) >= Vector2DDotProduct(&n, &P0) &&
					Vector2DDotProduct(&n, &Be) > Vector2DDotProduct(&n, &P0))
			{
				continue;
			}

			/* Value used to scale the colliders unit vector to determine if the */
			/* there will be a collison during the next frame                    */
			float ti = (Vector2DDotProduct(&n, &P0) - Vector2DDotProduct(&n, &Bs)) /
									Vector2DDotProduct(&n, &v);

			/* Calculations used to determine if line segment will be collided */
			/* with during the next frame render                               */
			Vector2D Bi;
			Vector2DScaleAdd(&Bi, &v, &Bs, ti);
			
			Vector2D p0Check1, p0Check2;
			Vector2DSub(&p0Check1, &P1, &P0);
			Vector2DSub(&p0Check2, &Bi, &P0);
			if (Vector2DDotProduct(&p0Check1, &p0Check2) < 0 )
				continue;

			Vector2D p1Check1, p1Check2;
			Vector2DSub(&p1Check1, &P0, &P1);
			Vector2DSub(&p1Check2, &Bi, &P1);
			if (Vector2DDotProduct(&p1Check1, &p1Check2) < 0)
				continue;

			/* Collision is taking place, parent object needs an updated translation */
			Vector2D i, s, r, Br, oldVel, newVel;
			Vector2DSub(&i, &Be, &Bi);
			Vector2DScale(&s, &n, Vector2DDotProduct(&i, &n));
			Vector2DScale(&s, &s, 2.0f);
			Vector2DSub(&r, &i, &s);
			Vector2DAdd(&Br, &Bi, &r);
			TransformSetTranslation(GameObjectGetTransform(other->parent), &Br);

			/* Collision is taking place, parent object needs an updated rotation angle */
			float angle = Vector2DToAngleRad(&r);
			TransformSetRotation(GameObjectGetTransform(other->parent), angle);

			/* Collision is taking place, parent object needs an updated velocity for */
			/* its new trajectory                                                     */
			oldVel = *PhysicsGetVelocity(GameObjectGetPhysics(other->parent));
			float speed = Vector2DLength(&oldVel);
			Vector2DNormalize(&newVel, &r);
			Vector2DScale(&newVel, &newVel, speed);
			PhysicsSetVelocity(GameObjectGetPhysics(other->parent), &newVel);

			/* Moves on to the next line segment */
			continue;
		}

	}

  	/* None of the line segments were being collided with */
	return false;
}

