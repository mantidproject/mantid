#ifndef ABRAGAMTEST_H_
#define ABRAGAMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/Abragam.h"

using namespace Mantid::CurveFitting::Functions;

class AbragamTest : public CxxTest::TestSuite {
public:
  void test_category() {

    Abragam ab;

    // check its categories
    TS_ASSERT(ab.categories().size() == 1);
    TS_ASSERT(ab.category() == "Muon");
  }

  void test_values() {

    // TODO
  }
};

#endif /*ABRAGAMTEST_H_*/
