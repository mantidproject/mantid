#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/Meier.h"
#include <array>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;
using namespace Mantid::CurveFitting::Algorithms;

class MeierTest : public CxxTest::TestSuite {
public:
  void testAgainstMockData() {
    MeierV2 meier;
    meier.initialize();

    meier.setParameter("A0", 0.5);
    meier.setParameter("FreqD", 0.01);
    meier.setParameter("FreqQ", 0.05);
    meier.setParameter("Lambda", 0.1);
    meier.setParameter("Sigma", 0.2);

    std::array<double, 4> xValues = {0.0, 4.0, 8.0, 12.0};
    std::array<double, 4> expected = {0.5, 0.0920992725837422, 0.0023798684614228663, 0.0007490849206591537};
    std::array<double, 4> yValues;

    meier.function1D(yValues.data(), xValues.data(), 4);

    for (size_t i = 0; i < 4; i++) {
      TS_ASSERT_DELTA(yValues[i], expected[i], 1e-5);
    }
  }
};
