// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/PolarizationCorrectionFredrikze.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/OptionalBool.h"

#include <memory>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace WorkspaceCreationHelper;

class PolarizationCorrectionFredrikzeTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PolarizationCorrectionFredrikzeTest *createSuite() { return new PolarizationCorrectionFredrikzeTest(); }
  static void destroySuite(PolarizationCorrectionFredrikzeTest *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  PolarizationCorrectionFredrikzeTest() {
    // To make sure API is initialized properly
    Mantid::API::FrameworkManager::Instance();
  }

  void test_Init() {
    PolarizationCorrectionFredrikze alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_set_wrong_workspace_type_throws() {
    MatrixWorkspace_sptr ws = std::make_shared<Mantid::DataObjects::Workspace2D>();
    PolarizationCorrectionFredrikze alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS(alg.setProperty("InputWorkspace", ws), std::invalid_argument &);
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
    TS_ASSERT_THROWS(alg.setProperty("PolarizationAnalysis", "_"), std::invalid_argument &);
  }

  Mantid::API::WorkspaceGroup_sptr makeWorkspaceGroup() {
    Mantid::API::WorkspaceGroup_sptr group = std::make_shared<WorkspaceGroup>();
    return group;
  }

  MatrixWorkspace_sptr makeEfficiencies(const Workspace_sptr &inWS, const std::string &rho, const std::string &pp,
                                        const std::string &alpha = "", const std::string &ap = "") {
    auto alg = AlgorithmManager::Instance().createUnmanaged("CreatePolarizationEfficiencies");
    alg->setChild(true);
    alg->setRethrows(true);
    alg->initialize();
    alg->setProperty("InputWorkspace", inWS);
    alg->setPropertyValue("Rho", rho);
    alg->setPropertyValue("Pp", pp);
    if (!ap.empty()) {
      alg->setPropertyValue("Ap", ap);
      alg->setPropertyValue("Alpha", alpha);
    }
    alg->setPropertyValue("OutputWorkspace", "dummy");
    alg->execute();
    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
    return outWS;
  }

  void test_throw_if_PA_and_group_is_wrong_size_throws() {
    Mantid::API::WorkspaceGroup_sptr inWS = std::make_shared<WorkspaceGroup>(); // Empty group ws.

    // Name of the output workspace.
    std::string outWSName("PolarizationCorrectionFredrikzeTest_OutputWS");
    auto efficiencies = makeEfficiencies(create1DWorkspace(4, 1, 1), "1,1,1,1", "1,1,1,1", "1,1,1,1", "1,1,1,1");

    PolarizationCorrectionFredrikze alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("PolarizationAnalysis", "PA");
    alg.setProperty("Efficiencies", efficiencies);

    alg.setPropertyValue("OutputWorkspace", outWSName);
    TSM_ASSERT_THROWS("Wrong number of grouped workspaces, should throw", alg.execute(), std::invalid_argument &);
  }

  void test_throw_if_PNR_and_group_is_wrong_size_throws() {
    Mantid::API::WorkspaceGroup_sptr inWS = std::make_shared<WorkspaceGroup>(); // Empty group ws.

    // Name of the output workspace.
    std::string outWSName("PolarizationCorrectionFredrikzeTest_OutputWS");
    auto efficiencies = makeEfficiencies(create1DWorkspace(4, 1, 1), "1,1,1,1", "1,1,1,1", "1,1,1,1", "1,1,1,1");

    PolarizationCorrectionFredrikze alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("PolarizationAnalysis", "PNR");
    alg.setPropertyValue("OutputWorkspace", outWSName);
    alg.setProperty("Efficiencies", efficiencies);
    TSM_ASSERT_THROWS("Wrong number of grouped workspaces, should throw", alg.execute(), std::invalid_argument &);
  }

  void test_throw_group_contains_other_workspace_types() {
    Mantid::API::WorkspaceGroup_sptr inWS = std::make_shared<WorkspaceGroup>(); // Empty group ws.

    inWS->addWorkspace(std::make_shared<Mantid::DataObjects::TableWorkspace>()); // Group now
                                                                                 // contains a
                                                                                 // table
                                                                                 // workspace

    // Name of the output workspace.
    std::string outWSName("PolarizationCorrectionFredrikzeTest_OutputWS");
    auto efficiencies = makeEfficiencies(create1DWorkspace(4, 1, 1), "1,1,1,1", "1,1,1,1", "1,1,1,1", "1,1,1,1");

    PolarizationCorrectionFredrikze alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("PolarizationAnalysis", "PNR");
    alg.setPropertyValue("OutputWorkspace", outWSName);
    alg.setProperty("Efficiencies", efficiencies);
    TSM_ASSERT_THROWS("Wrong workspace types in group", alg.execute(), std::invalid_argument &);
  }

  MatrixWorkspace_sptr create1DWorkspace(int size, double signal, double error) {
    auto ws = create1DWorkspaceConstant(size, signal, error, true);
    ws->getAxis(0)->setUnit("Wavelength");
    return ws;
  }

  // If polynomials are unity, no changes should be made.
  void test_run_PA_unity() {
    auto groupWS = std::make_shared<WorkspaceGroup>(); // Empty group ws.
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    auto efficiencies = makeEfficiencies(create1DWorkspace(4, 1, 1), "1,0,0,0", "1,0,0,0", "1,0,0,0", "1,0,0,0");

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

    TSM_ASSERT_EQUALS("Wrong number of output workspaces", outWS->size(), groupWS->size());

    for (size_t i = 0; i < outWS->size(); ++i) {
      std::cout << "Checking equivalent workspaces at index : " << i << '\n';
      auto checkAlg = AlgorithmManager::Instance().createUnmanaged("CompareWorkspaces");
      checkAlg->initialize();
      checkAlg->setChild(true);
      checkAlg->setProperty("Workspace1", groupWS->getItem(i));
      checkAlg->setProperty("Workspace2", outWS->getItem(i));
      checkAlg->setProperty("Tolerance", 3e-16);
      checkAlg->execute();
      TS_ASSERT(checkAlg->getProperty("Result"));
    }
  }

  void setInstrument(const Workspace_sptr &ws, const std::string &instrument_name) {
    auto alg = AlgorithmManager::Instance().createUnmanaged("LoadInstrument");
    AnalysisDataService::Instance().addOrReplace("dummy", ws);
    alg->initialize();
    alg->setProperty("Workspace", "dummy");
    alg->setProperty("InstrumentName", instrument_name);
    alg->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
    alg->execute();
  }

  void test_run_PA_default() {
    auto groupWS = std::make_shared<WorkspaceGroup>(); // Empty group ws.
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    setInstrument(groupWS, "POLREF");
    auto efficiencies = makeEfficiencies(create1DWorkspace(4, 1, 1), "1,0,0,0", "1,0,0,0");

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

    TSM_ASSERT_EQUALS("Wrong number of output workspaces", outWS->size(), groupWS->size());

    for (size_t i = 0; i < outWS->size(); ++i) {
      std::cout << "Checking equivalent workspaces at index : " << i << '\n';
      auto checkAlg = AlgorithmManager::Instance().createUnmanaged("CompareWorkspaces");
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
    auto groupWS = std::make_shared<WorkspaceGroup>(); // Empty group ws.
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    auto efficiencies = makeEfficiencies(create1DWorkspace(4, 1, 1), "1,0,0,0", "1,0,0,0");

    PolarizationCorrectionFredrikze alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", groupWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.setProperty("PolarizationAnalysis", "PA");
    alg.setProperty("Efficiencies", efficiencies);
    TSM_ASSERT_THROWS("Instrument doesn't have default efficiencies, should throw", alg.execute(),
                      std::invalid_argument &);
  }

  void test_run_PNR_unity() {
    auto groupWS = std::make_shared<WorkspaceGroup>(); // Empty group ws.
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    auto efficiencies = makeEfficiencies(create1DWorkspace(4, 1, 1), "1,0,0,0", "1,0,0,0");

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

    TSM_ASSERT_EQUALS("Wrong number of output workspaces", outWS->size(), groupWS->size());

    for (size_t i = 0; i < outWS->size(); ++i) {
      std::cout << "Checking equivalent workspaces at index : " << i << '\n';
      auto checkAlg = AlgorithmManager::Instance().createUnmanaged("CompareWorkspaces");
      checkAlg->initialize();
      checkAlg->setChild(true);
      checkAlg->setProperty("Workspace1", groupWS->getItem(i));
      checkAlg->setProperty("Workspace2", outWS->getItem(i));
      checkAlg->execute();
      TS_ASSERT(checkAlg->getProperty("Result"));
    }
  }

  void test_run_PA_non_unity() {
    auto Rpp = create1DWorkspace(4, 0.9, 1);
    auto Rpa = create1DWorkspace(4, 0.8, 1);
    auto Rap = create1DWorkspace(4, 0.7, 1);
    auto Raa = create1DWorkspace(4, 0.6, 1);

    auto Pp = create1DWorkspace(4, 0.99, 1);
    auto Ap = create1DWorkspace(4, 0.98, 1);
    auto Pa = create1DWorkspace(4, 0.97, 1);
    auto Aa = create1DWorkspace(4, 0.96, 1);

    auto Rho = Pa / Pp;
    auto Alpha = Aa / Ap;

    auto join_eff = AlgorithmManager::Instance().createUnmanaged("JoinISISPolarizationEfficiencies");
    join_eff->initialize();
    join_eff->setChild(true);
    join_eff->setRethrows(true);
    join_eff->setProperty("Pp", Pp);
    join_eff->setProperty("Ap", Ap);
    join_eff->setProperty("Rho", Rho);
    join_eff->setProperty("Alpha", Alpha);
    join_eff->setPropertyValue("OutputWorkspace", "dummy");
    join_eff->execute();
    MatrixWorkspace_sptr efficiencies = join_eff->getProperty("OutputWorkspace");
    TS_ASSERT(efficiencies);

    auto groupWS = std::make_shared<WorkspaceGroup>(); // Empty group ws.

    auto Ipp = (Rpp * (Pp + 1.0) * (Ap + 1.0) + Raa * (Pp * (-1.0) + 1.0) * (Ap * (-1.0) + 1.0) +
                Rpa * (Pp + 1.0) * (Ap * (-1.0) + 1.0) + Rap * (Pp * (-1.0) + 1.0) * (Ap + 1.0)) /
               4.;
    auto Iaa = (Raa * (Pa + 1.0) * (Aa + 1.0) + Rpp * (Pa * (-1.0) + 1.0) * (Aa * (-1.0) + 1.0) +
                Rap * (Pa + 1.0) * (Aa * (-1.0) + 1.0) + Rpa * (Pa * (-1.0) + 1.0) * (Aa + 1.0)) /
               4.;
    auto Ipa = (Rpa * (Pp + 1.0) * (Aa + 1.0) + Rap * (Pp * (-1.0) + 1.0) * (Aa * (-1.0) + 1.0) +
                Rpp * (Pp + 1.0) * (Aa * (-1.0) + 1.0) + Raa * (Pp * (-1.0) + 1.0) * (Aa + 1.0)) /
               4.;
    auto Iap = (Rap * (Pa + 1.0) * (Ap + 1.0) + Rpa * (Pa * (-1.0) + 1.0) * (Ap * (-1.0) + 1.0) +
                Raa * (Pa + 1.0) * (Ap * (-1.0) + 1.0) + Rpp * (Pa * (-1.0) + 1.0) * (Ap + 1.0)) /
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

    TSM_ASSERT_EQUALS("Wrong number of output workspaces", outWS->size(), groupWS->size());

    auto out1 = std::dynamic_pointer_cast<MatrixWorkspace>(outWS->getItem(0));
    auto out2 = std::dynamic_pointer_cast<MatrixWorkspace>(outWS->getItem(1));
    auto out3 = std::dynamic_pointer_cast<MatrixWorkspace>(outWS->getItem(2));
    auto out4 = std::dynamic_pointer_cast<MatrixWorkspace>(outWS->getItem(3));

    TS_ASSERT_DELTA(out1->y(0)[0], 0.9, 1e-14);
    TS_ASSERT_DELTA(out2->y(0)[0], 0.8, 1e-14);
    TS_ASSERT_DELTA(out3->y(0)[0], 0.7, 1e-14);
    TS_ASSERT_DELTA(out4->y(0)[0], 0.6, 1e-14);
  }

  void test_valid_spin_state_order_for_PA() {
    auto groupWS = std::make_shared<WorkspaceGroup>();
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));

    PolarizationCorrectionFredrikze alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", groupWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.setProperty("PolarizationAnalysis", "PA");
    auto efficiencies = makeEfficiencies(create1DWorkspace(4, 1, 1), "1,0,0,0", "1,0,0,0", "1,0,0,0", "1,0,0,0");
    alg.setProperty("Efficiencies", efficiencies);
    alg.setProperty("InputSpinStateOrder", "pp,pa,ap,aa");
    alg.setProperty("OutputSpinStateOrder", "xx,pa,ap,aa");
    // Ensure that the algorithm executes without throwing any errors
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

  // void test_invalid_spinstate_order_does_not_match_input_workspaces() {
  //   auto groupWS = std::make_shared<WorkspaceGroup>(); // Create group workspace
  //   groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
  //   groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
  //   groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
  //   groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
  //   auto efficiencies = makeEfficiencies(create1DWorkspace(4, 1, 1), "1,1,1,1", "1,1,1,1", "1,1,1,1", "1,1,1,1");
  //
  //   PolarizationCorrectionFredrikze alg;
  //   alg.setChild(true);
  //   alg.setRethrows(true);
  //   alg.initialize();
  //   alg.setProperty("InputWorkspace", groupWS);
  //   alg.setPropertyValue("OutputWorkspace", "dummy");
  //   alg.setProperty("PolarizationAnalysis", "PA");
  //   alg.setProperty("Efficiencies", efficiencies);
  //   alg.setProperty("InputSpinStateOrder", "pp,pa,ap");
  //   alg.setProperty("OutputSpinStateOrder", "pp,pa,ap");
  //
  //   TSM_ASSERT_THROWS("Spin state order count does not match input workspaces count, should throw", alg.execute(),
  //                     std::invalid_argument &);
  // }

  // Test various combinations of spin state orders
  void test_various_spin_state_orders_for_PA() {
    auto groupWS = std::make_shared<WorkspaceGroup>();
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));

    auto efficiencies = makeEfficiencies(create1DWorkspace(4, 1, 1), "1,0,0,0", "1,0,0,0", "1,0,0,0", "1,0,0,0");

    PolarizationCorrectionFredrikze alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", groupWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.setProperty("PolarizationAnalysis", "PA");
    alg.setProperty("Efficiencies", efficiencies);

    // Test different input and output spin state orders
    std::vector<std::string> spinStateOrders = {"pp,pa,ap,aa", "pa,pp,aa,ap", "ap,aa,pa,pp", "aa,ap,pp,pa"};

    for (const auto &inputOrder : spinStateOrders) {
      for (const auto &outputOrder : spinStateOrders) {
        alg.setProperty("InputSpinStateOrder", inputOrder);
        alg.setProperty("OutputSpinStateOrder", outputOrder);
        TS_ASSERT_THROWS_NOTHING(alg.execute());
      }
    }
  }
};
