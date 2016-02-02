#ifndef MANTID_MDAGORITHMS_INTEGRATEPEAKSCWSDTEST_H_
#define MANTID_MDAGORITHMS_INTEGRATEPEAKSCWSDTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidMDAlgorithms/IntegratePeaksCWSD.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

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


  /** Add a list of MDEvents around Q = (1, 2, 3)
   * @brief createMDWorkspace
   */
  void createMDWorkspace()
  {
    // Copy the code from 'ConvertCWSDExpToMomentum'

  }

  /** Add a peak at Q = (1, 2, 3)
   */
  void peakWorkspace()
  {
    // Create a peak workspace
  }

  PeaksWorkspace_sptr buildPW() {
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentRectangular2(1, 10);
    inst->setName("SillyInstrument");
    auto pw = PeaksWorkspace_sptr(new PeaksWorkspace);
    pw->setInstrument(inst);
    std::string val = "value";
    pw->mutableRun().addProperty("TestProp", val);
    Peak p(inst, 1, 3.0);
    pw->addPeak(p);
    return pw;
  }

};



#endif /* MANTID_MDEVENTS_INTEGRATEPEAKSCWSDTEST_H_ */
