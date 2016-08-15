#ifndef MANTID_CURVEFITTING_FLATBACKGROUNDTEST_H_
#define MANTID_CURVEFITTING_FLATBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/FlatBackground.h"

using Mantid::CurveFitting::Functions::FlatBackground;

class FlatBackgroundTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FlatBackgroundTest *createSuite() { return new FlatBackgroundTest(); }
  static void destroySuite(FlatBackgroundTest *suite) { delete suite; }

  void test_category() {
    FlatBackground cfn;
    cfn.initialize();

    std::vector<std::string> cats;
    TS_ASSERT_THROWS_NOTHING(cats = cfn.categories());
    TS_ASSERT_LESS_THAN_EQUALS(1, cats.size());
    TS_ASSERT_EQUALS(cats.front(), "Background");
    // This would enfonce one and only one category:
    // TS_ASSERT(cfn.category() == "Background");
  }

  void testZero() { checkFunctionValue(0); }

  void testFunctionMW() { checkFunctionValue(100); }

private:
  void checkFunctionValue(double val) {
    std::size_t numPoints = 100;
    std::vector<double> yValues(numPoints);

    FlatBackground bkgd;
    bkgd.initialize();
    bkgd.setParameter("A0", val);
    bkgd.function1D(yValues.data(), nullptr, numPoints); // don't need x-values

    for (size_t i = 0; i < numPoints; i++) {
      TS_ASSERT_EQUALS(yValues[i], val);
    }
  }
};

#endif /* MANTID_CURVEFITTING_FLATBACKGROUNDTEST_H_ */
