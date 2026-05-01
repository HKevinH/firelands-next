#pragma once

// 3x3 rotation matrix type for the vmap pipeline.
// Replaces G3D::Matrix3 with a minimal, self-contained implementation that
// matches only the operations actually used in ModelInstance:
//
//   fromEulerAnglesZYX(ry, rx, rz)   — builds rotation from Euler angles
//   inverse()                         — for rotation matrices == transpose
//   operator*(Vec3)                   — transforms a point
//   operator*(const Mat3&)            — concatenates two rotations

#include "Vec3.h"
#include <cmath>

namespace Firelands::VMap {

struct Mat3 {
    // Row-major storage: m[row][col]
    float m[3][3]{};

    Mat3() { m[0][0] = m[1][1] = m[2][2] = 1.f; }

    // Build from column-vectors (same convention as G3D).
    static Mat3 FromCols(Vec3 c0, Vec3 c1, Vec3 c2) {
        Mat3 r;
        for (int i = 0; i < 3; ++i) {
            r.m[i][0] = (&c0.x)[i];
            r.m[i][1] = (&c1.x)[i];
            r.m[i][2] = (&c2.x)[i];
        }
        return r;
    }

    // Matches G3D::Matrix3::fromEulerAnglesZYX(yaw, pitch, roll).
    // Euler convention: applied Z then Y then X (ZYX intrinsic).
    static Mat3 FromEulerAnglesZYX(float yaw, float pitch, float roll) {
        float cy = std::cos(yaw),   sy = std::sin(yaw);
        float cp = std::cos(pitch), sp = std::sin(pitch);
        float cr = std::cos(roll),  sr = std::sin(roll);

        // R = Rz(yaw) * Ry(pitch) * Rx(roll)
        Mat3 r;
        r.m[0][0] =  cy*cp;
        r.m[0][1] =  cy*sp*sr - sy*cr;
        r.m[0][2] =  cy*sp*cr + sy*sr;

        r.m[1][0] =  sy*cp;
        r.m[1][1] =  sy*sp*sr + cy*cr;
        r.m[1][2] =  sy*sp*cr - cy*sr;

        r.m[2][0] = -sp;
        r.m[2][1] =  cp*sr;
        r.m[2][2] =  cp*cr;
        return r;
    }

    // For rotation matrices the inverse is the transpose.
    Mat3 inverse() const {
        Mat3 r;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                r.m[i][j] = m[j][i];
        return r;
    }

    // Matrix * vector (right-multiply column vector).
    Vec3 operator*(Vec3 const& v) const {
        return {
            m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z,
            m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z,
            m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z
        };
    }

    // Matrix * matrix.
    Mat3 operator*(Mat3 const& o) const {
        Mat3 r;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) {
                r.m[i][j] = 0;
                for (int k = 0; k < 3; ++k)
                    r.m[i][j] += m[i][k] * o.m[k][j];
            }
        return r;
    }

    // vec * Mat  (left-multiply row vector) — used in ModelInstance world-Z calc.
    friend Vec3 operator*(Vec3 const& v, Mat3 const& mat) {
        return {
            v.x*mat.m[0][0] + v.y*mat.m[1][0] + v.z*mat.m[2][0],
            v.x*mat.m[0][1] + v.y*mat.m[1][1] + v.z*mat.m[2][1],
            v.x*mat.m[0][2] + v.y*mat.m[1][2] + v.z*mat.m[2][2]
        };
    }
};

} // namespace Firelands::VMap
