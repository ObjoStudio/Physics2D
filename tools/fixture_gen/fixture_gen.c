// Physics2D golden fixture generator.
//
// Links against the pinned Box2D 3.1.1 checkout (see docs/PORTING.md for the
// exact commit) and emits deterministic fixture records to stdout. The
// committed fixture files under testdata/golden/ are plain data; nothing in the
// normal build, tests, module, or demo invokes this tool or a Box2D binary.
//
// Build (from the repository root):
//   tools/fixture_gen/build.sh
// Run:
//   build/fixture_gen <fixture-name>   # prints one fixture stream to stdout

#include "box2d/box2d.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Deterministic printing with enough digits for Double tolerance comparisons.
static void PrintNum(float value)
{
	printf("%.9g", (double)value);
}

static void PrintVec(const char* label, b2Vec2 v)
{
	printf("%s ", label);
	PrintNum(v.x);
	printf(" ");
	PrintNum(v.y);
	printf("\n");
}

// ---------------------------------------------------------------- maths ----

static void EmitMaths(void)
{
	// Deterministic input pairs.
	const float inputs[][4] = {
		{1.0f, 0.0f, 0.0f, 1.0f},
		{3.0f, 4.0f, 1.0f, 2.0f},
		{-2.5f, 1.25f, 4.5f, -3.0f},
		{0.0f, 0.0f, 5.0f, 0.0f},
		{1e6f, -1e-6f, 2.0f, 3.0f},
		{0.70710678f, 0.70710678f, -0.5f, 0.5f},
	};
	const int count = (int)(sizeof(inputs) / sizeof(inputs[0]));
	for (int i = 0; i < count; ++i)
	{
		b2Vec2 a = {inputs[i][0], inputs[i][1]};
		b2Vec2 b = {inputs[i][2], inputs[i][3]};
		printf("maths|%d|", i);
		PrintNum(a.x); printf(" ");
		PrintNum(a.y); printf(" ");
		PrintNum(b.x); printf(" ");
		PrintNum(b.y); printf("|");
		PrintNum(b2Dot(a, b)); printf(" ");
		PrintNum(b2Cross(a, b)); printf(" ");
		b2Vec2 s = b2CrossSV(2.5f, b);
		PrintNum(s.x); printf(" ");
		PrintNum(s.y); printf(" ");
		b2Vec2 t = b2CrossVS(b, 2.5f);
		PrintNum(t.x); printf(" ");
		PrintNum(t.y); printf(" ");
		b2Vec2 sum = b2Add(a, b);
		PrintNum(sum.x); printf(" ");
		PrintNum(sum.y); printf(" ");
		b2Vec2 d = b2Sub(a, b);
		PrintNum(d.x); printf(" ");
		PrintNum(d.y); printf(" ");
		b2Vec2 l = b2Lerp(a, b, 0.25f);
		PrintNum(l.x); printf(" ");
		PrintNum(l.y); printf(" ");
		PrintNum(b2Length(a)); printf(" ");
		PrintNum(b2Distance(a, b)); printf(" ");
		b2Vec2 n = b2Normalize(a);
		PrintNum(n.x); printf(" ");
		PrintNum(n.y); printf(" ");
		b2Rot r = b2MakeRot(0.4f);
		b2Vec2 rt = b2RotateVector(r, a);
		PrintNum(rt.x); printf(" ");
		PrintNum(rt.y); printf(" ");
		PrintNum(b2Atan2(a.y, a.x));
		printf("\n");
	}
}

// ----------------------------------------------------------------- hull ----

static void EmitHull(void)
{
	// Square with an interior point.
	b2Vec2 points1[5] = {{0.0f, 0.0f}, {2.0f, 0.0f}, {2.0f, 2.0f}, {0.0f, 2.0f}, {1.0f, 1.0f}};
	// Convex pentagon.
	b2Vec2 points2[5] = {{0.0f, 0.0f}, {4.0f, 0.0f}, {5.0f, 2.0f}, {2.5f, 4.0f}, {0.5f, 2.0f}};
	// Collinear points plus extremes.
	b2Vec2 points3[5] = {{0.0f, 0.0f}, {1.0f, 1.0f}, {2.0f, 2.0f}, {3.0f, 0.0f}, {1.5f, 0.5f}};
	// Duplicated points.
	b2Vec2 points4[6] = {{0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}};
	// Near-collinear.
	b2Vec2 points5[4] = {{0.0f, 0.0f}, {1.0f, 0.0001f}, {2.0f, 0.0f}, {1.0f, 1.0f}};

	const b2Vec2* sets[5] = {points1, points2, points3, points4, points5};
	const int counts[5] = {5, 5, 5, 6, 4};
	for (int i = 0; i < 5; ++i)
	{
		b2Hull hull = b2ComputeHull(sets[i], counts[i]);
		printf("hull|%d|%d|", i, hull.count);
		for (int j = 0; j < hull.count; ++j)
		{
			PrintNum(hull.points[j].x);
			printf(" ");
			PrintNum(hull.points[j].y);
			if (j + 1 < hull.count)
				printf(" ");
		}
		printf("|%d\n", b2ValidateHull(&hull) ? 1 : 0);

		if (b2ValidateHull(&hull))
		{
			b2Polygon poly = b2MakePolygon(&hull, 0.0f);
			printf("poly|%d|%d|", i, poly.count);
			for (int j = 0; j < poly.count; ++j)
			{
				PrintNum(poly.vertices[j].x);
				printf(" ");
				PrintNum(poly.vertices[j].y);
				if (j + 1 < poly.count)
					printf(" ");
			}
			printf("|");
			PrintNum(poly.radius);
			printf("\n");

			b2Polygon box = b2MakeBox(1.0f, 0.5f);
			printf("box|%d|%d|", i, box.count);
			for (int j = 0; j < box.count; ++j)
			{
				PrintNum(box.vertices[j].x);
				printf(" ");
				PrintNum(box.vertices[j].y);
				if (j + 1 < box.count)
					printf(" ");
			}
			printf("|");
			PrintNum(box.radius);
			printf("\n");
		}
	}
}

// ------------------------------------------------------------- distance ----

