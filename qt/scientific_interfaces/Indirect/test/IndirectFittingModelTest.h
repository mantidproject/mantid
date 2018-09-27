#ifndef MANTID_INDIRECTFITTINGMODELTEST_H_
#define MANTID_INDIRECTFITTINGMODELTEST_H_

#include <cxxtest/TestSuite.h>

#include <iostream>

class IndirectFittingModelTest : public CxxTest::TestSuite {
public:
  static IndirectFittingModelTest *createSuite() {
    return new IndirectFittingModelTest();
  }

  static void destroySuite(IndirectFittingModelTest *suite) { delete suite; }

  void test_test() { std::cout << "here"; }
};

#endif
