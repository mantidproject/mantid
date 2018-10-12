#ifndef MANTID_INDIRECTFITOUTPUTTEST_H_
#define MANTID_INDIRECTFITOUTPUTTEST_H_

#include <cxxtest/TestSuite.h>

#include "IndirectFitOutput.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <iostream>

class IndirectFitOutputTest : public CxxTest::TestSuite {
public:
  static IndirectFitOutputTest *createSuite() {
    return new IndirectFitOutputTest();
  }

  static void destroySuite(IndirectFitOutputTest *suite) { delete suite; }

  void test_test() { std::cout << "Hello"; }
};

#endif