static void EmitDistance(void)
{
	// Segment-segment distance: p1 q1 p2 q2.
	const float segs[][8] = {
		{0, 0, 1, 0, 0, 1, 1, 1},
		{0, 0, 2, 0, 1, -1, 1, 3},
		{0, 0, 1, 1, 2, 2, 3, 3},
		{-1, 0, 1, 0, 0, -2, 0, 2},
		{0, 0, 1, 0, 0.5f, 0.25f, 2.0f, 0.25f},
	};
	const int count = (int)(sizeof(segs) / sizeof(segs[0]));
	for (int i = 0; i < count; ++i)
	{
		b2SegmentDistanceResult r = b2SegmentDistance(
			(b2Vec2){segs[i][0], segs[i][1]}, (b2Vec2){segs[i][2], segs[i][3]},
			(b2Vec2){segs[i][4], segs[i][5]}, (b2Vec2){segs[i][6], segs[i][7]});
		printf("segdist|%d|", i);
		PrintNum(r.fraction1); printf(" ");
		PrintNum(r.fraction2); printf(" ");
		PrintNum(r.distanceSquared); printf(" ");
		PrintNum(r.closest1.x); printf(" ");
		PrintNum(r.closest1.y); printf(" ");
		PrintNum(r.closest2.x); printf(" ");
		PrintNum(r.closest2.y);
		printf("\n");
	}

	// GJK shape distance: circle versus box.
	b2Circle circle = {{0.5f, 0.5f}, 0.5f};
	b2Polygon box = b2MakeBox(1.0f, 1.0f);
	b2Transform xfA = {{0.0f, 0.0f}, b2Rot_identity};
	b2Transform xfB = {{2.5f, 0.0f}, b2MakeRot(0.3f)};

	b2ShapeProxy proxyA = b2MakeProxy(&circle.center, 1, circle.radius);
	b2ShapeProxy proxyB = b2MakeProxy(box.vertices, box.count, box.radius);

	b2DistanceInput input;
	input.proxyA = proxyA;
	input.proxyB = proxyB;
	input.transformA = xfA;
	input.transformB = xfB;
	input.useRadii = true;

	b2SimplexCache cache;
	cache.count = 0;
	b2Simplex simplexPool[3];
	b2DistanceOutput output = b2ShapeDistance(&input, &cache, simplexPool, false);

	printf("gjk|circle_box|");
	PrintNum(output.distance); printf(" ");
	PrintNum(output.pointA.x); printf(" ");
	PrintNum(output.pointA.y); printf(" ");
	PrintNum(output.pointB.x); printf(" ");
	PrintNum(output.pointB.y); printf(" ");
	PrintNum(cache.count);
	printf("\n");
}

// ------------------------------------------------------------- ray casts ----

static void EmitRayCasts(void)
{
	const b2Circle circle = {{0.0f, 0.0f}, 1.0f};
	const b2Capsule capsule = {{-1.0f, 0.0f}, {1.0f, 0.0f}, 0.5f};
	const b2Segment segment = {{-2.0f, 0.0f}, {2.0f, 0.0f}};
	const b2Polygon box = b2MakeBox(1.0f, 1.0f);

	// origin, translation, maxFraction
	const float rays[][5] = {
		{0.0f, 5.0f, 0.0f, -10.0f, 1.0f},
		{-5.0f, 0.5f, 10.0f, 0.0f, 1.0f},
		{0.0f, 3.0f, 0.0f, 1.0f, 1.0f},   // miss
		{0.0f, 0.0f, 0.0f, -4.0f, 1.0f},  // starts inside circle
		{0.0f, 2.0f, 0.0f, -6.0f, 0.25f}, // clipped fraction
	};
	const int count = (int)(sizeof(rays) / sizeof(rays[0]));
	for (int i = 0; i < count; ++i)
	{
		b2RayCastInput input;
		input.origin = (b2Vec2){rays[i][0], rays[i][1]};
		input.translation = (b2Vec2){rays[i][2], rays[i][3]};
		input.maxFraction = rays[i][4];

		b2CastOutput c = b2RayCastCircle(&input, &circle);
		printf("ray|circle|%d|", i);
		PrintNum(c.fraction); printf(" ");
		PrintNum(c.normal.x); printf(" ");
		PrintNum(c.normal.y); printf(" ");
		PrintNum(c.point.x); printf(" ");
		PrintNum(c.point.y); printf(" ");
		PrintNum(c.iterations); printf(" %d\n", c.hit ? 1 : 0);

		c = b2RayCastCapsule(&input, &capsule);
		printf("ray|capsule|%d|", i);
		PrintNum(c.fraction); printf(" ");
		PrintNum(c.normal.x); printf(" ");
		PrintNum(c.normal.y); printf(" ");
		PrintNum(c.point.x); printf(" ");
		PrintNum(c.point.y); printf(" ");
		PrintNum(c.iterations); printf(" %d\n", c.hit ? 1 : 0);

		c = b2RayCastSegment(&input, &segment, false);
		printf("ray|segment|%d|", i);
		PrintNum(c.fraction); printf(" ");
		PrintNum(c.normal.x); printf(" ");
		PrintNum(c.normal.y); printf(" ");
		PrintNum(c.point.x); printf(" ");
		PrintNum(c.point.y); printf(" ");
		PrintNum(c.iterations); printf(" %d\n", c.hit ? 1 : 0);

		c = b2RayCastPolygon(&input, &box);
		printf("ray|polygon|%d|", i);
		PrintNum(c.fraction); printf(" ");
		PrintNum(c.normal.x); printf(" ");
		PrintNum(c.normal.y); printf(" ");
		PrintNum(c.point.x); printf(" ");
		PrintNum(c.point.y); printf(" ");
		PrintNum(c.iterations); printf(" %d\n", c.hit ? 1 : 0);
	}
}

// ----------------------------------------------------------- shape casts ----

static void EmitShapeCasts(void)
{
	// Cast a small circle proxy downward onto shapes sitting at the origin.
	b2Vec2 castPoints[1] = {{0.0f, 0.0f}};

	const b2Circle circle = {{0.0f, 0.0f}, 1.0f};
	const b2Capsule capsule = {{-1.0f, 0.0f}, {1.0f, 0.0f}, 0.5f};
	const b2Polygon box = b2MakeBox(1.0f, 1.0f);
	const b2Segment segment = {{-2.0f, 0.0f}, {2.0f, 0.0f}};

	const float casts[][5] = {
		{0.0f, 4.0f, 0.0f, -8.0f, 1.0f},
		{1.5f, 4.0f, 0.0f, -8.0f, 1.0f}, // off to the side
		{0.0f, 0.5f, 0.0f, -4.0f, 1.0f}, // starts close
	};
	const int count = (int)(sizeof(casts) / sizeof(casts[0]));
	for (int i = 0; i < count; ++i)
	{
		b2ShapeCastInput input;
		input.proxy = b2MakeProxy(castPoints, 1, 0.25f);
		input.translation = (b2Vec2){casts[i][2], casts[i][3]};
		input.maxFraction = casts[i][4];
		input.canEncroach = false;

		b2ShapeCastInput moved = input;
		moved.proxy.points[0] = (b2Vec2){casts[i][0], casts[i][1]};

		b2CastOutput c = b2ShapeCastCircle(&moved, &circle);
		printf("shapecast|circle|%d|", i);
		PrintNum(c.fraction); printf(" ");
		PrintNum(c.normal.x); printf(" ");
		PrintNum(c.normal.y); printf(" ");
		PrintNum(c.point.x); printf(" ");
		PrintNum(c.point.y); printf(" %d\n", c.hit ? 1 : 0);

		c = b2ShapeCastCapsule(&moved, &capsule);
		printf("shapecast|capsule|%d|", i);
		PrintNum(c.fraction); printf(" ");
		PrintNum(c.normal.x); printf(" ");
		PrintNum(c.normal.y); printf(" ");
		PrintNum(c.point.x); printf(" ");
		PrintNum(c.point.y); printf(" %d\n", c.hit ? 1 : 0);

		c = b2ShapeCastPolygon(&moved, &box);
		printf("shapecast|polygon|%d|", i);
		PrintNum(c.fraction); printf(" ");
		PrintNum(c.normal.x); printf(" ");
		PrintNum(c.normal.y); printf(" ");
		PrintNum(c.point.x); printf(" ");
		PrintNum(c.point.y); printf(" %d\n", c.hit ? 1 : 0);

		c = b2ShapeCastSegment(&moved, &segment);
		printf("shapecast|segment|%d|", i);
		PrintNum(c.fraction); printf(" ");
		PrintNum(c.normal.x); printf(" ");
		PrintNum(c.normal.y); printf(" ");
		PrintNum(c.point.x); printf(" ");
		PrintNum(c.point.y); printf(" %d\n", c.hit ? 1 : 0);
	}
}

