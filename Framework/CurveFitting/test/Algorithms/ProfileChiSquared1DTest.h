// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidCurveFitting/Algorithms/ProfileChiSquared1D.h"
#include "MantidDataObjects/TableWorkspace.h"

using Mantid::CurveFitting::Algorithms::ProfileChiSquared1D;
using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace {
constexpr const char *linearFunctionString = "name = LinearBackground, A0=0.8753627851076761,  A1 = "
                                             "2.026706319695708 ";
}

class ProfileChiSquared1DTest : public CxxTest::TestSuite {
public:
  static ProfileChiSquared1DTest *createSuite() { return new ProfileChiSquared1DTest(); }
  static void destroySuite(ProfileChiSquared1DTest *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  void loadLinearData(const std::string &workspaceName) {
    auto algo = AlgorithmManager::Instance().create("Load");
    algo->setPropertyValue("Filename", "ProfileChiSquared1DData_linear.nxs");
    algo->setPropertyValue("OutputWorkspace", workspaceName);
    algo->execute();
  }

  void executeAlgorithmOnLinearData(const std::string &outputName) {
    std::string wsName = "ProfileChiSquared1DData_linear";
    loadLinearData(wsName);
    auto ws = AnalysisDataService::Instance().retrieveWS<Workspace>(wsName);
    auto functionString = std::string(linearFunctionString);
    auto profileAlg = ProfileChiSquared1D();
    profileAlg.initialize();
    profileAlg.setProperty("Function", functionString);
    profileAlg.setProperty("InputWorkspace", ws);
    profileAlg.setProperty("Output", outputName);
    profileAlg.execute();
  }

  void test_Init() {
    ProfileChiSquared1D alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_Alg_produces_expected_outputs() {
    executeAlgorithmOnLinearData("OutputName1");
    TS_ASSERT(AnalysisDataService::Instance().doesExist("OutputName1_errors"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("OutputName1_pdf"));

    // if name is empty, workspaces will use default name of ProfileChiSquared1D
    executeAlgorithmOnLinearData("");
    TS_ASSERT(AnalysisDataService::Instance().doesExist("ProfileChiSquared1D_errors"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("ProfileChiSquared1D_pdf"));

    AnalysisDataService::Instance().clear();
  }
  // the tests for these linear problems are comparing against analytical
  // calculations which can be solved in closed form for a linear function
  void test_errors_for_linear_function_are_correct() {
    executeAlgorithmOnLinearData("OutputName1");
    TableWorkspace_sptr errorsTable;
    TS_ASSERT_THROWS_NOTHING(errorsTable =
                                 AnalysisDataService::Instance().retrieveWS<TableWorkspace>("OutputName1_errors"));

    TS_ASSERT_EQUALS(errorsTable->String(0, 0), "A0");
    TS_ASSERT_EQUALS(errorsTable->String(1, 0), "A1");
    // check a0 parameter
    TS_ASSERT_DELTA(errorsTable->Double(0, 1), 0.8753627851076761, 1e-6);
    TS_ASSERT_DELTA(errorsTable->Double(0, 2), 0.8753627851076761, 1e-6);
    TS_ASSERT_DELTA(errorsTable->Double(0, 3), -0.03447782006595415, 1e-6);
    TS_ASSERT_DELTA(errorsTable->Double(0, 4), 0.03447782006595401, 1e-6);
    TS_ASSERT_DELTA(errorsTable->Double(0, 5), -0.06895564013190685, 1e-6);
    TS_ASSERT_DELTA(errorsTable->Double(0, 6), 0.06895564013190683, 1e-6);
    TS_ASSERT_DELTA(errorsTable->Double(0, 7), -0.10343346019785989, 1e-6);
    TS_ASSERT_DELTA(errorsTable->Double(0, 8), 0.1034334601978597, 1e-6);
    TS_ASSERT_DELTA(errorsTable->Double(0, 9), 0.03447782006595295, 1e-6);
    // check a1 parameter
    TS_ASSERT_DELTA(errorsTable->Double(1, 1), 2.026706319695708, 1e-6);
    TS_ASSERT_DELTA(errorsTable->Double(1, 2), 2.026706319695708, 1e-6);
    TS_ASSERT_DELTA(errorsTable->Double(1, 3), -0.006137378377995283, 1e-6);
    TS_ASSERT_DELTA(errorsTable->Double(1, 4), 0.006137378377995297, 1e-6);
    TS_ASSERT_DELTA(errorsTable->Double(1, 5), -0.012274756755989097, 1e-6);
    TS_ASSERT_DELTA(errorsTable->Double(1, 6), 0.012274756755989113, 1e-6);
    TS_ASSERT_DELTA(errorsTable->Double(1, 7), -0.01841213513398322, 1e-6);
    TS_ASSERT_DELTA(errorsTable->Double(1, 8), 0.01841213513398322, 1e-6);
    TS_ASSERT_DELTA(errorsTable->Double(1, 9), 0.006137378377994362, 1e-6);
    AnalysisDataService::Instance().clear();
  }

  void test_pdf_values_for_linear_function_are_correct() {
    executeAlgorithmOnLinearData("OutputName2");
    TableWorkspace_sptr pdfTable;
    TS_ASSERT_THROWS_NOTHING(pdfTable = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("OutputName2_pdf"));
    // check some values of pdf table
    TS_ASSERT_DELTA(pdfTable->Double(0, 0), 0.696088486717624, 1e-6);
    TS_ASSERT_DELTA(pdfTable->Double(0, 1), 27.03687327118116, 1e-2);
    TS_ASSERT_DELTA(pdfTable->Double(0, 3), 2.0007644788036028, 1e-6);
    TS_ASSERT_DELTA(pdfTable->Double(0, 4), 17.86634783053958, 1e-2);

    TS_ASSERT_DELTA(pdfTable->Double(25, 0), 0.78572563591265, 1e-6);
    TS_ASSERT_DELTA(pdfTable->Double(25, 1), 6.75921831779535, 1e-2);
    TS_ASSERT_DELTA(pdfTable->Double(25, 3), 2.0137353992496556, 1e-6);
    TS_ASSERT_DELTA(pdfTable->Double(25, 4), 4.466586957634646, 1e-2);

    TS_ASSERT_DELTA(pdfTable->Double(50, 0), 0.8753627851076761, 1e-6);
    TS_ASSERT_DELTA(pdfTable->Double(50, 1), -5.32907051820075e-14, 1e-2);
    TS_ASSERT_DELTA(pdfTable->Double(50, 3), 2.026706319695708, 1e-6);
    TS_ASSERT_DELTA(pdfTable->Double(50, 4), -3.197442310920451e-13, 1e-2);

    TS_ASSERT_DELTA(pdfTable->Double(75, 0), 0.9649999343027021, 1e-6);
    TS_ASSERT_DELTA(pdfTable->Double(75, 1), 6.759218317794955, 1e-2);
    TS_ASSERT_DELTA(pdfTable->Double(75, 3), 2.0396772401417604, 1e-6);
    TS_ASSERT_DELTA(pdfTable->Double(75, 4), 4.4665869576346875, 1e-2);

    TS_ASSERT_DELTA(pdfTable->Double(99, 0), 1.051051597529927, 1e-6);
    TS_ASSERT_DELTA(pdfTable->Double(99, 1), 25.96621308964161, 1e-2);
    TS_ASSERT_DELTA(pdfTable->Double(99, 3), 2.052129323769971, 1e-6);
    TS_ASSERT_DELTA(pdfTable->Double(99, 4), 17.15884045645028, 1e-2);

    AnalysisDataService::Instance().clear();
  }

  void test_errors_table_has_correct_shape() {
    executeAlgorithmOnLinearData("OutputName3");
    TableWorkspace_sptr errorsTable;
    TS_ASSERT_THROWS_NOTHING(errorsTable =
                                 AnalysisDataService::Instance().retrieveWS<TableWorkspace>("OutputName3_errors"));
    TS_ASSERT_EQUALS(errorsTable->columnCount(), 10);
    TS_ASSERT_EQUALS(errorsTable->rowCount(), 2);
    AnalysisDataService::Instance().clear();
  }

  void test_pdf_table_has_correct_shape() {
    executeAlgorithmOnLinearData("OutputName4");
    TableWorkspace_sptr pdfTable;
    TS_ASSERT_THROWS_NOTHING(pdfTable = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("OutputName4_pdf"));
    TS_ASSERT_EQUALS(pdfTable->columnCount(), 6);
    TS_ASSERT_EQUALS(pdfTable->rowCount(), 100);
    AnalysisDataService::Instance().clear();
  };
};