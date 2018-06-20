#ifndef MANTID_ALGORITHMS_MAXENTTRANSFORMMULTIFOURIERTEST_H_
#define MANTID_ALGORITHMS_MAXENTTRANSFORMMULTIFOURIERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/MaxEnt/MaxentSpaceComplex.h"
#include "MantidAlgorithms/MaxEnt/MaxentSpaceReal.h"
#include "MantidAlgorithms/MaxEnt/MaxentTransformMultiFourier.h"
#include <cmath>
#include <memory>

using Mantid::Algorithms::MaxentSpaceComplex;
using Mantid::Algorithms::MaxentSpaceReal;
using Mantid::Algorithms::MaxentTransformFourier;
using Mantid::Algorithms::MaxentTransformMultiFourier;

using MaxentSpace_sptr = std::shared_ptr<Mantid::Algorithms::MaxentSpace>;
using MaxentSpaceComplex_sptr =
    std::shared_ptr<Mantid::Algorithms::MaxentSpaceComplex>;
using MaxentSpaceReal_sptr =
    std::shared_ptr<Mantid::Algorithms::MaxentSpaceReal>;

class MaxentTransformMultiFourierTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaxentTransformMultiFourierTest *createSuite() {
    return new MaxentTransformMultiFourierTest();
  }
  static void destroySuite(MaxentTransformMultiFourierTest *suite) { delete suite; }

  void test_nothing_yet() {

  }

};

#endif /* MANTID_ALGORITHMS_MAXENTTRANSFORMMULTIFOURIERTEST_H_ */