// -------------------------------------------------------------- manifolds ----

static void PrintManifold(const char* name, int index, const b2Manifold* manifold)
{
	printf("manifold|%s|%d|%d|", name, index, manifold->pointCount);
	PrintNum(manifold->normal.x); printf(" ");
	PrintNum(manifold->normal.y); printf(" ");
	for (int i = 0; i < 2; ++i)
	{
		if (i < manifold->pointCount)
		{
			const b2ManifoldPoint* p = &manifold->points[i];
			PrintNum(p->point.x); printf(" ");
			PrintNum(p->point.y); printf(" ");
			PrintNum(p->anchorA.x); printf(" ");
			PrintNum(p->anchorA.y); printf(" ");
			PrintNum(p->anchorB.x); printf(" ");
			PrintNum(p->anchorB.y); printf(" ");
			PrintNum(p->separation); printf(" ");
			printf("%u ", (unsigned)p->id);
			printf("%d ", p->persisted ? 1 : 0);
		}
		else
		{
			printf("0 0 0 0 0 0 0 0 0 ");
		}
	}
	printf("\n");
}

static void EmitManifolds(void)
{
	const b2Circle circle = {{0.0f, 0.0f}, 1.0f};
	const b2Capsule capsule = {{-1.0f, 0.0f}, {1.0f, 0.0f}, 0.5f};
	const b2Polygon boxA = b2MakeBox(1.0f, 1.0f);
	const b2Polygon boxB = b2MakeBox(0.5f, 0.5f);
	const b2Segment segment = {{-2.0f, 0.0f}, {2.0f, 0.0f}};
	b2ChainSegment chainSegment;
	chainSegment.segment.point1 = (b2Vec2){-2.0f, 0.0f};
	chainSegment.segment.point2 = (b2Vec2){2.0f, 0.0f};
	chainSegment.ghost1 = (b2Vec2){-3.0f, 0.0f};
	chainSegment.ghost2 = (b2Vec2){3.0f, 0.0f};
	chainSegment.chainId = 0;

	b2Transform xfA = {{0.0f, 0.0f}, b2Rot_identity};

	// Manifold cases: B offset by (dx, dy) with optional rotation.
	const float cases[][3] = {
		{0.0f, -0.5f, 0.0f},
		{0.75f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f},    // exact overlap
		{1.6f, 0.8f, 0.3f},
		{0.0f, 2.5f, 0.0f},    // separated
	};
	const int count = (int)(sizeof(cases) / sizeof(cases[0]));
	for (int i = 0; i < count; ++i)
	{
		b2Transform xfB = {{cases[i][0], cases[i][1]}, b2MakeRot(cases[i][2])};

		b2Manifold m = b2CollideCircles(&circle, xfA, &circle, xfB);
		PrintManifold("circles", i, &m);

		m = b2CollideCapsuleAndCircle(&capsule, xfA, &circle, xfB);
		PrintManifold("capsule_circle", i, &m);

		m = b2CollideSegmentAndCircle(&segment, xfA, &circle, xfB);
		PrintManifold("segment_circle", i, &m);

		m = b2CollidePolygonAndCircle(&boxA, xfA, &circle, xfB);
		PrintManifold("polygon_circle", i, &m);

		m = b2CollideCapsules(&capsule, xfA, &capsule, xfB);
		PrintManifold("capsules", i, &m);

		m = b2CollideSegmentAndCapsule(&segment, xfA, &capsule, xfB);
		PrintManifold("segment_capsule", i, &m);

		m = b2CollidePolygonAndCapsule(&boxA, xfA, &capsule, xfB);
		PrintManifold("polygon_capsule", i, &m);

		m = b2CollidePolygons(&boxA, xfA, &boxB, xfB);
		PrintManifold("polygons", i, &m);

		m = b2CollideSegmentAndPolygon(&segment, xfA, &boxB, xfB);
		PrintManifold("segment_polygon", i, &m);

		m = b2CollideChainSegmentAndCircle(&chainSegment, xfA, &circle, xfB);
		PrintManifold("chain_circle", i, &m);

		b2SimplexCache chainCache1;
		chainCache1.count = 0;
		m = b2CollideChainSegmentAndCapsule(&chainSegment, xfA, &capsule, xfB, &chainCache1);
		PrintManifold("chain_capsule", i, &m);

		b2SimplexCache chainCache2;
		chainCache2.count = 0;
		m = b2CollideChainSegmentAndPolygon(&chainSegment, xfA, &boxB, xfB, &chainCache2);
		PrintManifold("chain_polygon", i, &m);
	}
}

// ------------------------------------------------------------------ mass ----

static void EmitMass(void)
{
	const float densities[3] = {1.0f, 2.5f, 0.0f};

	const b2Circle circle = {{0.25f, -0.5f}, 1.5f};
	const b2Capsule capsule = {{-1.0f, 0.5f}, {2.0f, -0.5f}, 0.5f};
	b2Hull hull = b2ComputeHull(
		(b2Vec2[]){{0.0f, 0.0f}, {2.0f, 0.0f}, {2.5f, 2.0f}, {1.0f, 3.0f}, {-0.5f, 1.5f}}, 5);
	b2Polygon poly = b2MakePolygon(&hull, 0.1f);

	for (int i = 0; i < 3; ++i)
	{
		float density = densities[i];
		b2MassData m = b2ComputeCircleMass(&circle, density);
		printf("mass|circle|%d|", i);
		PrintNum(m.mass); printf(" ");
		PrintNum(m.center.x); printf(" ");
		PrintNum(m.center.y); printf(" ");
		PrintNum(m.rotationalInertia);
		printf("\n");

		m = b2ComputeCapsuleMass(&capsule, density);
		printf("mass|capsule|%d|", i);
		PrintNum(m.mass); printf(" ");
		PrintNum(m.center.x); printf(" ");
		PrintNum(m.center.y); printf(" ");
		PrintNum(m.rotationalInertia);
		printf("\n");

		m = b2ComputePolygonMass(&poly, density);
		printf("mass|polygon|%d|", i);
		PrintNum(m.mass); printf(" ");
		PrintNum(m.center.x); printf(" ");
		PrintNum(m.center.y); printf(" ");
		PrintNum(m.rotationalInertia);
		printf("\n");
	}
}

// ----------------------------------------------------------------- scenes ----

