# ase-math

**ASE Math Library** - Foundation for spatial calculations

## Features

- ✅ **Header-only** - No compilation required
- ✅ **Vec3** - 3D vectors with SIMD-friendly layout
- ✅ **constexpr** - Compile-time calculations where possible
- 🚧 **Mat4** - 4x4 matrices (planned)
- 🚧 **Quaternion** - Rotations (planned)
- 🚧 **Noise** - Perlin/Simplex noise (planned)

## Usage

```cpp
#include <ase/math/vec3.hpp>

using namespace ase::math;

Vec3 a{1, 2, 3};
Vec3 b{4, 5, 6};

Vec3 c = a + b;              // {5, 7, 9}
float d = dot(a, b);         // 32
Vec3 e = cross(a, b);        // {-3, 6, -3}
Vec3 f = normalize(a);       // Unit vector
```

## Layer

**Foundation (Layer 0)** - No dependencies

## License

TBD
