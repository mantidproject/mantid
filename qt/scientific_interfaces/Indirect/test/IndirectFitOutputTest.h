#ifndef MANTID_INDIRECTFITOUTPUTTEST_H_
#define MANTID_INDIRECTFITOUTPUTTEST_H_

#include <cxxtest/TestSuite.h>

#include "IndirectFitOutput.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

#include <iostream>

using namespace Mantid::IndirectFitDataCreationHelper;

class IndirectFitOutputTest : public CxxTest::TestSuite {
public:
  static IndirectFitOutputTest *createSuite() {
    return new IndirectFitOutputTest();
  }

  static void destroySuite(IndirectFitOutputTest *suite) { delete suite; }

  void
  test_that_isSpectrumFit_returns_false_if_the_spectrum_has_not_been_previously_fit() {
  }

  void
  test_that_isSpectrumFit_returns_true_if_the_spectrum_has_been_previously_fit() {
  }

  void test_test() { std::cout << "Hello"; }
};

#endif