static void EmitScene(const char* name, int frameCount, int levels, int dumpBodyLimit)
{
	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = (b2Vec2){0.0f, -10.0f};
	b2WorldId worldId = b2CreateWorld(&worldDef);

	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.position = (b2Vec2){0.0f, -1.0f};
	b2BodyId groundId = b2CreateBody(worldId, &bodyDef);

	b2ShapeDef shapeDef = b2DefaultShapeDef();
	b2Polygon groundBox = b2MakeBox(100.0f, 1.0f);
	b2CreatePolygonShape(groundId, &shapeDef, &groundBox);

	int bodyCount = 0;
	b2BodyId bodyIds[64 * 64];
	float halfWidth = 0.5f;
	float heightSpacing = 1.0f;

	for (int level = 0; level < levels; ++level)
	{
		float y = 0.5f + 2.0f * halfWidth + level * heightSpacing;
		int boxes = levels - level;
		float xBase = -boxes * halfWidth;
		for (int i = 0; i < boxes; ++i)
		{
			bodyDef.position = (b2Vec2){xBase + (i + 0.5f), y};
			bodyDef.type = b2_dynamicBody;
			b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);
			b2Polygon box = b2MakeBox(halfWidth, halfWidth);
			b2CreatePolygonShape(bodyId, &shapeDef, &box);
			bodyIds[bodyCount++] = bodyId;
		}
	}

	for (int frame = 0; frame < frameCount; ++frame)
	{
		b2World_Step(worldId, 1.0f / 60.0f, 4);
	}

	b2BodyEvents events = b2World_GetBodyEvents(worldId);
	printf("scene|%s|bodies|%d|moved|%d\n", name, bodyCount, events.moveCount);

	int dumpCount = bodyCount < dumpBodyLimit ? bodyCount : dumpBodyLimit;
	for (int i = 0; i < dumpCount; ++i)
	{
		b2Vec2 p = b2Body_GetPosition(bodyIds[i]);
		float angle = b2Rot_GetAngle(b2Body_GetRotation(bodyIds[i]));
		bool awake = b2Body_IsAwake(bodyIds[i]);
		printf("scene|%s|body|%d|", name, i);
		PrintNum(p.x); printf(" ");
		PrintNum(p.y); printf(" ");
		PrintNum(angle); printf(" %d\n", awake ? 1 : 0);
	}

	b2DestroyWorld(worldId);
}

// ----------------------------------------------------------------- joints ----

// Distance-joint scene fixture: four deterministic distance-joint cases
// stepped with the upstream default of four substeps, exercising the rigid
// rope, the length limits, the soft spring, and the motor. Each case runs in
// its own world and dumps final body transforms plus joint measurements.
static void DumpJointBody(const char* name, int bodyIndex, b2BodyId bodyId)
{
	b2Vec2 p = b2Body_GetPosition(bodyId);
	float angle = b2Rot_GetAngle(b2Body_GetRotation(bodyId));
	bool awake = b2Body_IsAwake(bodyId);
	printf("joint|%s|body|%d|", name, bodyIndex);
	PrintNum(p.x); printf(" ");
	PrintNum(p.y); printf(" ");
	PrintNum(angle); printf(" %d\n", awake ? 1 : 0);
}

static void DumpJointJoint(const char* name, b2JointId jointId)
{
	b2Vec2 force = b2Joint_GetConstraintForce(jointId);
	float torque = b2Joint_GetConstraintTorque(jointId);
	float length = b2DistanceJoint_GetCurrentLength(jointId);
	float motorForce = b2DistanceJoint_GetMotorForce(jointId);
	printf("joint|%s|joint|", name);
	PrintNum(force.x); printf(" ");
	PrintNum(force.y); printf(" ");
	PrintNum(torque); printf(" ");
	PrintNum(length); printf(" ");
	PrintNum(motorForce); printf("\n");
}

static void EmitJointRope(void)
{
	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = (b2Vec2){0.0f, -10.0f};
	b2WorldId worldId = b2CreateWorld(&worldDef);

	b2BodyDef groundDef = b2DefaultBodyDef();
	groundDef.position = (b2Vec2){0.0f, -1.0f};
	b2BodyId groundId = b2CreateBody(worldId, &groundDef);
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	b2Polygon groundBox = b2MakeBox(50.0f, 1.0f);
	b2CreatePolygonShape(groundId, &shapeDef, &groundBox);

	b2BodyDef bodyDef = b2DefaultBodyDef();
	b2Polygon box = b2MakeBox(0.25f, 0.25f);
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = (b2Vec2){-1.0f, 6.0f};
	b2BodyId bodyA = b2CreateBody(worldId, &bodyDef);
	b2CreatePolygonShape(bodyA, &shapeDef, &box);
	bodyDef.position = (b2Vec2){1.0f, 6.0f};
	b2BodyId bodyB = b2CreateBody(worldId, &bodyDef);
	b2CreatePolygonShape(bodyB, &shapeDef, &box);

	b2DistanceJointDef jointDef = b2DefaultDistanceJointDef();
	jointDef.bodyIdA = bodyA;
	jointDef.bodyIdB = bodyB;
	jointDef.length = 2.0f;
	b2JointId jointId = b2CreateDistanceJoint(worldId, &jointDef);

	for (int frame = 0; frame < 90; ++frame)
	{
		b2World_Step(worldId, 1.0f / 60.0f, 4);
	}

	DumpJointBody("rope", 0, bodyA);
	DumpJointBody("rope", 1, bodyB);
	DumpJointJoint("rope", jointId);
	b2DestroyWorld(worldId);
}

static void EmitJointLimit(void)
{
	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = (b2Vec2){0.0f, -10.0f};
	b2WorldId worldId = b2CreateWorld(&worldDef);

	b2BodyDef anchorDef = b2DefaultBodyDef();
	anchorDef.position = (b2Vec2){0.0f, 8.0f};
	b2BodyId anchorId = b2CreateBody(worldId, &anchorDef);

	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = (b2Vec2){0.0f, 6.5f};
	bodyDef.linearVelocity = (b2Vec2){0.0f, -12.0f};
	b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	b2Polygon box = b2MakeBox(0.25f, 0.25f);
	b2CreatePolygonShape(bodyId, &shapeDef, &box);

	b2DistanceJointDef jointDef = b2DefaultDistanceJointDef();
	jointDef.bodyIdA = anchorId;
	jointDef.bodyIdB = bodyId;
	jointDef.length = 1.5f;
	jointDef.enableSpring = true;
	jointDef.hertz = 5.0f;
	jointDef.dampingRatio = 0.5f;
	jointDef.enableLimit = true;
	jointDef.minLength = 0.5f;
	jointDef.maxLength = 1.5f;
	b2JointId jointId = b2CreateDistanceJoint(worldId, &jointDef);

	for (int frame = 0; frame < 120; ++frame)
	{
		b2World_Step(worldId, 1.0f / 60.0f, 4);
	}

	DumpJointBody("limit", 0, bodyId);
	DumpJointJoint("limit", jointId);
	b2DestroyWorld(worldId);
}

