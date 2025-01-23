# boids

SFML-based flocking simulation program

![boids](docs/boids.png)

# Requirements
 * C++20
 * CMake 3.28

# Building & Running

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target run
```

# Controls

| Action            | Control         |
| ----------------- | --------------- |
| Select boid       | Left click      |
| Select parameter  | A, C, S, R, G   |
| Change parameter  | Up, Down        |
| Reset             | Space           |
