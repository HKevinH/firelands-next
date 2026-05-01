#include <gtest/gtest.h>

#include "Mat3.h"
#include "QuatMath.h"
#include "Vec3.h"
#include "VMapMagic.h"

#include <cstring>

using namespace Firelands::VMap;
using namespace Firelands::VMap::Vmap4;

TEST(QuatMathTests, IdentityQuaternion) {
    Mat3 m = Mat3FromQuaternion(0.f, 0.f, 0.f, 1.f);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            float expected = (i == j) ? 1.f : 0.f;
            EXPECT_NEAR(m.m[i][j], expected, 1e-5f) << " at [" << i << "][" << j << "]";
        }
    }
}

TEST(QuatMathTests, NonUnitQuaternionNormalizes) {
    Mat3 m = Mat3FromQuaternion(0.f, 0.f, 0.f, 10.f);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            float expected = (i == j) ? 1.f : 0.f;
            EXPECT_NEAR(m.m[i][j], expected, 1e-4f);
        }
    }
}

TEST(QuatMathTests, DegToRadHalfTurn) {
    EXPECT_NEAR(DegToRad(180.f), FPi(), 1e-5f);
}

TEST(QuatMathTests, RadToDegPi) {
    EXPECT_NEAR(RadToDeg(FPi()), 180.f, 1e-4f);
}

TEST(QuatMathTests, EulerXYZIdentity) {
    Mat3 id;
    float z{}, x{}, y{};
    Mat3ToEulerAnglesXYZ_G3D(id, z, x, y);
    EXPECT_NEAR(z, 0.f, 1e-5f);
    EXPECT_NEAR(x, 0.f, 1e-5f);
    EXPECT_NEAR(y, 0.f, 1e-5f);
}

TEST(VMapMagicTests, RawMagicWrittenSize) {
    EXPECT_EQ(sizeof(kRawVmapMagic), 8u);
    EXPECT_EQ(std::strlen(kRawVmapMagic), 7u);
}
