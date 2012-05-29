#ifndef MANTID_MDEVENTS_CONVERTTOREFLECTOMETRYQTEST_H_
#define MANTID_MDEVENTS_CONVERTTOREFLECTOMETRYQTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidMDEvents/ConvertToReflectometryQ.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/FrameworkManager.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::Geometry;

class ConvertToReflectometryQTest : public CxxTest::TestSuite
{
private:

  /*
  Boiler plate to produce a ConvertToReflectometryQ algorithm with a working set of input values.

  Each test can customise with properties it wishes to override over these standard values.

  Makes the tests much more readable like this.
  */
  boost::shared_ptr<ConvertToReflectometryQ> make_standard_algorithm()
  {
    MatrixWorkspace_sptr in_ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    in_ws->getAxis(0)->setUnit("Wavelength");

    Mantid::API::NumericAxis* const newAxis = new Mantid::API::NumericAxis(in_ws->getAxis(1)->length());
    in_ws->replaceAxis(1,newAxis);
    newAxis->unit() = boost::make_shared<Mantid::Kernel::Units::Degrees>();

    auto alg = boost::make_shared<ConvertToReflectometryQ>();
    alg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg->initialize() )
    TS_ASSERT( alg->isInitialized() )
    alg->setProperty("InputWorkspace", in_ws);
    alg->setProperty("OutputDimensions", "Q (lab frame)");
    alg->setPropertyValue("OutputWorkspace", "OutputTransformedWorkspace");
    alg->setProperty("OverrideIncidentTheta", true);
    alg->setProperty("IncidentTheta", 0.5);
    return alg;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertToReflectometryQTest *createSuite() { return new ConvertToReflectometryQTest(); }
  static void destroySuite( ConvertToReflectometryQTest *suite ) { delete suite; }

  void setUp()
  {
    Mantid::API::FrameworkManager::Instance();
  }

  void test_Init()
  {
    ConvertToReflectometryQ alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_theta_initial_negative_throws()
  {
    auto alg = make_standard_algorithm();
    alg->setProperty("OverrideIncidentTheta", true);
    alg->setProperty("IncidentTheta", -0.0001);
    TSM_ASSERT_THROWS("Incident theta is negative, should throw", alg->execute(), std::logic_error);
  }

  void test_theta_initial_too_large_throws()
  {
    auto alg = make_standard_algorithm();
    alg->setProperty("OverrideIncidentTheta", true);
    alg->setProperty("IncidentTheta", 90.001);
    TSM_ASSERT_THROWS("Incident theta is too large, should throw", alg->execute(), std::logic_error);
  }

  void test_wrong_number_of_extents_throws()
  {
    auto alg = make_standard_algorithm();
    alg->setProperty("Extents", "-1");
    TSM_ASSERT_THROWS("Should only accept 4 extents", alg->execute(), std::runtime_error);
  }

  void test_extents_with_qxmin_equals_qxmax_throws()
  {
    auto alg = make_standard_algorithm();
    alg->setProperty("Extents", "-1,-1,-1,1");
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
  }

  void test_extents_with_qxmin_more_than_qxmax_throws()
  {
    auto alg = make_standard_algorithm();
    alg->setProperty("Extents", "-1,-1.01,-1,1");
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
  }

  void test_extents_with_qxmin_less_than_qxmax()
  {
    auto alg = make_standard_algorithm();
    alg->setProperty("Extents", "-1,-0.999,-1,1");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
  }

  void test_extents_with_qzmin_equals_qzmax_throws()
  {
    auto alg = make_standard_algorithm();
    alg->setProperty("Extents", "-1,1,-1,-1");
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
  }

  void test_extents_with_qzmin_more_than_qzmax_throws()
  {
    auto alg = make_standard_algorithm();
    alg->setProperty("Extents", "-1,1,-1,-1.01");
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
  }

  void test_extents_with_qzmin_less_than_qzmax()
  {
    auto alg = make_standard_algorithm();
    alg->setProperty("Extents", "-1,1,0.999,1");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
  }

  //Characterisation test for the current state of the algorithm
  void test_only_support_q_conversion()
  {
    auto alg = make_standard_algorithm();
    alg->setProperty("OutputDimensions", "P (lab frame)");
    TSM_ASSERT_THROWS("Should throw as this mode is not supported yet", alg->execute(), std::runtime_error);
  }


};


#endif /* MANTID_MDALGORITHMS_CONVERTTOREFLECTOMETRYQTEST_H_ */