static void EmitJointSpring(void)
{
	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = (b2Vec2){0.0f, -10.0f};
	b2WorldId worldId = b2CreateWorld(&worldDef);

	b2BodyDef anchorDef = b2DefaultBodyDef();
	anchorDef.position = (b2Vec2){0.0f, 8.0f};
	b2BodyId anchorId = b2CreateBody(worldId, &anchorDef);

	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = (b2Vec2){0.0f, 5.5f};
	b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	b2Polygon box = b2MakeBox(0.25f, 0.25f);
	b2CreatePolygonShape(bodyId, &shapeDef, &box);

	b2DistanceJointDef jointDef = b2DefaultDistanceJointDef();
	jointDef.bodyIdA = anchorId;
	jointDef.bodyIdB = bodyId;
	jointDef.length = 2.0f;
	jointDef.enableSpring = true;
	jointDef.hertz = 3.0f;
	jointDef.dampingRatio = 0.6f;
	b2JointId jointId = b2CreateDistanceJoint(worldId, &jointDef);

	for (int frame = 0; frame < 240; ++frame)
	{
		b2World_Step(worldId, 1.0f / 60.0f, 4);
	}

	DumpJointBody("spring", 0, bodyId);
	DumpJointJoint("spring", jointId);
	b2DestroyWorld(worldId);
}

static void EmitJointMotor(void)
{
	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = (b2Vec2){0.0f, -10.0f};
	b2WorldId worldId = b2CreateWorld(&worldDef);

	b2BodyDef anchorDef = b2DefaultBodyDef();
	anchorDef.position = (b2Vec2){0.0f, 8.0f};
	b2BodyId anchorId = b2CreateBody(worldId, &anchorDef);

	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = (b2Vec2){0.0f, 7.0f};
	b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	b2Polygon box = b2MakeBox(0.25f, 0.25f);
	b2CreatePolygonShape(bodyId, &shapeDef, &box);

	b2DistanceJointDef jointDef = b2DefaultDistanceJointDef();
	jointDef.bodyIdA = anchorId;
	jointDef.bodyIdB = bodyId;
	jointDef.length = 0.5f;
	jointDef.enableSpring = true;
	jointDef.hertz = 20.0f;
	jointDef.dampingRatio = 1.0f;
	jointDef.enableLimit = true;
	jointDef.minLength = 0.5f;
	jointDef.maxLength = 5.0f;
	jointDef.enableMotor = true;
	jointDef.motorSpeed = 2.0f;
	jointDef.maxMotorForce = 2000.0f;
	b2JointId jointId = b2CreateDistanceJoint(worldId, &jointDef);

	for (int frame = 0; frame < 240; ++frame)
	{
		b2World_Step(worldId, 1.0f / 60.0f, 4);
	}

	DumpJointBody("motor", 0, bodyId);
	DumpJointJoint("motor", jointId);
	b2DestroyWorld(worldId);
}

static void EmitJointDistance(void)
{
	EmitJointRope();
	EmitJointLimit();
	EmitJointSpring();
	EmitJointMotor();
}

// --------------------------------------------------------- mouse joints ----

// Mouse-joint scene fixture: three deterministic dragging cases stepped with
// the upstream default of four substeps. Every case anchors the joint at the
// body centre (target equals the initial body position) so the constraint
// stays a pure damped point-mass spring: offset-anchor targets make the body
// orbit the target in a chaotic limit cycle that no cross-float comparison
// can pin. The drag and stiff cases retarget after one second and wake the
// body explicitly, matching the upstream sample's drag flow, then re-converge
// and re-sleep. The weak case clamps at one newton below the 2.5 newton
// weight and falls at a constant 6 m/s^2 with the impulse clamp saturated.
static void DumpMouseJoint(const char* name, b2JointId jointId)
{
	b2Vec2 force = b2Joint_GetConstraintForce(jointId);
	float torque = b2Joint_GetConstraintTorque(jointId);
	printf("joint|%s|mouse|", name);
	PrintNum(force.x); printf(" ");
	PrintNum(force.y); printf(" ");
	PrintNum(torque); printf("\n");
}

static void EmitMouseDrag(void)
{
	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = (b2Vec2){0.0f, -10.0f};
	b2WorldId worldId = b2CreateWorld(&worldDef);

	b2BodyDef groundDef = b2DefaultBodyDef();
	groundDef.position = (b2Vec2){0.0f, -1.0f};
	b2BodyId groundId = b2CreateBody(worldId, &groundDef);
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	b2Polygon groundBox = b2MakeBox(50.0f, 1.0f);
	b2CreatePolygonShape(groundId, &shapeDef, &groundBox);

	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = (b2Vec2){0.0f, 5.0f};
	b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);
	b2Polygon box = b2MakeBox(0.25f, 0.25f);
	b2CreatePolygonShape(bodyId, &shapeDef, &box);

	b2MouseJointDef jointDef = b2DefaultMouseJointDef();
	jointDef.bodyIdA = groundId;
	jointDef.bodyIdB = bodyId;
	jointDef.target = (b2Vec2){0.0f, 5.0f};
	jointDef.hertz = 5.0f;
	jointDef.dampingRatio = 0.7f;
	jointDef.maxForce = 1000.0f;
	b2JointId jointId = b2CreateMouseJoint(worldId, &jointDef);

	for (int frame = 0; frame < 180; ++frame)
	{
		b2World_Step(worldId, 1.0f / 60.0f, 4);
		if (frame == 60)
		{
			// Exercise the runtime target setter mid-scene. Upstream does
			// not wake the body, so the sample's wake call is replicated.
			b2MouseJoint_SetTarget(jointId, (b2Vec2){2.0f, 5.5f});
			b2Body_SetAwake(bodyId, true);
		}
	}

	DumpJointBody("drag", 0, bodyId);
	DumpMouseJoint("drag", jointId);
	b2DestroyWorld(worldId);
}

static void EmitMouseWeak(void)
{
	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = (b2Vec2){0.0f, -10.0f};
	b2WorldId worldId = b2CreateWorld(&worldDef);

	b2BodyDef groundDef = b2DefaultBodyDef();
	groundDef.position = (b2Vec2){0.0f, -1.0f};
	b2BodyId groundId = b2CreateBody(worldId, &groundDef);
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	b2Polygon groundBox = b2MakeBox(50.0f, 1.0f);
	b2CreatePolygonShape(groundId, &shapeDef, &groundBox);

	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = (b2Vec2){0.0f, 5.0f};
	b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);
	b2Polygon box = b2MakeBox(0.25f, 0.25f);
	b2CreatePolygonShape(bodyId, &shapeDef, &box);

	// A one-newton maximum force cannot hold the 0.25 kg box (2.5 N
	// weight): the impulse clamp saturates and the body falls at a constant
	// 6 m/s^2. Contacts with the jointed ground are suppressed by the
	// default collideConnected = false, matching upstream.
	b2MouseJointDef jointDef = b2DefaultMouseJointDef();
	jointDef.bodyIdA = groundId;
	jointDef.bodyIdB = bodyId;
	jointDef.target = (b2Vec2){0.0f, 5.0f};
	jointDef.hertz = 5.0f;
	jointDef.dampingRatio = 0.7f;
	jointDef.maxForce = 1.0f;
	b2JointId jointId = b2CreateMouseJoint(worldId, &jointDef);

	for (int frame = 0; frame < 150; ++frame)
	{
		b2World_Step(worldId, 1.0f / 60.0f, 4);
	}

	DumpJointBody("weak", 0, bodyId);
	DumpMouseJoint("weak", jointId);
	b2DestroyWorld(worldId);
}

