#ifndef MANTID_ALGORITHMS_POLARIZATIONCORRECTIONFREDRIKZE_TEST_H_
#define MANTID_ALGORITHMS_POLARIZATIONCORRECTIONFREDRIKZE_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/PolarizationCorrectionFredrikze.h"
#include "MantidDataHandling/CreatePolarizationEfficiencies.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/OptionalBool.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <boost/make_shared.hpp>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace WorkspaceCreationHelper;

class PolarizationCorrectionFredrikzeTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PolarizationCorrectionFredrikzeTest *createSuite() {
    return new PolarizationCorrectionFredrikzeTest();
  }
  static void destroySuite(PolarizationCorrectionFredrikzeTest *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  void test_Init() {
    PolarizationCorrectionFredrikze alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_set_wrong_workspace_type_throws() {
    MatrixWorkspace_sptr ws = boost::make_shared<Workspace2D>();
    PolarizationCorrectionFredrikze alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS(alg.setProperty("InputWorkspace", ws),
                     std::invalid_argument &);
  }

  void test_set_analysis_to_PA() {
    PolarizationCorrectionFredrikze alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PolarizationAnalysis", "PA"));
  }

  void test_set_analysis_to_PNR() {
    PolarizationCorrectionFredrikze alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PolarizationAnalysis", "PNR"));
  }

  void test_set_analysis_to_invalid_throws() {
    PolarizationCorrectionFredrikze alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS(alg.setProperty("PolarizationAnalysis", "_"),
                     std::invalid_argument &);
  }

  Mantid::API::WorkspaceGroup_sptr makeWorkspaceGroup() {
    Mantid::API::WorkspaceGroup_sptr group =
        boost::make_shared<WorkspaceGroup>();
    return group;
  }

  MatrixWorkspace_sptr makeEfficiencies(Workspace_sptr inWS,
                                        const std::string &rho,
                                        const std::string &pp,
                                        const std::string &alpha = "",
                                        const std::string &ap = "") {
    CreatePolarizationEfficiencies alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setPropertyValue("Rho", rho);
    alg.setPropertyValue("Pp", pp);
    if (!ap.empty()) {
      alg.setPropertyValue("Ap", ap);
      alg.setPropertyValue("Alpha", alpha);
    }
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    return outWS;
  }

  void test_throw_if_PA_and_group_is_wrong_size_throws() {
    Mantid::API::WorkspaceGroup_sptr inWS =
        boost::make_shared<WorkspaceGroup>(); // Empty group ws.

    // Name of the output workspace.
    std::string outWSName("PolarizationCorrectionFredrikzeTest_OutputWS");
    auto efficiencies = makeEfficiencies(create1DWorkspace(4, 1, 1), "1,1,1,1",
                                         "1,1,1,1", "1,1,1,1", "1,1,1,1");

    PolarizationCorrectionFredrikze alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("PolarizationAnalysis", "PA");
    alg.setProperty("Efficiencies", efficiencies);

    alg.setPropertyValue("OutputWorkspace", outWSName);
    TSM_ASSERT_THROWS("Wrong number of grouped workspaces, should throw",
                      alg.execute(), std::invalid_argument &);
  }

  void test_throw_if_PNR_and_group_is_wrong_size_throws() {
    Mantid::API::WorkspaceGroup_sptr inWS =
        boost::make_shared<WorkspaceGroup>(); // Empty group ws.

    // Name of the output workspace.
    std::string outWSName("PolarizationCorrectionFredrikzeTest_OutputWS");
    auto efficiencies = makeEfficiencies(create1DWorkspace(4, 1, 1), "1,1,1,1",
                                         "1,1,1,1", "1,1,1,1", "1,1,1,1");

    PolarizationCorrectionFredrikze alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("PolarizationAnalysis", "PNR");
    alg.setPropertyValue("OutputWorkspace", outWSName);
    alg.setProperty("Efficiencies", efficiencies);
    TSM_ASSERT_THROWS("Wrong number of grouped workspaces, should throw",
                      alg.execute(), std::invalid_argument &);
  }

  void test_throw_group_contains_other_workspace_types() {
    Mantid::API::WorkspaceGroup_sptr inWS =
        boost::make_shared<WorkspaceGroup>(); // Empty group ws.

    inWS->addWorkspace(boost::make_shared<TableWorkspace>()); // Group now
                                                              // contains a
                                                              // table workspace

    // Name of the output workspace.
    std::string outWSName("PolarizationCorrectionFredrikzeTest_OutputWS");
    auto efficiencies = makeEfficiencies(create1DWorkspace(4, 1, 1), "1,1,1,1",
                                         "1,1,1,1", "1,1,1,1", "1,1,1,1");

    PolarizationCorrectionFredrikze alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("PolarizationAnalysis", "PNR");
    alg.setPropertyValue("OutputWorkspace", outWSName);
    alg.setProperty("Efficiencies", efficiencies);
    TSM_ASSERT_THROWS("Wrong workspace types in group", alg.execute(),
                      std::invalid_argument &);
  }

  MatrixWorkspace_sptr create1DWorkspace(int size, double signal,
                                         double error) {
    auto ws = create1DWorkspaceConstant(size, signal, error, true);
    ws->getAxis(0)->setUnit("Wavelength");
    return ws;
  }

  // If polynomials are unity, no changes should be made.
  void test_run_PA_unity() {
    auto groupWS = boost::make_shared<WorkspaceGroup>(); // Empty group ws.
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    auto efficiencies = makeEfficiencies(create1DWorkspace(4, 1, 1), "1,0,0,0",
                                         "1,0,0,0", "1,0,0,0", "1,0,0,0");

    PolarizationCorrectionFredrikze alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", groupWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.setProperty("PolarizationAnalysis", "PA");
    alg.setProperty("Efficiencies", efficiencies);
    alg.execute();
    WorkspaceGroup_sptr outWS = alg.getProperty("OutputWorkspace");

    TSM_ASSERT_EQUALS("Wrong number of output workspaces", outWS->size(),
                      groupWS->size());

    for (size_t i = 0; i < outWS->size(); ++i) {
      std::cout << "Checking equivalent workspaces at index : " << i << '\n';
      auto checkAlg =
          AlgorithmManager::Instance().createUnmanaged("CompareWorkspaces");
      checkAlg->initialize();
      checkAlg->setChild(true);
      checkAlg->setProperty("Workspace1", groupWS->getItem(i));
      checkAlg->setProperty("Workspace2", outWS->getItem(i));
      checkAlg->setProperty("Tolerance", 3e-16);
      checkAlg->execute();
      TS_ASSERT(checkAlg->getProperty("Result"));
    }
  }

  void setInstrument(Workspace_sptr ws, const std::string &instrument_name) {
    auto alg = AlgorithmManager::Instance().createUnmanaged("LoadInstrument");
    AnalysisDataService::Instance().addOrReplace("dummy", ws);
    alg->initialize();
    alg->setProperty("Workspace", "dummy");
    alg->setProperty("InstrumentName", instrument_name);
    alg->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
    alg->execute();
  }

  void test_run_PA_default() {
    auto groupWS = boost::make_shared<WorkspaceGroup>(); // Empty group ws.
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    setInstrument(groupWS, "POLREF");
    auto efficiencies =
        makeEfficiencies(create1DWorkspace(4, 1, 1), "1,0,0,0", "1,0,0,0");

    PolarizationCorrectionFredrikze alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", groupWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.setProperty("PolarizationAnalysis", "PA");
    alg.setProperty("Efficiencies", efficiencies);
    alg.execute();
    WorkspaceGroup_sptr outWS = alg.getProperty("OutputWorkspace");

    TSM_ASSERT_EQUALS("Wrong number of output workspaces", outWS->size(),
                      groupWS->size());

    for (size_t i = 0; i < outWS->size(); ++i) {
      std::cout << "Checking equivalent workspaces at index : " << i << '\n';
      auto checkAlg =
          AlgorithmManager::Instance().createUnmanaged("CompareWorkspaces");
      checkAlg->initialize();
      checkAlg->setChild(true);
      checkAlg->setProperty("Workspace1", groupWS->getItem(i));
      checkAlg->setProperty("Workspace2", outWS->getItem(i));
      checkAlg->setProperty("Tolerance", 3e-16);
      checkAlg->execute();
      TS_ASSERT(!checkAlg->getProperty("Result"));
    }
  }

  void test_run_PA_default_no_instrument_parameters() {
    auto groupWS = boost::make_shared<WorkspaceGroup>(); // Empty group ws.
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    auto efficiencies =
        makeEfficiencies(create1DWorkspace(4, 1, 1), "1,0,0,0", "1,0,0,0");

    PolarizationCorrectionFredrikze alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", groupWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.setProperty("PolarizationAnalysis", "PA");
    alg.setProperty("Efficiencies", efficiencies);
    TSM_ASSERT_THROWS(
        "Instrument doesn't have default efficiencies, should throw",
        alg.execute(), std::invalid_argument &);
  }

  void test_run_PNR_unity() {
    auto groupWS = boost::make_shared<WorkspaceGroup>(); // Empty group ws.
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    auto efficiencies =
        makeEfficiencies(create1DWorkspace(4, 1, 1), "1,0,0,0", "1,0,0,0");

    PolarizationCorrectionFredrikze alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", groupWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.setProperty("PolarizationAnalysis", "PNR");
    alg.setProperty("Efficiencies", efficiencies);
    alg.execute();
    WorkspaceGroup_sptr outWS = alg.getProperty("OutputWorkspace");

    TSM_ASSERT_EQUALS("Wrong number of output workspaces", outWS->size(),
                      groupWS->size());

    for (size_t i = 0; i < outWS->size(); ++i) {
      std::cout << "Checking equivalent workspaces at index : " << i << '\n';
      auto checkAlg =
          AlgorithmManager::Instance().createUnmanaged("CompareWorkspaces");
      checkAlg->initialize();
      checkAlg->setChild(true);
      checkAlg->setProperty("Workspace1", groupWS->getItem(i));
      checkAlg->setProperty("Workspace2", outWS->getItem(i));
      checkAlg->execute();
      TS_ASSERT(checkAlg->getProperty("Result"));
    }
  }
};

#endif /* MANTID_ALGORITHMS_POLARIZATIONCORRECTIONFREDRIKZE_TEST_H_ */
