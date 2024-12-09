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
#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/OptionalBool.h"

#include <memory>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace WorkspaceCreationHelper;

class PolarizationCorrectionFredrikzeTest : public CxxTest::TestSuite {
private:
  const std::string m_outputWSName{"output"};
  static const int PA_GROUP_SIZE = 4;
  static const int PNR_GROUP_SIZE = 2;

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
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
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
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PolarizationAnalysis", "PNR"));
  }

  void test_set_analysis_to_invalid_throws() {
    PolarizationCorrectionFredrikze alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS(alg.setProperty("PolarizationAnalysis", "_"), std::invalid_argument &);
  }

  void test_throw_if_PA_and_group_is_wrong_size_throws() {
    Mantid::API::WorkspaceGroup_sptr inWS = std::make_shared<WorkspaceGroup>(); // Empty group ws.

    // Name of the output workspace.
    std::string outWSName("PolarizationCorrectionFredrikzeTest_OutputWS");
    auto alg = initializeAlgorithm(inWS, "PA", "1,1,1,1");

    alg->setPropertyValue("OutputWorkspace", outWSName);
    TSM_ASSERT_THROWS("Wrong number of grouped workspaces, should throw", alg->execute(), std::invalid_argument &);
  }

  void test_throw_if_PNR_and_group_is_wrong_size_throws() {
    Mantid::API::WorkspaceGroup_sptr inWS = std::make_shared<WorkspaceGroup>(); // Empty group ws.

    // Name of the output workspace.
    std::string outWSName("PolarizationCorrectionFredrikzeTest_OutputWS");
    auto alg = initializeAlgorithm(inWS, "PNR", "1,1,1,1");

    TSM_ASSERT_THROWS("Wrong number of grouped workspaces, should throw", alg->execute(), std::invalid_argument &);
  }

  void test_throw_group_contains_other_workspace_types() {
    Mantid::API::WorkspaceGroup_sptr inWS = std::make_shared<WorkspaceGroup>(); // Empty group ws.

    inWS->addWorkspace(std::make_shared<Mantid::DataObjects::TableWorkspace>()); // Group now
                                                                                 // contains a
                                                                                 // table
                                                                                 // workspace

    // Name of the output workspace.
    std::string outWSName("PolarizationCorrectionFredrikzeTest_OutputWS");
    auto alg = initializeAlgorithm(inWS, "PNR", "1,1,1,1");

    TSM_ASSERT_THROWS("Wrong workspace types in group", alg->execute(), std::invalid_argument &);
  }

  MatrixWorkspace_sptr create1DWorkspace(int size, double signal, double error) {
    auto ws = create1DWorkspaceConstant(size, signal, error, true);
    ws->getAxis(0)->setUnit("Wavelength");
    return ws;
  }

  // If polynomials are unity, no changes should be made.
  void test_run_PA_unity() {
    auto groupWS = createGroupWorkspace(4, 4, 1, 1);

    auto efficiencies = makeEfficiencies(create1DWorkspace(4, 1, 1), "1,0,0,0", "1,0,0,0", "1,0,0,0", "1,0,0,0");
    auto alg = initializeAlgorithm(groupWS, "PA", "", efficiencies);

    alg->execute();
    WorkspaceGroup_sptr outWS = alg->getProperty("OutputWorkspace");

    TSM_ASSERT_EQUALS("Wrong number of output workspaces", outWS->size(), groupWS->size());

    for (size_t i = 0; i < outWS->size(); i++) {
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
    AnalysisDataService::Instance().addOrReplace(m_outputWSName, ws);
    alg->initialize();
    alg->setProperty("Workspace", m_outputWSName);
    alg->setProperty("InstrumentName", instrument_name);
    alg->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
    alg->execute();
  }

  void test_run_PA_default() {
    auto groupWS = createGroupWorkspace(4, 4, 1, 1);

    setInstrument(groupWS, "POLREF");
    auto efficiencies = makeEfficiencies(create1DWorkspace(4, 1, 1), "1,0,0,0", "1,0,0,0");

    auto alg = initializeAlgorithm(groupWS, "PA", "", efficiencies);

    alg->execute();
    WorkspaceGroup_sptr outWS = alg->getProperty("OutputWorkspace");

    TSM_ASSERT_EQUALS("Wrong number of output workspaces", outWS->size(), groupWS->size());

    for (size_t i = 0; i < outWS->size(); i++) {
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
    auto groupWS = createGroupWorkspace(4, 4, 1, 1);
    auto efficiencies = makeEfficiencies(create1DWorkspace(4, 1, 1), "1,0,0,0", "1,0,0,0");
    auto alg = initializeAlgorithm(groupWS, "PA", "", efficiencies);

    TSM_ASSERT_THROWS("Instrument doesn't have default efficiencies, should throw", alg->execute(),
                      std::invalid_argument &);
  }

  void test_run_PNR_unity() {
    auto groupWS = createGroupWorkspace(2, 4, 1, 1);
    auto efficiencies = makeEfficiencies(create1DWorkspace(4, 1, 1), "1,0,0,0", "1,0,0,0");
    auto alg = initializeAlgorithm(groupWS, "PNR", "", efficiencies);

    alg->execute();
    WorkspaceGroup_sptr outWS = alg->getProperty("OutputWorkspace");

    TSM_ASSERT_EQUALS("Wrong number of output workspaces", outWS->size(), groupWS->size());

    for (size_t i = 0; i < outWS->size(); i++) {
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
    join_eff->setPropertyValue("OutputWorkspace", m_outputWSName);
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

    auto alg = initializeAlgorithm(groupWS, "PA", "", efficiencies);

    alg->execute();
    WorkspaceGroup_sptr outWS = alg->getProperty("OutputWorkspace");

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

  void test_default_spin_state_order_for_PA() {
    auto groupWS = createGroupWorkspace(4, 4, 1, 1);
    auto alg = initializeAlgorithm(groupWS, "PA", "1,0,0,0");
    alg->setProperty("AddSpinStateToLog", true);

    TS_ASSERT_THROWS_NOTHING(alg->execute());
    const Mantid::API::WorkspaceGroup_sptr outputWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), PA_GROUP_SIZE)
    const std::array<std::string, 4> EXPECTED_LOG_VALUES{
        {SpinStatesORSO::PP, SpinStatesORSO::PM, SpinStatesORSO::MP, SpinStatesORSO::MM}};

    for (size_t i = 0; i != 4; i++) {
      MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(i));
      TS_ASSERT(ws)
      const auto &run = ws->run();
      TS_ASSERT(run.hasProperty(SpinStatesORSO::LOG_NAME))
      TS_ASSERT_EQUALS(run.getPropertyValueAsType<std::string>(SpinStatesORSO::LOG_NAME), EXPECTED_LOG_VALUES[i])
    }
  }

  void test_spin_state_not_added_to_sample_log_by_default_for_PA() {
    auto groupWS = createGroupWorkspace(4, 4, 1, 1);
    auto alg = initializeAlgorithm(groupWS, "PA", "1,0,0,0");

    TS_ASSERT_THROWS_NOTHING(alg->execute());
    const Mantid::API::WorkspaceGroup_sptr outputWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), PA_GROUP_SIZE)

    for (size_t i = 0; i != 4; i++) {
      MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(i));
      TS_ASSERT(ws)
      TS_ASSERT(!ws->run().hasProperty(SpinStatesORSO::LOG_NAME))
    }
  }

  void test_invalid_input_and_output_spin_state_order_for_PA() {
    auto groupWS = createGroupWorkspace(4, 4, 1, 1);
    auto alg = initializeAlgorithm(groupWS, "PA", "1,0,0,0");

    TSM_ASSERT_THROWS("Invalid value for property InputSpinStates (string) from string pu,pa,ap,aa: The spin states "
                      "must either be one or two characters, "
                      "with each being either a p or a",
                      alg->setProperty("InputSpinStates", "pu,pa,ap,aa"), std::invalid_argument &);
    TSM_ASSERT_THROWS("Invalid value for property OutputSpinStates (string) from string xx,pa,ap,aa: The spin states "
                      "must either be one or two characters, "
                      "with each being either a p or a",
                      alg->setProperty("OutputSpinStates", "xx,pp,ap,aa"), std::invalid_argument &);

    alg->setProperty("InputSpinStates", "pp,aa");
    TSM_ASSERT_THROWS("Invalid value for property InputSpinStates (string) from string pp,aa: The spin states must "
                      "either be one or two characters, "
                      "with each being either a p or a",
                      alg->execute(), std::invalid_argument &);

    alg->setProperty("OutputSpinStates", "pp,aa");
    TSM_ASSERT_THROWS("Invalid value for property OutputSpinStates (string) from string pp,aa: The spin states must "
                      "either be one or two characters, "
                      "with each being either a p or a",
                      alg->execute(), std::invalid_argument &);
  }

  void test_valid_spin_state_order_for_PA() {
    auto groupWS = createGroupWorkspace(4, 4, 1, 1);
    auto alg = initializeAlgorithm(groupWS, "PA", "1,0,0,0");

    // Define spin state orders and expected log values
    std::vector<std::string> spinStateOrders = {
        SpinStateConfigurationsFredrikze::PARA_PARA + "," + SpinStateConfigurationsFredrikze::PARA_ANTI + "," +
            SpinStateConfigurationsFredrikze::ANTI_PARA + "," + SpinStateConfigurationsFredrikze::ANTI_ANTI,

        SpinStateConfigurationsFredrikze::PARA_ANTI + "," + SpinStateConfigurationsFredrikze::PARA_PARA + "," +
            SpinStateConfigurationsFredrikze::ANTI_ANTI + "," + SpinStateConfigurationsFredrikze::ANTI_PARA,

        SpinStateConfigurationsFredrikze::ANTI_PARA + "," + SpinStateConfigurationsFredrikze::ANTI_ANTI + "," +
            SpinStateConfigurationsFredrikze::PARA_ANTI + "," + SpinStateConfigurationsFredrikze::PARA_PARA,

        SpinStateConfigurationsFredrikze::ANTI_ANTI + "," + SpinStateConfigurationsFredrikze::ANTI_PARA + "," +
            SpinStateConfigurationsFredrikze::PARA_PARA + "," + SpinStateConfigurationsFredrikze::PARA_ANTI};

    std::vector<std::array<std::string, 4>> expectedLogValues = {
        {SpinStatesORSO::PP, SpinStatesORSO::PM, SpinStatesORSO::MP, SpinStatesORSO::MM},
        {SpinStatesORSO::PM, SpinStatesORSO::PP, SpinStatesORSO::MM, SpinStatesORSO::MP},
        {SpinStatesORSO::MP, SpinStatesORSO::MM, SpinStatesORSO::PM, SpinStatesORSO::PP},
        {SpinStatesORSO::MM, SpinStatesORSO::MP, SpinStatesORSO::PP, SpinStatesORSO::PM}};

    // Iterate over spin state orders
    for (size_t orderIdx = 0; orderIdx < spinStateOrders.size(); ++orderIdx) {
      alg->setProperty("InputSpinStates", spinStateOrders[orderIdx]);
      alg->setProperty("OutputSpinStates", spinStateOrders[orderIdx]);
      alg->setProperty("AddSpinStateToLog", true);

      TS_ASSERT_THROWS_NOTHING(alg->execute());
      const Mantid::API::WorkspaceGroup_sptr outputWS = alg->getProperty("OutputWorkspace");
      TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), PA_GROUP_SIZE);

      // Validate logs for each workspace
      for (size_t i = 0; i < 4; i++) {
        MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(i));
        TS_ASSERT(ws);

        // Check the spin state log property
        const auto &run = ws->run();
        TS_ASSERT(run.hasProperty(SpinStatesORSO::LOG_NAME));
        TS_ASSERT_EQUALS(run.getPropertyValueAsType<std::string>(SpinStatesORSO::LOG_NAME),
                         expectedLogValues[orderIdx][i]);
      }
    }
  }

  void test_invalid_spin_state_order_for_PNR() {
    auto groupWS = createGroupWorkspace(2, 4, 1, 1);
    auto alg = initializeAlgorithm(groupWS, "PNR", "1,0,0,0");

    TSM_ASSERT_THROWS(
        "Invalid value for property InputSpinStateOrder (string) from string x, y: When setting value of property "
        "InputSpinStateOrder: The spin states must either be one or two characters, with each being either a or p",
        alg->setProperty("InputSpinStates", "x, y"), std::invalid_argument &);
    TSM_ASSERT_THROWS(
        "Invalid value for property OutputSpinStateOrder (string) from string y, x: When setting value of property "
        "OutputSpinStateOrder: The spin states must either be one or two characters, with each being either a or p",
        alg->setProperty("OutputSpinStates", "y, x"), std::invalid_argument &);
  }

  void test_default_spin_state_order_for_PNR() {
    auto groupWS = createGroupWorkspace(2, 4, 1, 1);
    auto alg = initializeAlgorithm(groupWS, "PNR", "1,0,0,0");
    alg->setProperty("AddSpinStateToLog", true);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    const Mantid::API::WorkspaceGroup_sptr outputWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), PNR_GROUP_SIZE)
    const std::array<std::string, 2> EXPECTED_LOG_VALUES{{SpinStatesORSO::PO, SpinStatesORSO::MO}};

    for (size_t i = 0; i != 2; i++) {
      MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(i));
      TS_ASSERT(ws)
      const auto &run = ws->run();
      TS_ASSERT(run.hasProperty(SpinStatesORSO::LOG_NAME))
      TS_ASSERT_EQUALS(run.getPropertyValueAsType<std::string>(SpinStatesORSO::LOG_NAME), EXPECTED_LOG_VALUES[i])
    }
  }

  void test_spin_state_not_added_to_sample_log_by_default_for_PNR() {
    auto groupWS = createGroupWorkspace(2, 4, 1, 1);
    auto alg = initializeAlgorithm(groupWS, "PNR", "1,0,0,0");
    alg->setProperty("InputSpinStates", "p, a");
    alg->setProperty("OutputSpinStates", "a, p");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    const Mantid::API::WorkspaceGroup_sptr outputWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), PNR_GROUP_SIZE)
    const std::array<std::string, 2> EXPECTED_LOG_VALUES{{SpinStatesORSO::MO, SpinStatesORSO::PO}};

    for (size_t i = 0; i != 2; i++) {
      MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(i));
      TS_ASSERT(ws)
      TS_ASSERT(!ws->run().hasProperty(SpinStatesORSO::LOG_NAME))
    }
  }

  void test_valid_spin_state_order_for_PNR() {
    auto groupWS = createGroupWorkspace(2, 4, 1, 1);
    auto eff = makeEfficiencies(create1DWorkspace(4, 1, 1), "1,0,0,0", "1,0,0,0");
    auto alg = initializeAlgorithm(groupWS, "PNR", "1,0,0,0");

    // Define spin state orders and expected log values
    std::vector<std::string> spinStateOrders = {
        SpinStateConfigurationsFredrikze::PARA + "," + SpinStateConfigurationsFredrikze::ANTI,
        SpinStateConfigurationsFredrikze::ANTI + "," + SpinStateConfigurationsFredrikze::PARA};

    std::vector<std::array<std::string, 2>> EXPECTED_LOG_VALUES = {{SpinStatesORSO::PO, SpinStatesORSO::MO},
                                                                   {SpinStatesORSO::MO, SpinStatesORSO::PO}};

    // Iterate over spin state orders
    for (size_t orderIdx = 0; orderIdx < spinStateOrders.size(); ++orderIdx) {
      alg->setProperty("InputSpinStates", spinStateOrders[orderIdx]);
      alg->setProperty("OutputSpinStates", spinStateOrders[orderIdx]);
      alg->setProperty("AddSpinStateToLog", true);

      TS_ASSERT_THROWS_NOTHING(alg->execute());
      const Mantid::API::WorkspaceGroup_sptr outputWS = alg->getProperty("OutputWorkspace");
      TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), PNR_GROUP_SIZE);

      // Validate logs for each workspace
      for (size_t i = 0; i != 2; i++) {
        MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(i));
        TS_ASSERT(ws)
        const auto &run = ws->run();
        TS_ASSERT(run.hasProperty(SpinStatesORSO::LOG_NAME))
        TS_ASSERT_EQUALS(run.getPropertyValueAsType<std::string>(SpinStatesORSO::LOG_NAME),
                         EXPECTED_LOG_VALUES[orderIdx][i])
      }
    }
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
    alg->setPropertyValue("OutputWorkspace", m_outputWSName);
    alg->execute();
    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
    return outWS;
  }

  std::shared_ptr<WorkspaceGroup> createGroupWorkspace(int numWorkspaces, int bins, int xVal, int yVal) {
    auto groupWS = std::make_shared<WorkspaceGroup>();
    for (int i = 0; i < numWorkspaces; i++) {
      groupWS->addWorkspace(create1DWorkspace(bins, xVal, yVal));
    }
    return groupWS;
  }

  std::unique_ptr<PolarizationCorrectionFredrikze>
  initializeAlgorithm(std::shared_ptr<WorkspaceGroup> groupWS, const std::string &polarizationType,
                      const std::string &efficienciesStr = "",
                      std::shared_ptr<MatrixWorkspace> efficiencies = nullptr) {
    auto alg = std::make_unique<PolarizationCorrectionFredrikze>();
    alg->setChild(true);
    alg->setRethrows(true);
    alg->initialize();
    alg->setProperty("InputWorkspace", groupWS);
    alg->setPropertyValue("OutputWorkspace", m_outputWSName);
    alg->setProperty("PolarizationAnalysis", polarizationType);
    if (efficiencies != nullptr) {
      alg->setProperty("Efficiencies", efficiencies);
    } else if (!efficienciesStr.empty()) {
      auto efficiencies = makeEfficiencies(create1DWorkspace(4, 1, 1), efficienciesStr, efficienciesStr,
                                           efficienciesStr, efficienciesStr);
      alg->setProperty("Efficiencies", efficiencies);
    }

    return alg;
  }
};
