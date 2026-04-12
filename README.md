# ase-math

[![Layer](https://img.shields.io/badge/Layer-0%20Foundation-blue.svg)]()
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)]()
[![Header Only](https://img.shields.io/badge/Header-Only-green.svg)]()

> Mathematical primitives and algorithms for 3D planetary simulation

Part of [ASE - Antares Simulation Engine](../../..)

## Overview

`ase-math` provides foundational mathematical types and functions optimized for planetary-scale simulations where double-precision coordinates span millions of meters while local calculations use fast single-precision float. All components are header-only, constexpr-friendly, and designed for high performance in real-time physics, rendering, and procedural generation. The module provides Vec2, Vec3, Vec4, Mat3, Mat4, and Quat types with SIMD-friendly memory layout, replacing std::min, std::max, and std::clamp with ASE-specific implementations that are explicitly allowed in components (std:: math functions are forbidden in ECS code). Noise functions (Perlin, Simplex, multi-octave fractal) power the terrain generation pipeline, while interpolation utilities (lerp, slerp, smoothstep, ease curves) drive animation and transition systems. Trigonometric wrappers provide degree/radian conversion, fast approximations for physics ticks, and precise implementations for orbital calculations in the ephemeris module. As a Layer 0 foundation library with zero ASE dependencies, ase-math is imported by virtually every module in the engine — it is the mathematical vocabulary that all simulation code speaks.

## Features

### Core Types
- **Vec3**: 3D vector with arithmetic operations, dot/cross products, normalization
- **Quaternion**: Rotation representation without gimbal lock, axis-angle and Euler conversions, SLERP
- **Color3**: RGB color with arithmetic, luminance calculation, and interpolation
- **Spherical**: Coordinate system conversions (cartesian, spherical, RA/Dec for celestial bodies)

### Algorithms
- **Perlin Noise**: Deterministic 2D noise generation with fractal Brownian motion (fBm) for terrain
- **Interpolation**: Linear, smoothstep, smootherstep, and remap utilities

### Constants
- `PI`, `TWO_PI`, `HALF_PI` - Mathematical constants for trigonometry

## Installation

```cmake
# Add to your CMakeLists.txt
add_subdirectory(foundation/ase-math)
target_link_libraries(your_target PRIVATE ase-math)
```

Header-only library - include what you need:

```cpp
#include <ase/math/vec3.hpp>
#include <ase/math/quaternion.hpp>
#include <ase/math/color3.hpp>
#include <ase/math/perlin.hpp>
#include <ase/math/spherical.hpp>
#include <ase/math/interpolation.hpp>
```

## Usage

### Vec3 - Spatial Calculations

```cpp
using namespace ase::math;

// Creation
Vec3 pos{1.0f, 2.0f, 3.0f};
Vec3 velocity = Vec3::forward() * 10.0f;

// Operations
Vec3 result = pos + velocity * 0.016f;  // Physics step
float distance = pos.distance_to(Vec3::zero());

// Normalization
Vec3 direction = velocity.normalized();
Vec3 horizontal = velocity.normalized_xz();  // Flatten to XZ plane

// Products
float projection = velocity.dot(Vec3::up());
Vec3 perpendicular = Vec3::right().cross(Vec3::up());
```

### Quaternion - Rotation Without Gimbal Lock

```cpp
using namespace ase::math;

// Create from axis-angle
Quaternion rot = Quaternion::from_axis_angle(Vec3::up(), PI / 4.0f);

// Create from Euler angles (yaw, pitch, roll)
Quaternion camera = Quaternion::from_euler(0.0f, 0.1f, 0.0f);

// Rotate vectors
Vec3 rotated = rot.rotate(Vec3::forward());
Vec3 look_dir = camera.forward();
Vec3 right_dir = camera.right();

// Interpolation
Quaternion smooth = Quaternion::slerp(rot, camera, 0.5f);

// Combine rotations
Quaternion combined = rot * camera;
```

### Perlin Noise - Procedural Terrain

```cpp
using namespace ase::math;

// Create noise generator with seed
Perlin noise{12345};

// Sample noise at position
float height = noise.noise(x * 0.01f, z * 0.01f);  // Returns [-1, 1]

// Fractal Brownian Motion for terrain
float terrain = noise.fbm(
    x * 0.001f,
    z * 0.001f,
    6,      // octaves (layers of detail)
    0.5f,   // persistence (amplitude decay)
    2.0f    // lacunarity (frequency increase)
);

// Convert to world height
float world_y = terrain * 100.0f;  // Scale to [-100, 100]
```

### Color3 - RGB Operations

```cpp
using namespace ase::math;

// Creation
Color3 sky = Color3{0.5f, 0.7f, 1.0f};
Color3 sunset = Color3::lerp(Color3::blue(), Color3::red(), 0.3f);

// Operations
Color3 tinted = sky * Color3{1.0f, 0.9f, 0.8f};  // Component-wise multiply
Color3 bright = sky * 1.5f;  // Scalar multiply

// Luminance (perceived brightness)
float brightness = sky.luminance();

// Clamp to valid range
Color3 clamped = bright.clamped();  // [0, 1]
```

### Spherical Coordinates - Celestial Bodies

```cpp
using namespace ase::math;

// Convert spherical to cartesian (sun position)
Vec3 sun_pos = spherical_to_cartesian(
    0.0f,      // phi (azimuthal angle)
    PI / 4.0f, // theta (polar angle from +Y)
    150e9f     // radius (AU to meters)
);

// Convert cartesian to spherical
Spherical coords = cartesian_to_spherical(sun_pos);

// Celestial coordinates (stars)
Vec3 star_pos = ra_dec_to_cartesian(
    6.75f,  // Right Ascension (hours)
    -16.7f, // Declination (degrees)
    1000.0f // Distance
);

auto [ra, dec] = cartesian_to_ra_dec(star_pos);
```

### Interpolation - Smooth Transitions

```cpp
using namespace ase::math;

// Linear interpolation
float alpha = lerp(0.0f, 1.0f, 0.5f);  // 0.5

// Smooth curves
float smooth = smoothstep(0.3f);      // Hermite interpolation
float smoother = smootherstep(0.3f);  // Perlin's improved version

// Remap ranges
float normalized = remap(
    75.0f,   // value
    0.0f,    // input min
    100.0f,  // input max
    0.0f,    // output min
    1.0f     // output max
);  // 0.75

// Clamp to [0, 1]
float clamped = saturate(1.5f);  // 1.0
```

## API Reference

### Vec3

| Member | Description |
|--------|-------------|
| `Vec3(x, y, z)` | Constructor |
| `operator+, -, *, /` | Arithmetic operations |
| `dot(other)` | Dot product |
| `cross(other)` | Cross product |
| `length()` | Euclidean length |
| `length_squared()` | Squared length (faster) |
| `length_xz()` | Horizontal distance (Y=0) |
| `normalized()` | Unit vector |
| `normalized_xz()` | Normalized in XZ plane |
| `distance_to(other)` | Distance to another point |
| `zero(), one(), up(), forward(), right()` | Static constructors |

### Quaternion

| Member | Description |
|--------|-------------|
| `Quaternion(w, x, y, z)` | Constructor |
| `identity()` | No rotation (static) |
| `from_axis_angle(axis, angle)` | Create from rotation axis and angle |
| `from_euler(yaw, pitch, roll)` | Create from Euler angles |
| `operator*(q)` | Combine rotations |
| `rotate(v)` | Rotate a vector |
| `conjugate()` | Inverse for unit quaternions |
| `normalized()` | Convert to unit quaternion |
| `slerp(a, b, t)` | Spherical linear interpolation |
| `forward(), right(), up()` | Extract basis vectors |
| `yaw(), pitch()` | Extract Euler angles |

### Color3

| Member | Description |
|--------|-------------|
| `Color3(r, g, b)` | Constructor |
| `operator+, -, *, /` | Arithmetic operations |
| `clamped()` | Clamp to [0, 1] |
| `luminance()` | Perceived brightness |
| `lerp(a, b, t)` | Linear interpolation |
| `black(), white(), red(), green(), blue()` | Color presets |

### Perlin

| Member | Description |
|--------|-------------|
| `Perlin(seed)` | Constructor with seed |
| `noise(x, y)` | 2D Perlin noise [-1, 1] |
| `fbm(x, y, octaves, persistence, lacunarity)` | Fractal Brownian Motion |

### Spherical

| Function | Description |
|----------|-------------|
| `spherical_to_cartesian(phi, theta, radius)` | Convert to Vec3 |
| `cartesian_to_spherical(v)` | Convert from Vec3 |
| `ra_dec_to_cartesian(ra, dec, radius)` | Celestial coordinates to Vec3 |
| `cartesian_to_ra_dec(v)` | Vec3 to celestial coordinates |

### Interpolation

| Function | Description |
|----------|-------------|
| `lerp(a, b, t)` | Linear interpolation |
| `saturate(t)` | Clamp to [0, 1] |
| `smoothstep(t)` | Hermite interpolation |
| `smootherstep(t)` | Perlin's improved smoothstep |
| `remap(value, in_min, in_max, out_min, out_max)` | Map between ranges |

## Design Philosophy

### Header-Only
All implementations are in headers for optimal inlining and compile-time optimization.

### Constexpr
Most operations are `constexpr` for compile-time computation where possible.

### Performance
- No dynamic allocation
- SIMD-friendly data layout (future optimization)
- Minimal branching in hot paths
- Squared length operations to avoid `sqrt()` when possible

### Layer 0 Compliance
- Zero dependencies on other ASE modules
- No ECS types (pure math)
- No external libraries (STL only)

## Dependencies

### External
- C++20 standard library (`<cmath>`, `<array>`, `<algorithm>`)

### Internal
- None (Layer 0 - Foundation)

## License

Proprietary - ASE Engine

---

**Layer 0 Foundation** | No ASE dependencies | Header-only | C++20
