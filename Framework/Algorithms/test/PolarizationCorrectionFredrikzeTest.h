#ifndef MANTID_ALGORITHMS_POLARIZATIONCORRECTIONFREDRIKZE_TEST_H_
#define MANTID_ALGORITHMS_POLARIZATIONCORRECTIONFREDRIKZE_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/CreatePolarizationEfficiencies.h"
#include "MantidAlgorithms/PolarizationCorrectionFredrikze.h"
#include "MantidDataHandling/JoinISISPolarizationEfficiencies.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/OptionalBool.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <boost/make_shared.hpp>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
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

  void xtest_run_PA_non_unity() {
    auto Rpp = create1DWorkspace(4, 0.9, 1);
    auto Raa = create1DWorkspace(4, 0.8, 1);
    auto Rpa = create1DWorkspace(4, 0.7, 1);
    auto Rap = create1DWorkspace(4, 0.6, 1);

    auto Pp = create1DWorkspace(4, 0.99, 1);
    auto Ap = create1DWorkspace(4, 0.98, 1);
    auto Pa = create1DWorkspace(4, 0.97, 1);
    auto Aa = create1DWorkspace(4, 0.96, 1);

    auto Rho = Pa / Pp;
    auto Alpha = Aa / Ap;

    Mantid::DataHandling::JoinISISPolarizationEfficiencies join_eff;
    join_eff.initialize();
    join_eff.setChild(true);
    join_eff.setRethrows(true);
    join_eff.setProperty("Pp", Pp);
    join_eff.setProperty("Ap", Ap);
    join_eff.setProperty("Rho", Rho);
    join_eff.setProperty("Alpha", Alpha);
    join_eff.setPropertyValue("OutputWorkspace", "dummy");
    join_eff.execute();
    MatrixWorkspace_sptr efficiencies = join_eff.getProperty("OutputWorkspace");
    TS_ASSERT(efficiencies);

    auto groupWS = boost::make_shared<WorkspaceGroup>(); // Empty group ws.

    auto Ipp = (Rpp * (Pp + 1.0) * (Ap + 1.0) +
                Raa * (Pp * (-1.0) + 1.0) * (Ap * (-1.0) + 1.0) +
                Rpa * (Pp + 1.0) * (Ap * (-1.0) + 1.0) +
                Rap * (Pp * (-1.0) + 1.0) * (Ap + 1.0)) /
               4.;
    auto Iaa = (Raa * (Pa + 1.0) * (Aa + 1.0) +
                Rpp * (Pa * (-1.0) + 1.0) * (Aa * (-1.0) + 1.0) +
                Rap * (Pa + 1.0) * (Aa * (-1.0) + 1.0) +
                Rpa * (Pa * (-1.0) + 1.0) * (Aa + 1.0)) /
               4.;
    auto Ipa = (Rpa * (Pp + 1.0) * (Aa + 1.0) +
                Rap * (Pp * (-1.0) + 1.0) * (Aa * (-1.0) + 1.0) +
                Rpp * (Pp + 1.0) * (Aa * (-1.0) + 1.0) +
                Raa * (Pp * (-1.0) + 1.0) * (Aa + 1.0)) /
               4.;
    auto Iap = (Rap * (Pa + 1.0) * (Ap + 1.0) +
                Rpa * (Pa * (-1.0) + 1.0) * (Ap * (-1.0) + 1.0) +
                Raa * (Pa + 1.0) * (Ap * (-1.0) + 1.0) +
                Rpp * (Pa * (-1.0) + 1.0) * (Ap + 1.0)) /
               4.;

    groupWS->addWorkspace(Ipp);
    groupWS->addWorkspace(Ipa);
    groupWS->addWorkspace(Iap);
    groupWS->addWorkspace(Iaa);

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

    auto out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(outWS->getItem(0));
    auto out2 = boost::dynamic_pointer_cast<MatrixWorkspace>(outWS->getItem(1));
    auto out3 = boost::dynamic_pointer_cast<MatrixWorkspace>(outWS->getItem(2));
    auto out4 = boost::dynamic_pointer_cast<MatrixWorkspace>(outWS->getItem(3));

    std::cerr << std::endl;
    std::cerr << out1->y(0)[0] << ' ' << out1->y(0)[1] << std::endl;
    std::cerr << out2->y(0)[0] << ' ' << out2->y(0)[1] << std::endl;
    std::cerr << out3->y(0)[0] << ' ' << out3->y(0)[1] << std::endl;
    std::cerr << out4->y(0)[0] << ' ' << out4->y(0)[1] << std::endl;

    TS_ASSERT_DELTA(out1->y(0)[0], 0.9, 1e-14);
    TS_ASSERT_DELTA(out2->y(0)[0], 0.9, 1e-14);
    TS_ASSERT_DELTA(out3->y(0)[0], 0.9, 1e-14);
    TS_ASSERT_DELTA(out4->y(0)[0], 0.9, 1e-14);
  }
};

#endif /* MANTID_ALGORITHMS_POLARIZATIONCORRECTIONFREDRIKZE_TEST_H_ */
