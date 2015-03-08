#ifndef MANTID_MDEVENTS_CONVERTTOREFLECTOMETRYQTEST_H_
#define MANTID_MDEVENTS_CONVERTTOREFLECTOMETRYQTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidMDAlgorithms/ConvertToReflectometryQ.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/FrameworkManager.h"
#include <boost/assign.hpp>

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
  boost::shared_ptr<ConvertToReflectometryQ> make_standard_algorithm(const std::string outputdimensions="Q (lab frame)", bool outputAsMD = true)
  {
    MatrixWorkspace_sptr in_ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    in_ws->getAxis(0)->setUnit("Wavelength");

    Kernel::PropertyWithValue<std::string>* testProperty = new Kernel::PropertyWithValue<std::string>("test_property", "test_value", Kernel::Direction::Input);
    in_ws->mutableRun().addLogData(testProperty);

    Mantid::API::NumericAxis* const newAxis = new Mantid::API::NumericAxis(in_ws->getAxis(1)->length());
    in_ws->replaceAxis(1,newAxis);
    newAxis->unit() = boost::make_shared<Mantid::Kernel::Units::Degrees>();

    auto alg = boost::make_shared<ConvertToReflectometryQ>();
    alg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg->initialize() )
    TS_ASSERT( alg->isInitialized() )
    alg->setProperty("InputWorkspace", in_ws);
    alg->setProperty("OutputDimensions", outputdimensions);
    alg->setPropertyValue("OutputWorkspace", "OutputTransformedWorkspace");
    alg->setProperty("OverrideIncidentTheta", true);
    alg->setProperty("OutputAsMDWorkspace", outputAsMD);
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

  void test_name()
  {
    ConvertToReflectometryQ alg;
    TS_ASSERT_EQUALS("ConvertToReflectometryQ", alg.name());
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

  void test_execute_qxqz_md()
  {
     auto alg = make_standard_algorithm();
     alg->execute();
     auto ws = boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve("OutputTransformedWorkspace"));
     TS_ASSERT(ws != NULL);
     TS_ASSERT_EQUALS(2, ws->getExperimentInfo(0)->run().getLogData().size());
  }

  void test_execute_kikf_md()
  {
     auto alg = make_standard_algorithm("K (incident, final)");
     TS_ASSERT_THROWS_NOTHING(alg->execute());
     auto ws = boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve("OutputTransformedWorkspace"));
     TS_ASSERT(ws != NULL);
  }

  void test_execute_pipf_md()
  {
     auto alg = make_standard_algorithm("P (lab frame)");
     TS_ASSERT_THROWS_NOTHING(alg->execute());
     auto ws = boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve("OutputTransformedWorkspace"));
     TS_ASSERT(ws != NULL);
  }

  void test_execute_qxqz_2D()
  {
     const bool outputAsMD = false;
     auto alg = make_standard_algorithm("Q (lab frame)", outputAsMD);
     alg->execute();
     auto ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve("OutputTransformedWorkspace"));
     TS_ASSERT(ws != NULL);
     TS_ASSERT_EQUALS(2, ws->run().getLogData().size());
  }

  void test_execute_kikf_2D()
  {
     const bool outputAsMD = false;
     auto alg = make_standard_algorithm("K (incident, final)", outputAsMD);
     TS_ASSERT_THROWS_NOTHING(alg->execute());
     auto ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve("OutputTransformedWorkspace"));
     TS_ASSERT(ws != NULL);
  }

  void test_execute_pipf_2D()
  {
     const bool outputAsMD = false;
     auto alg = make_standard_algorithm("P (lab frame)", outputAsMD);
     TS_ASSERT_THROWS_NOTHING(alg->execute());
     auto ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve("OutputTransformedWorkspace"));
     TS_ASSERT(ws != NULL);
  }

  void test_box_controller_defaults()
  {
    auto alg = make_standard_algorithm();
    alg->setProperty("Extents", "-1,1,0.999,1");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    std::string outWSName = alg->getPropertyValue("OutputWorkspace");
    auto outWS = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(outWSName);
    auto bc = outWS->getBoxController();

    TS_ASSERT_EQUALS(2, bc->getSplitInto(0));
    TS_ASSERT_EQUALS(2, bc->getSplitInto(1));
    TS_ASSERT_EQUALS(50, bc->getSplitThreshold());
    TS_ASSERT_EQUALS(10, bc->getMaxDepth());
  }

  void test_apply_box_controller_settings()
  {
    auto alg = make_standard_algorithm();
    alg->setProperty("Extents", "-1,1,0.999,1");

    const int splitThreshold = 3;
    const int splitInto = 6;
    const int maxDepth = 12;
    alg->setProperty("SplitThreshold", splitThreshold);
    alg->setProperty("SplitInto", boost::assign::list_of(splitInto).convert_to_container<std::vector<int> >());
    alg->setProperty("MaxRecursionDepth", maxDepth);

    TS_ASSERT_THROWS_NOTHING(alg->execute());
    std::string outWSName = alg->getPropertyValue("OutputWorkspace");
    auto outWS = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(outWSName);
    auto bc = outWS->getBoxController();

    // Check that the box controller settings percolate through to the output workspace.
    TS_ASSERT_EQUALS(splitInto, bc->getSplitInto(0));
    TS_ASSERT_EQUALS(splitInto, bc->getSplitInto(1));
    TS_ASSERT_EQUALS(splitThreshold, bc->getSplitThreshold());
    TS_ASSERT_EQUALS(maxDepth, bc->getMaxDepth());
  }
};

