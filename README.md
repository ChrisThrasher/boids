# boids

SFML-based flocking simulation program

![boids](docs/boids.png)

# Requirements
 * C++17
 * CMake 3.16
 * SFML 2.5.1

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
