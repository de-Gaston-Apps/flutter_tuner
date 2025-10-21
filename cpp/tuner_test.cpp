#include "gtest/gtest.h"
#include "tuner.h"

TEST(TunerTest, ParabolicInterpolation_DivisionByZero) {
    TunerCPP tuner(44100, 1024);
    std::vector<double> y = {1.0, 2.0, 3.0};
    std::vector<double> result = tuner.parabolic(y, 1);
    ASSERT_NEAR(result.at(0), 1.0, 1e-6);
    ASSERT_NEAR(result.at(1), 2.0, 1e-6);
}
