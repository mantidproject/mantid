#ifndef MANTID_MDAGORITHMS_INTEGRATEPEAKSCWSDTEST_H_
#define MANTID_MDAGORITHMS_INTEGRATEPEAKSCWSDTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidMDAlgorithms/IntegratePeaksCWSD.h"

#include <cxxtest/TestSuite.h>

#include <Poco/File.h>

using Mantid::API::AnalysisDataService;
using Mantid::Geometry::MDHistoDimension;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::MDAlgorithms;
using Mantid::Kernel::V3D;

class IntegratePeaksCWSDTest : public CxxTest::TestSuite {
public:

  static IntegratePeaksCWSDTest *createSuite()
  {
    return new IntegratePeaksCWSDTest();
  }

  static void destroySuite(IntegratePeaksCWSDTest *suite)
  {
    delete suite;
  }

  //-------------------------------------------------------------------------------
  void test_Init() {
    IntegratePeaksCWSD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }
};



#endif /* MANTID_MDEVENTS_INTEGRATEPEAKSCWSDTEST_H_ */