/* Peformance testing */
class ConvertToReflectometryQTestPerformance : public CxxTest::TestSuite
{
private:

  WorkspaceGroup_sptr ws;

public:

  void setUp()
  {
    // Load some data
    IAlgorithm* loadalg = FrameworkManager::Instance().createAlgorithm("Load");
    loadalg->setRethrows(true);
    loadalg->initialize();
    loadalg->setPropertyValue("Filename", "POLREF00004699.nxs");
    loadalg->setPropertyValue("OutputWorkspace", "testws");
    loadalg->execute();
    
    // Convert units to wavelength
    IAlgorithm* unitsalg = FrameworkManager::Instance().createAlgorithm("ConvertUnits");
    unitsalg->initialize();
    unitsalg->setPropertyValue("InputWorkspace", "testws");
    unitsalg->setPropertyValue("OutputWorkspace", "testws");
    unitsalg->setPropertyValue("Target", "Wavelength");
    unitsalg->execute();

    // Convert the specturm axis ot signed_theta
    IAlgorithm* specaxisalg = FrameworkManager::Instance().createAlgorithm("ConvertSpectrumAxis");
    specaxisalg->initialize();
    specaxisalg->setPropertyValue("InputWorkspace", "testws");
    specaxisalg->setPropertyValue("OutputWorkspace", "testws");
    specaxisalg->setPropertyValue("Target", "signed_theta");
    specaxisalg->execute();
    
    ws = API::AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("testws");

  }

  void testPerformance()
  {
    TS_ASSERT(true);
    ConvertToReflectometryQ alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", ws->getItem(0));
    alg.setProperty("OutputDimensions", "Q (lab frame)");
    alg.setPropertyValue("OutputWorkspace", "OutputTransformedWorkspace");
    alg.setProperty("OverrideIncidentTheta", true);
    alg.setProperty("IncidentTheta", 0.5);
    TS_ASSERT(alg.execute());
    TS_ASSERT(alg.isExecuted());
    IMDWorkspace_sptr out = API::AnalysisDataService::Instance().retrieveWS<IMDWorkspace>("OutputTransformedWorkspace");
    TS_ASSERT(out != NULL);
    TS_ASSERT_EQUALS(out->getNumDims(), 2);
  }
};


#endif /* MANTID_MDALGORITHMS_CONVERTTOREFLECTOMETRYQTEST_H_ */