static void EmitMouseStiff(void)
{
	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = (b2Vec2){0.0f, -10.0f};
	b2WorldId worldId = b2CreateWorld(&worldDef);

	b2BodyDef groundDef = b2DefaultBodyDef();
	groundDef.position = (b2Vec2){0.0f, -1.0f};
	b2BodyId groundId = b2CreateBody(worldId, &groundDef);
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	b2Polygon groundBox = b2MakeBox(50.0f, 1.0f);
	b2CreatePolygonShape(groundId, &shapeDef, &groundBox);

	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = (b2Vec2){0.0f, 5.0f};
	b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);
	b2Polygon box = b2MakeBox(0.25f, 0.25f);
	b2CreatePolygonShape(bodyId, &shapeDef, &box);

	// A stiff, well-damped spring snaps the box to the retargeted pose
	// with a sub-millimetre sag.
	b2MouseJointDef jointDef = b2DefaultMouseJointDef();
	jointDef.bodyIdA = groundId;
	jointDef.bodyIdB = bodyId;
	jointDef.target = (b2Vec2){0.0f, 5.0f};
	jointDef.hertz = 20.0f;
	jointDef.dampingRatio = 1.0f;
	jointDef.maxForce = 5000.0f;
	b2JointId jointId = b2CreateMouseJoint(worldId, &jointDef);

	for (int frame = 0; frame < 180; ++frame)
	{
		b2World_Step(worldId, 1.0f / 60.0f, 4);
		if (frame == 60)
		{
			b2MouseJoint_SetTarget(jointId, (b2Vec2){2.0f, 5.5f});
			b2Body_SetAwake(bodyId, true);
		}
	}

	DumpJointBody("stiff", 0, bodyId);
	DumpMouseJoint("stiff", jointId);
	b2DestroyWorld(worldId);
}

static void EmitJointMouse(void)
{
	EmitMouseDrag();
	EmitMouseWeak();
	EmitMouseStiff();
}

// -------------------------------------------------------- motor joints ----

// Motor-joint scene fixture: three deterministic pose-driving cases stepped
// with the upstream default of four substeps. Every case hangs a 0.25 kg
// box off a static ground with the box driven toward a position and angle
// offset; each ends asleep at the offset pose, so the records are stable
// across float widths.
static void DumpMotorJoint(const char* name, b2JointId jointId)
{
	b2Vec2 force = b2Joint_GetConstraintForce(jointId);
	float torque = b2Joint_GetConstraintTorque(jointId);
	printf("joint|%s|motor|", name);
	PrintNum(force.x); printf(" ");
	PrintNum(force.y); printf(" ");
	PrintNum(torque); printf("\n");
}

static void EmitMotorPose(void)
{
	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = (b2Vec2){0.0f, -10.0f};
	b2WorldId worldId = b2CreateWorld(&worldDef);

	b2BodyDef groundDef = b2DefaultBodyDef();
	groundDef.position = (b2Vec2){0.0f, -1.0f};
	b2BodyId groundId = b2CreateBody(worldId, &groundDef);
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	b2Polygon groundBox = b2MakeBox(50.0f, 1.0f);
	b2CreatePolygonShape(groundId, &shapeDef, &groundBox);

	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = (b2Vec2){3.0f, 0.5f};
	b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);
	b2Polygon box = b2MakeBox(0.25f, 0.25f);
	b2CreatePolygonShape(bodyId, &shapeDef, &box);

	// Upstream default correction factor of 0.3 drives the box up to the
	// offset pose and rotates it to half a radian.
	b2MotorJointDef jointDef = b2DefaultMotorJointDef();
	jointDef.bodyIdA = groundId;
	jointDef.bodyIdB = bodyId;
	jointDef.linearOffset = (b2Vec2){3.0f, 2.0f};
	jointDef.angularOffset = 0.5f;
	jointDef.maxForce = 500.0f;
	jointDef.maxTorque = 200.0f;
	b2JointId jointId = b2CreateMotorJoint(worldId, &jointDef);

	for (int frame = 0; frame < 240; ++frame)
	{
		b2World_Step(worldId, 1.0f / 60.0f, 4);
	}

	DumpJointBody("pose", 0, bodyId);
	DumpMotorJoint("pose", jointId);
	b2DestroyWorld(worldId);
}

static void EmitMotorRetarget(void)
{
	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = (b2Vec2){0.0f, -10.0f};
	b2WorldId worldId = b2CreateWorld(&worldDef);

	b2BodyDef groundDef = b2DefaultBodyDef();
	groundDef.position = (b2Vec2){0.0f, -1.0f};
	b2BodyId groundId = b2CreateBody(worldId, &groundDef);
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	b2Polygon groundBox = b2MakeBox(50.0f, 1.0f);
	b2CreatePolygonShape(groundId, &shapeDef, &groundBox);

	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = (b2Vec2){3.0f, 0.5f};
	b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);
	b2Polygon box = b2MakeBox(0.25f, 0.25f);
	b2CreatePolygonShape(bodyId, &shapeDef, &box);

	// The box starts at its offset pose, sleeps, then a strong correction
	// factor retargets it one metre up and half a radian over via the
	// runtime setters with an explicit wake.
	b2MotorJointDef jointDef = b2DefaultMotorJointDef();
	jointDef.bodyIdA = groundId;
	jointDef.bodyIdB = bodyId;
	jointDef.linearOffset = (b2Vec2){3.0f, 1.5f};
	jointDef.angularOffset = 0.0f;
	jointDef.maxForce = 2000.0f;
	jointDef.maxTorque = 800.0f;
	jointDef.correctionFactor = 1.0f;
	b2JointId jointId = b2CreateMotorJoint(worldId, &jointDef);

	for (int frame = 0; frame < 240; ++frame)
	{
		b2World_Step(worldId, 1.0f / 60.0f, 4);
		if (frame == 60)
		{
			b2MotorJoint_SetLinearOffset(jointId, (b2Vec2){3.0f, 3.0f});
			b2MotorJoint_SetAngularOffset(jointId, 0.5f);
			b2Body_SetAwake(bodyId, true);
		}
	}

	DumpJointBody("retarget", 0, bodyId);
	DumpMotorJoint("retarget", jointId);
	b2DestroyWorld(worldId);
}

