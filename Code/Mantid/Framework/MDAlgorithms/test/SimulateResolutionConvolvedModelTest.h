#ifndef MANTID_MDALGORITHMS_SIMULATERESOLUTIONCONVOLVEDMODELTEST_H_
#define MANTID_MDALGORITHMS_SIMULATERESOLUTIONCONVOLVEDMODELTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/Quantification/SimulateResolutionConvolvedModel.h"

using Mantid::MDAlgorithms::SimulateResolutionConvolvedModel;

class SimulateResolutionConvolvedModelTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SimulateResolutionConvolvedModelTest *createSuite() { return new SimulateResolutionConvolvedModelTest(); }
  static void destroySuite( SimulateResolutionConvolvedModelTest *suite ) { delete suite; }

  void test_Init_Gives_Five_Properties()
  {
    SimulateResolutionConvolvedModel alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_EQUALS(alg.propertyCount(), 6);
  }

};


#endif /* MANTID_MDALGORITHMS_SIMULATERESOLUTIONCONVOLVEDMODELTEST_H_ */
