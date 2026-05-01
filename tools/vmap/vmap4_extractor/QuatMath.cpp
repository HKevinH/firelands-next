#include "QuatMath.h"

#include <cmath>

namespace Firelands::VMap::Vmap4 {

Mat3 Mat3FromQuaternion(float x, float y, float z, float w) {
    // Normalise (same as G3D Quat::unitize before Matrix3(Quat))
    float len = std::sqrt(x*x + y*y + z*z + w*w);
    if (len > 1e-8f) {
        x /= len; y /= len; z /= len; w /= len;
    }
    float xx = 2.f * x * x;
    float xy = 2.f * x * y;
    float xz = 2.f * x * z;
    float xw = 2.f * x * w;

    float yy = 2.f * y * y;
    float yz = 2.f * y * z;
    float yw = 2.f * y * w;

    float zz = 2.f * z * z;
    float zw = 2.f * z * w;

    Mat3 r;
    r.m[0][0] = 1.f - yy - zz; r.m[0][1] = xy - zw;     r.m[0][2] = xz + yw;
    r.m[1][0] = xy + zw;     r.m[1][1] = 1.f - xx - zz; r.m[1][2] = yz - xw;
    r.m[2][0] = xz - yw;     r.m[2][1] = yz + xw;     r.m[2][2] = 1.f - xx - yy;
    return r;
}

void Mat3ToEulerAnglesXYZ_G3D(const Mat3& m, float& outZ, float& outX, float& outY) {
    constexpr float kHalfPi = 1.57079632679489661923f;
    float rfXAngle{}, rfYAngle{}, rfZAngle{};

    if (m.m[0][2] < 1.f) {
        if (m.m[0][2] > -1.f) {
            rfXAngle = std::atan2(-m.m[1][2], m.m[2][2]);
            rfYAngle  = std::asin(m.m[0][2]);
            rfZAngle  = std::atan2(-m.m[0][1], m.m[0][0]);
        } else {
            rfXAngle = -std::atan2(m.m[1][0], m.m[1][1]);
            rfYAngle = -kHalfPi;
            rfZAngle = 0.f;
        }
    } else {
        rfXAngle = std::atan2(m.m[1][0], m.m[1][1]);
        rfYAngle  = kHalfPi;
        rfZAngle  = 0.f;
    }
    outZ = rfXAngle;
    outX = rfYAngle;
    outY = rfZAngle;
}

} // namespace Firelands::VMap::Vmap4
