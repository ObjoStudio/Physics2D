# Physics2D

Physics2D is a planned native Objo port of the Box2D 3.1.1 rigid-body physics
engine. It will be delivered as one dependency-free `Physics2D` module that can
be added to Objo Studio projects and imported with:

```objo
Import Physics2D
```

The public API will use Objo's built-in `Vector2` and `Matrix` maths types and
an idiomatic object-oriented façade. Internally, the solver will use benchmarked
data-oriented storage for the best practical performance in native Objo.

The implementation will include a complete automated test suite, reproducible
performance benchmarks, extensive teaching documentation, and an interactive
desktop demo application.

See [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) for the staged porting plan
and [AGENTS.md](AGENTS.md) for the repository's implementation rules.
