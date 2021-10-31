# boids

SFML-based flocking simulation program

![boids](docs/boids.png)

# Requirements
 * C++20
 * CMake 3.16
 * SFML 2.5

# Building

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

# Controls

| Action            | Control         |
| ----------------- | --------------- |
| Select boid       | Left click      |
| Select parameter  | A, C, S, R, G   |
| Change parameter  | Up, Down        |
| Reset             | Space           |
