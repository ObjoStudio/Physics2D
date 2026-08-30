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

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "usage: fixture_gen <maths|hull|distance|raycast|shapecast|manifold|mass|scene_falling|scene_pyramid|scene_stack>\n");
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
	else
	{
		fprintf(stderr, "unknown fixture: %s\n", name);
		return 2;
	}
	return 0;
}
