#ifndef MANTID_WORKFLOWALGORITHMS_IMUONASYMMETRYCALCULATORTEST_H_
#define MANTID_WORKFLOWALGORITHMS_IMUONASYMMETRYCALCULATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidWorkflowAlgorithms/IMuonAsymmetryCalculator.h"

using Mantid::WorkflowAlgorithms::IMuonAsymmetryCalculator;

class IMuonAsymmetryCalculatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IMuonAsymmetryCalculatorTest *createSuite() { return new IMuonAsymmetryCalculatorTest(); }
  static void destroySuite( IMuonAsymmetryCalculatorTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_WORKFLOWALGORITHMS_IMUONASYMMETRYCALCULATORTEST_H_ */