static void EmitMotorClamped(void)
{
	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = (b2Vec2){0.0f, -10.0f};
	b2WorldId worldId = b2CreateWorld(&worldDef);

	b2BodyDef groundDef = b2DefaultBodyDef();
	groundDef.position = (b2Vec2){0.0f, -1.0f};
	b2BodyId groundId = b2CreateBody(worldId, &groundDef);
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	b2Polygon groundBox = b2MakeBox(50.0f, 1.0f);
	b2CreatePolygonShape(groundId, &shapeDef, &groundBox);

	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = (b2Vec2){3.0f, 0.5f};
	b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);
	b2Polygon box = b2MakeBox(0.25f, 0.25f);
	b2CreatePolygonShape(bodyId, &shapeDef, &box);

	// A 3 N maximum force sits just above the 2.5 N weight: the linear
	// clamp saturates through the early transit and the box crawls up to
	// the pose, while a 1 N-m clamp limits the angular drive.
	b2MotorJointDef jointDef = b2DefaultMotorJointDef();
	jointDef.bodyIdA = groundId;
	jointDef.bodyIdB = bodyId;
	jointDef.linearOffset = (b2Vec2){3.0f, 2.0f};
	jointDef.angularOffset = 0.0f;
	jointDef.maxForce = 3.0f;
	jointDef.maxTorque = 1.0f;
	jointDef.correctionFactor = 1.0f;
	b2JointId jointId = b2CreateMotorJoint(worldId, &jointDef);

	for (int frame = 0; frame < 240; ++frame)
	{
		b2World_Step(worldId, 1.0f / 60.0f, 4);
	}

	DumpJointBody("clamped", 0, bodyId);
	DumpMotorJoint("clamped", jointId);
	b2DestroyWorld(worldId);
}

static void EmitMotorCases(void)
{
	EmitMotorPose();
	EmitMotorRetarget();
	EmitMotorClamped();
}

// ----------------------------------------------------- revolute joints ----

// Revolute-joint scene fixture: four deterministic hinge cases stepped with
// the upstream default of four substeps. Every case pins a 0.25 kg box to a
// static ground through a hinge at (2, -0.25) and ends asleep, so the
// records are stable across float widths. The joint line carries the final
// constraint force, torque, and joint angle.
static void DumpRevoluteJoint(const char* name, b2JointId jointId)
{
	b2Vec2 force = b2Joint_GetConstraintForce(jointId);
	float torque = b2Joint_GetConstraintTorque(jointId);
	float angle = b2RevoluteJoint_GetAngle(jointId);
	printf("joint|%s|revolute|", name);
	PrintNum(force.x); printf(" ");
	PrintNum(force.y); printf(" ");
	PrintNum(torque); printf(" ");
	PrintNum(angle); printf("\n");
}

static void EmitRevoluteScene(const char* name, float angularDamping, int kickAt60, void (*configure)(b2JointId jointId))
{
	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = (b2Vec2){0.0f, -10.0f};
	b2WorldId worldId = b2CreateWorld(&worldDef);

	b2BodyDef groundDef = b2DefaultBodyDef();
	b2BodyId groundId = b2CreateBody(worldId, &groundDef);

	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = (b2Vec2){2.0f, -0.5f};
	bodyDef.angularDamping = angularDamping;
	b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	b2Polygon box = b2MakeBox(0.25f, 0.25f);
	b2CreatePolygonShape(bodyId, &shapeDef, &box);

	b2RevoluteJointDef jointDef = b2DefaultRevoluteJointDef();
	jointDef.bodyIdA = groundId;
	jointDef.bodyIdB = bodyId;
	jointDef.localAnchorA = (b2Vec2){2.0f, -0.25f};
	jointDef.localAnchorB = (b2Vec2){0.0f, 0.25f};
	b2JointId jointId = b2CreateRevoluteJoint(worldId, &jointDef);
	configure(jointId);

	for (int frame = 0; frame < 240; ++frame)
	{
		b2World_Step(worldId, 1.0f / 60.0f, 4);
		if (frame == 60 && kickAt60)
		{
			b2Body_SetAngularVelocity(bodyId, 1.0f);
		}
	}

	DumpJointBody(name, 0, bodyId);
	DumpRevoluteJoint(name, jointId);
	b2DestroyWorld(worldId);
}

static void RevoluteConfigureNone(b2JointId jointId)
{
	(void)jointId;
}

static void RevoluteConfigureSpring(b2JointId jointId)
{
	// A damped 2 Hz spring drives the box toward 0.8 rad; gravity sags the
	// settled angle to about 0.59 rad.
	b2RevoluteJoint_EnableSpring(jointId, true);
	b2RevoluteJoint_SetSpringHertz(jointId, 2.0f);
	b2RevoluteJoint_SetSpringDampingRatio(jointId, 0.6f);
	b2RevoluteJoint_SetTargetAngle(jointId, 0.8f);
}

static void RevoluteConfigureBrake(b2JointId jointId)
{
	// A zero-speed motor acts as a torque-clamped brake on the kicked box.
	b2RevoluteJoint_EnableMotor(jointId, true);
	b2RevoluteJoint_SetMaxMotorTorque(jointId, 0.5f);
}

static void RevoluteConfigureLimits(b2JointId jointId)
{
	// The hanging equilibrium at angle zero sits below the lower limit, so
	// the limit holds the box at 0.5 rad against gravity.
	b2RevoluteJoint_EnableLimit(jointId, true);
	b2RevoluteJoint_SetLimits(jointId, 0.5f, 0.9f);
}

static void EmitRevoluteCases(void)
{
	EmitRevoluteScene("hinge", 5.0f, 1, RevoluteConfigureNone);
	EmitRevoluteScene("spring", 0.0f, 0, RevoluteConfigureSpring);
	EmitRevoluteScene("brake", 0.0f, 1, RevoluteConfigureBrake);
	EmitRevoluteScene("limits", 2.0f, 0, RevoluteConfigureLimits);
}

// ---------------------------------------------------- prismatic joints ----

// Prismatic-joint scene fixture: four deterministic slider cases stepped
// with the upstream default of four substeps. Every case pins a 0.25 kg
// box to a static ground through a slider anchored at (0, -0.5) and ends
// asleep, so the records are stable across float widths. The joint line
// carries the final constraint force, torque, and translation.
static void DumpPrismaticJoint(const char* name, b2JointId jointId)
{
	b2Vec2 force = b2Joint_GetConstraintForce(jointId);
	float torque = b2Joint_GetConstraintTorque(jointId);
	float translation = b2PrismaticJoint_GetTranslation(jointId);
	printf("joint|%s|prismatic|", name);
	PrintNum(force.x); printf(" ");
	PrintNum(force.y); printf(" ");
	PrintNum(torque); printf(" ");
	PrintNum(translation); printf("\n");
}

static void EmitPrismaticScene(const char* name, int tilted, int kickAt30, void (*configure)(b2JointId jointId))
{
	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = (b2Vec2){0.0f, -10.0f};
	b2WorldId worldId = b2CreateWorld(&worldDef);

	b2BodyDef groundDef = b2DefaultBodyDef();
	groundDef.position = (b2Vec2){0.0f, -1.0f};
	b2BodyId groundId = b2CreateBody(worldId, &groundDef);

	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = (b2Vec2){0.0f, -0.5f};
	b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	b2Polygon box = b2MakeBox(0.25f, 0.25f);
	b2CreatePolygonShape(bodyId, &shapeDef, &box);

	b2PrismaticJointDef jointDef = b2DefaultPrismaticJointDef();
	jointDef.bodyIdA = groundId;
	jointDef.bodyIdB = bodyId;
	jointDef.localAnchorA = (b2Vec2){0.0f, 0.5f};
	jointDef.localAnchorB = (b2Vec2){0.0f, 0.0f};
	if (tilted)
	{
		// A diagonal rail loads the axis with a gravity component.
		jointDef.localAxisA = (b2Vec2){1.0f, 1.0f};
	}
	b2JointId jointId = b2CreatePrismaticJoint(worldId, &jointDef);
	configure(jointId);

	for (int frame = 0; frame < 240; ++frame)
	{
		b2World_Step(worldId, 1.0f / 60.0f, 4);
		if (frame == 30 && kickAt30)
		{
			if (tilted)
			{
				b2Body_SetLinearVelocity(bodyId, (b2Vec2){0.0f, -2.0f});
			}
			else
			{
				b2Body_SetLinearVelocity(bodyId, (b2Vec2){1.5f, 0.0f});
			}
		}
	}

	DumpJointBody(name, 0, bodyId);
	DumpPrismaticJoint(name, jointId);
	b2DestroyWorld(worldId);
}

