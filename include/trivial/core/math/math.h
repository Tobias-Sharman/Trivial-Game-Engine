#ifndef TRIVIAL_CORE_MATH_MATH_H
#define TRIVIAL_CORE_MATH_MATH_H

#include <trivial/core/math/affine2.h>
#include <trivial/core/math/angle.h>
#include <trivial/core/math/concepts.h>
#include <trivial/core/math/constants.h>
#include <trivial/core/math/mat4.h>
#include <trivial/core/math/transform2.h>
#include <trivial/core/math/vec2.h>
#include <trivial/core/math/vec3.h>
#include <trivial/core/math/vec4.h>

// TODO: Evaluate explicit fma usage in hot paths
//       Test on windows and linux for any issues
//       More SIMD backing to functions
//       More matrix functions, in particular inverse
//       Similarly review all classes for missing support
//       Extend SIMD backing to more than just float
//       Mat3 class
//       Math helpers
//       Graphics related classes
//       Maybe some geometry classes
//       Test against glm
//       Maybe have batch functions here as a general case for use in simulation

#endif // TRIVIAL_CORE_MATH_MATH_H
