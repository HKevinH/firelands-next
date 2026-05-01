#include <gtest/gtest.h>

#include "Vec3.h"
#include "Mat3.h"

#include <cmath>

using namespace Firelands::VMap;

// ─── Vec3 ─────────────────────────────────────────────────────────────────────

TEST(Vec3Tests, BasicArithmetic) {
    Vec3 a{1, 2, 3}, b{4, 5, 6};
    Vec3 sum = a + b;
    EXPECT_FLOAT_EQ(sum.x, 5.f);
    EXPECT_FLOAT_EQ(sum.y, 7.f);
    EXPECT_FLOAT_EQ(sum.z, 9.f);

    Vec3 diff = b - a;
    EXPECT_FLOAT_EQ(diff.x, 3.f);

    float dot = a * b;
    EXPECT_FLOAT_EQ(dot, 1*4 + 2*5 + 3*6);
}

TEST(Vec3Tests, Cross) {
    Vec3 x{1,0,0}, y{0,1,0};
    Vec3 z = x % y;
    EXPECT_FLOAT_EQ(z.x, 0.f);
    EXPECT_FLOAT_EQ(z.y, 0.f);
    EXPECT_FLOAT_EQ(z.z, 1.f);
}

TEST(Vec3Tests, Normalize) {
    Vec3 v{3, 0, 0};
    v.normalize();
    EXPECT_FLOAT_EQ(v.length(), 1.f);
    EXPECT_FLOAT_EQ(v.x, 1.f);
}

TEST(Vec3Tests, IndexAccess) {
    Vec3 v{7, 8, 9};
    EXPECT_FLOAT_EQ(v[0], 7.f);
    EXPECT_FLOAT_EQ(v[1], 8.f);
    EXPECT_FLOAT_EQ(v[2], 9.f);
}

TEST(Vec3Tests, PrimaryAxis) {
    EXPECT_EQ(Vec3(5, 1, 1).primaryAxis(), 0);
    EXPECT_EQ(Vec3(1, 5, 1).primaryAxis(), 1);
    EXPECT_EQ(Vec3(1, 1, 5).primaryAxis(), 2);
}

// ─── AaBox3 ───────────────────────────────────────────────────────────────────

TEST(AaBox3Tests, Contains) {
    AaBox3 box{{0,0,0},{10,10,10}};
    EXPECT_TRUE(box.contains({5,5,5}));
    EXPECT_FALSE(box.contains({11,5,5}));
    EXPECT_FALSE(box.contains({-1,5,5}));
}

TEST(AaBox3Tests, Merge) {
    AaBox3 a{{0,0,0},{5,5,5}};
    AaBox3 b{{3,3,3},{10,10,10}};
    a.merge(b);
    EXPECT_FLOAT_EQ(a.lo.x, 0.f);
    EXPECT_FLOAT_EQ(a.hi.x, 10.f);
}

// ─── Mat3 ────────────────────────────────────────────────────────────────────

TEST(Mat3Tests, Identity) {
    Mat3 I;
    Vec3 v{1, 2, 3};
    Vec3 result = I * v;
    EXPECT_FLOAT_EQ(result.x, 1.f);
    EXPECT_FLOAT_EQ(result.y, 2.f);
    EXPECT_FLOAT_EQ(result.z, 3.f);
}

TEST(Mat3Tests, InverseIsTranspose) {
    // A 90° rotation around Z: x→y, y→-x
    float yaw = FPi() / 2.f;
    Mat3 R = Mat3::FromEulerAnglesZYX(yaw, 0.f, 0.f);
    Mat3 Rinv = R.inverse();
    Mat3 prod = R * Rinv;

    // prod should be close to identity
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            float expected = (i == j) ? 1.f : 0.f;
            EXPECT_NEAR(prod.m[i][j], expected, 1e-5f)
                << "at [" << i << "][" << j << "]";
        }
}

TEST(Mat3Tests, EulerRotationZ90) {
    // 90° around Z: transforms (1,0,0) → (0,1,0)
    Mat3 R = Mat3::FromEulerAnglesZYX(FPi() / 2.f, 0.f, 0.f);
    Vec3 v = R * Vec3{1, 0, 0};
    EXPECT_NEAR(v.x, 0.f, 1e-5f);
    EXPECT_NEAR(v.y, 1.f, 1e-5f);
    EXPECT_NEAR(v.z, 0.f, 1e-5f);
}

TEST(Mat3Tests, BitCastHelpers) {
    float f = 3.14f;
    uint32_t bits = FloatToRawBits(f);
    float back = RawBitsToFloat(bits);
    EXPECT_FLOAT_EQ(back, f);
}