static void PrismaticConfigureLimits(b2JointId jointId)
{
	// The kicked box slides along the axis until the upper limit stops it.
	b2PrismaticJoint_EnableLimit(jointId, true);
	b2PrismaticJoint_SetLimits(jointId, -1.0f, 1.0f);
}

static void PrismaticConfigureTiltedLimits(b2JointId jointId)
{
	// The diagonal rail carries a gravity component, so the lower limit
	// holds the box against part of its weight.
	b2PrismaticJoint_EnableLimit(jointId, true);
	b2PrismaticJoint_SetLimits(jointId, -0.5f, 0.5f);
}

static void PrismaticConfigureSpring(b2JointId jointId)
{
	// A damped 3 Hz spring drives the box to half a metre along the axis;
	// gravity is perpendicular, so the settled translation matches the
	// target.
	b2PrismaticJoint_EnableSpring(jointId, true);
	b2PrismaticJoint_SetSpringHertz(jointId, 3.0f);
	b2PrismaticJoint_SetSpringDampingRatio(jointId, 0.7f);
	b2PrismaticJoint_SetTargetTranslation(jointId, 0.5f);
}

static void PrismaticConfigureBrake(b2JointId jointId)
{
	// A zero-speed motor acts as a force-clamped brake on the kicked box.
	b2PrismaticJoint_EnableMotor(jointId, true);
	b2PrismaticJoint_SetMaxMotorForce(jointId, 5.0f);
}

static void EmitPrismaticCases(void)
{
	EmitPrismaticScene("limits", 0, 1, PrismaticConfigureLimits);
	EmitPrismaticScene("tilted", 1, 0, PrismaticConfigureTiltedLimits);
	EmitPrismaticScene("spring", 0, 0, PrismaticConfigureSpring);
	EmitPrismaticScene("brake", 0, 1, PrismaticConfigureBrake);
}

// --------------------------------------------------------- weld joints ----

// Weld-joint scene fixture: two deterministic weld cases stepped with the
// upstream default of four substeps. Each case welds two 0.25 kg boxes
// together in the air and drops the pair onto a static ground; both cases
// settle asleep, so the records are stable across float widths. The rigid
// case topples and comes to rest; the soft case flexes and holds a bent
// pose under gravity.
static void EmitWeldScene(const char* name, float linearHertz, float angularHertz, float damping)
{
	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = (b2Vec2){0.0f, -10.0f};
	b2WorldId worldId = b2CreateWorld(&worldDef);

	b2BodyDef groundDef = b2DefaultBodyDef();
	b2BodyId groundId = b2CreateBody(worldId, &groundDef);
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	b2Polygon groundBox = b2MakeBox(50.0f, 0.5f);
	b2CreatePolygonShape(groundId, &shapeDef, &groundBox);

	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = (b2Vec2){0.0f, 3.0f};
	b2BodyId boxAId = b2CreateBody(worldId, &bodyDef);
	b2Polygon box = b2MakeBox(0.25f, 0.25f);
	b2CreatePolygonShape(boxAId, &shapeDef, &box);
	bodyDef.position = (b2Vec2){0.4f, 3.2f};
	b2BodyId boxBId = b2CreateBody(worldId, &bodyDef);
	b2CreatePolygonShape(boxBId, &shapeDef, &box);

	b2WeldJointDef jointDef = b2DefaultWeldJointDef();
	jointDef.bodyIdA = boxAId;
	jointDef.bodyIdB = boxBId;
	jointDef.localAnchorA = (b2Vec2){0.2f, 0.2f};
	jointDef.localAnchorB = (b2Vec2){-0.2f, -0.2f};
	jointDef.linearHertz = linearHertz;
	jointDef.angularHertz = angularHertz;
	jointDef.linearDampingRatio = damping;
	jointDef.angularDampingRatio = damping;
	b2JointId jointId = b2CreateWeldJoint(worldId, &jointDef);

	for (int frame = 0; frame < 300; ++frame)
	{
		b2World_Step(worldId, 1.0f / 60.0f, 4);
	}

	DumpJointBody(name, 0, boxAId);
	DumpJointBody(name, 1, boxBId);
	b2Vec2 force = b2Joint_GetConstraintForce(jointId);
	float torque = b2Joint_GetConstraintTorque(jointId);
	printf("joint|%s|weld|", name);
	PrintNum(force.x); printf(" ");
	PrintNum(force.y); printf(" ");
	PrintNum(torque); printf("\n");
	b2DestroyWorld(worldId);
}

static void EmitWeldCases(void)
{
	EmitWeldScene("rigid", 0.0f, 0.0f, 0.0f);
	EmitWeldScene("soft", 2.0f, 2.0f, 0.6f);
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "usage: fixture_gen <maths|hull|distance|raycast|shapecast|manifold|mass|scene_falling|scene_pyramid|scene_stack|joint_distance|joint_mouse|joint_motor|joint_revolute|joint_prismatic|joint_weld>\n");
		return 2;
	}

	const char* name = argv[1];
	if (strcmp(name, "maths") == 0) EmitMaths();
	else if (strcmp(name, "hull") == 0) EmitHull();
	else if (strcmp(name, "distance") == 0) EmitDistance();
	else if (strcmp(name, "raycast") == 0) EmitRayCasts();
	else if (strcmp(name, "shapecast") == 0) EmitShapeCasts();
	else if (strcmp(name, "manifold") == 0) EmitManifolds();
	else if (strcmp(name, "mass") == 0) EmitMass();
	else if (strcmp(name, "scene_falling") == 0) EmitScene("falling", 60, 1, 64);
	else if (strcmp(name, "scene_pyramid") == 0) EmitScene("pyramid", 120, 10, 64);
	else if (strcmp(name, "scene_stack") == 0) EmitScene("stack", 240, 8, 64);
	else if (strcmp(name, "joint_distance") == 0) EmitJointDistance();
	else if (strcmp(name, "joint_mouse") == 0) EmitJointMouse();
	else if (strcmp(name, "joint_motor") == 0) EmitMotorCases();
	else if (strcmp(name, "joint_revolute") == 0) EmitRevoluteCases();
	else if (strcmp(name, "joint_prismatic") == 0) EmitPrismaticCases();
	else if (strcmp(name, "joint_weld") == 0) EmitWeldCases();
	else
	{
		fprintf(stderr, "unknown fixture: %s\n", name);
		return 2;
	}
	return 0;
}
