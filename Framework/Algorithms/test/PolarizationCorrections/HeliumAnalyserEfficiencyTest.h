// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/PolarizationCorrections/HeliumAnalyserEfficiency.h"
#include <boost/format.hpp>
#include <cmath>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class HeliumAnalyserEfficiencyTest : public CxxTest::TestSuite {
public:
  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void testName() {
    HeliumAnalyserEfficiency alg;
    TS_ASSERT_EQUALS(alg.name(), "HeliumAnalyserEfficiency");
  }

  void testInit() {
    HeliumAnalyserEfficiency alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
  }

  void testInputWorkspaceNotAGroupThrows() {
    // Should accept a group workspace containing four workspaces, corresponding to the four spin configurations
    std::vector<double> x{1, 2, 3, 4, 5};
    std::vector<double> y{1, 4, 9, 16, 25};
    MatrixWorkspace_sptr ws1 = generateWorkspace("ws1", x, y);
    auto heliumAnalyserEfficiency = AlgorithmManager::Instance().create("HeliumAnalyserEfficiency");
    heliumAnalyserEfficiency->initialize();
    heliumAnalyserEfficiency->setProperty("OutputWorkspace", "P");
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->setProperty("InputWorkspace", ws1), std::invalid_argument &);
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->execute(), const std::runtime_error &);
  }

  void testInputWorkspaceWithWrongSizedGroupThrows() {
    // Should accept a group workspace containing four workspaces, corresponding to the four spin configurations
    std::vector<double> x{1, 2, 3, 4, 5};
    std::vector<double> y{1, 4, 9, 16, 25};
    MatrixWorkspace_sptr ws1 = generateWorkspace("ws1", x, y);
    MatrixWorkspace_sptr ws2 = generateWorkspace("ws2", x, y);
    MatrixWorkspace_sptr ws3 = generateWorkspace("ws3", x, y);
    auto groupWs = groupWorkspaces("grp", std::vector<MatrixWorkspace_sptr>{ws1, ws2, ws3});
    auto heliumAnalyserEfficiency = createHeliumAnalyserEfficiencyAlgorithm(groupWs, "P");
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->execute(), const std::runtime_error &);
  }

  void testInvalidSpinStateFormatThrowsError() {
    auto heliumAnalyserEfficiency = AlgorithmManager::Instance().create("HeliumAnalyserEfficiency");
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->setProperty("SpinStates", "bad"), std::invalid_argument &);
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->setProperty("SpinStates", "10,01"), std::invalid_argument &);
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->setProperty("SpinStates", "00,00,11,11"), std::invalid_argument &);
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->setProperty("SpinStates", "02,20,22,00"), std::invalid_argument &);
  }

  void testNonWavelengthInput() {
    // The units of the input workspace should be wavelength
    MantidVec e;
    auto wsGrp = createExampleGroupWorkspace("wsGrp", e, "TOF");
    auto heliumAnalyserEfficiency = createHeliumAnalyserEfficiencyAlgorithm(wsGrp, "out");

    TS_ASSERT_THROWS_EQUALS(
        heliumAnalyserEfficiency->execute(), std::runtime_error const &e, std::string(e.what()),
        "Some invalid Properties found: \n InputWorkspace: All input workspaces must be in units of Wavelength.");
  }

  void testInputWorkspaceNotSingleSpectrumThrowsError() {
    MantidVec e;
    auto wsGrp = createExampleGroupWorkspace("wsGrp", e, "Wavelength", 10, 0.2, 2);
    auto heliumAnalyserEfficiency = createHeliumAnalyserEfficiencyAlgorithm(wsGrp, "out");

    TS_ASSERT_THROWS_EQUALS(
        heliumAnalyserEfficiency->execute(), std::runtime_error const &e, std::string(e.what()),
        "Some invalid Properties found: \n InputWorkspace: All input workspaces must contain a single histogram.");
  }

  void testInputWorkspaceNotHistogramDataThrowsError() {
    MantidVec e;
    auto wsGrp = createExampleGroupWorkspace("wsGrp", e, "Wavelength");

    MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(wsGrp->getItem(0));
    auto convert = AlgorithmManager::Instance().create("ConvertToPointData");
    convert->initialize();
    convert->setProperty("InputWorkspace", ws);
    convert->setProperty("OutputWorkspace", ws->getName());
    convert->execute();

    auto heliumAnalyserEfficiency = createHeliumAnalyserEfficiencyAlgorithm(wsGrp, "out");

    TS_ASSERT_THROWS_EQUALS(
        heliumAnalyserEfficiency->execute(), std::runtime_error const &e, std::string(e.what()),
        "Some invalid Properties found: \n InputWorkspace: All input workspaces must be histogram data.");
  }

  void testZeroPdError() {
    compareOutputValues(0, {0.4845053416, 0.6550113464, 0.6525155755, 0.5478694489, 0.4142358259});
  }

  void testNonZeroPdError() {
    compareOutputValues(1000, {14.8320754089, 20.6772556357, 20.7518789689, 17.4738260412, 13.2301633100});
  }

  void testSmallNumberOfBins() {
    // With less than 3 bins it's not possible to perform the error calculation correctly, because the
    // number of parameters exceeds the number of data points.
    MantidVec e;
    auto wsGrp = createExampleGroupWorkspace("wsGrp", e, "Wavelength", 2);
    auto heliumAnalyserEfficiency = createHeliumAnalyserEfficiencyAlgorithm(wsGrp, "P");
    heliumAnalyserEfficiency->execute();
    TS_ASSERT_EQUALS(true, heliumAnalyserEfficiency->isExecuted());
  }

  void testCorrectNumberOfOutputBins() {
    MantidVec e;
    auto wsGrp = createExampleGroupWorkspace("wsGrp", e, "Wavelength");
    auto heliumAnalyserEfficiency = createHeliumAnalyserEfficiencyAlgorithm(wsGrp, "E");
    heliumAnalyserEfficiency->setProperty("StartX", 4.0);
    heliumAnalyserEfficiency->setProperty("EndX", 6.0);
    heliumAnalyserEfficiency->setProperty("IgnoreFitQualityError", true);
    heliumAnalyserEfficiency->execute();
    TS_ASSERT(heliumAnalyserEfficiency->isExecuted());
    MatrixWorkspace_sptr eff = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("E");
    MatrixWorkspace_sptr firstWs = std::dynamic_pointer_cast<MatrixWorkspace>(wsGrp->getItem(0));
    // The output number of wavelength bins should match those from the input.
    TS_ASSERT_EQUALS(firstWs->blocksize(), eff->blocksize());
  }

  void testFitCurvesOutputWhenOptionalPropertySet() {
    // GIVEN
    MantidVec e;
    const WorkspaceGroup_sptr &wsGrp = createExampleGroupWorkspace("wsGrp", e, "Wavelength");
    auto alg = createHeliumAnalyserEfficiencyAlgorithm(wsGrp, "E");

    // WHEN
    alg->setPropertyValue("OutputFitCurves", "__unused_for_child");
    alg->setChild(true);
    alg->execute();

    // THEN
    TS_ASSERT(alg->isExecuted());
    const MatrixWorkspace_sptr &outputWs = alg->getProperty("OutputWorkspace");
    const MatrixWorkspace_sptr &fitWs = alg->getProperty("OutputFitCurves");
    TS_ASSERT_EQUALS(outputWs->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(fitWs->getNumberHistograms(), 3);
  }

  void testParametersTableOutputWhenOptionalPropertySet() {
    // GIVEN
    MantidVec e;
    const WorkspaceGroup_sptr &wsGrp = createExampleGroupWorkspace("wsGrp", e, "Wavelength");
    auto alg = createHeliumAnalyserEfficiencyAlgorithm(wsGrp, "E");

    // WHEN
    alg->setPropertyValue("OutputFitParameters", "__unused_for_child");
    alg->setChild(true);
    alg->execute();

    // THEN
    TS_ASSERT(alg->isExecuted());
    const MatrixWorkspace_sptr &outputWs = alg->getProperty("OutputWorkspace");
    const ITableWorkspace_sptr &paramsWs = alg->getProperty("OutputFitParameters");
    TS_ASSERT_EQUALS(outputWs->getNumberHistograms(), 1);
    const std::vector<std::string> &expectedColumns = {"Name", "Value", "Error"};
    TS_ASSERT_EQUALS(paramsWs->getColumnNames(), expectedColumns);
  }

  void testChildAlgorithmExecutesSuccessfully() {
    MantidVec e;
    auto wsGrp = createExampleGroupWorkspace("wsGrp", e);

    auto heliumAnalyserEfficiency = createHeliumAnalyserEfficiencyAlgorithm(wsGrp, "E");
    heliumAnalyserEfficiency->setChild(true);

    heliumAnalyserEfficiency->execute();

    TS_ASSERT(heliumAnalyserEfficiency->isExecuted());

    MatrixWorkspace_sptr outputWorkspace = heliumAnalyserEfficiency->getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(outputWorkspace->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outputWorkspace->dataY(0).size(), e.size());
  }

private:
  IAlgorithm_sptr createHeliumAnalyserEfficiencyAlgorithm(WorkspaceGroup_sptr inputWs,
                                                          const std::string &outputWsName) {
    auto heliumAnalyserEfficiency = AlgorithmManager::Instance().create("HeliumAnalyserEfficiency");
    heliumAnalyserEfficiency->initialize();
    heliumAnalyserEfficiency->setProperty("InputWorkspace", inputWs->getName());
    heliumAnalyserEfficiency->setProperty("OutputWorkspace", outputWsName);
    return heliumAnalyserEfficiency;
  }

  WorkspaceGroup_sptr createExampleGroupWorkspace(const std::string &name, MantidVec &expectedEfficiency,
                                                  const std::string &xUnit = "Wavelength", const size_t numBins = 5,
                                                  const double examplePHe = 0.2, const int nSpec = 1) {
    std::vector<double> x(numBins);
    std::vector<double> yNsf(numBins);
    std::vector<double> ySf(numBins);
    for (size_t i = 0; i < numBins; ++i) {
      x[i] = 2.0 + static_cast<double>(i) * 8.0 / static_cast<double>(numBins);
      yNsf[i] = 0.9 * std::exp(-0.0733 * x[i] * 12 * (1 - examplePHe));
      ySf[i] = 0.9 * std::exp(-0.0733 * x[i] * 12 * (1 + examplePHe));
    }
    std::vector<MatrixWorkspace_sptr> wsVec(4);
    wsVec[0] = generateWorkspace("ws0", x, yNsf, xUnit, nSpec);
    wsVec[1] = generateWorkspace("ws1", x, ySf, xUnit, nSpec);
    wsVec[2] = generateWorkspace("ws2", x, ySf, xUnit, nSpec);
    wsVec[3] = generateWorkspace("ws3", x, yNsf, xUnit, nSpec);

    const auto histPoints = wsVec[0]->histogram(0).points();
    const auto &wavelengthPoints = histPoints.rawData();
    expectedEfficiency.resize(wavelengthPoints.size());
    for (size_t i = 0; i < wavelengthPoints.size(); ++i) {
      yNsf[i] = 0.9 * std::exp(-0.0733 * wavelengthPoints[i] * 12 * (1 - examplePHe));
      ySf[i] = 0.9 * std::exp(-0.0733 * wavelengthPoints[i] * 12 * (1 + examplePHe));
      expectedEfficiency[i] = yNsf[i] / (yNsf[i] + ySf[i]);
    }

    return groupWorkspaces(name, wsVec);
  }

  MatrixWorkspace_sptr generateWorkspace(const std::string &name, const std::vector<double> &x,
                                         const std::vector<double> &y, const std::string &xUnit = "Wavelength",
                                         const int nSpec = 1) {
    auto createWorkspace = AlgorithmManager::Instance().create("CreateWorkspace");
    createWorkspace->initialize();
    createWorkspace->setProperty("DataX", x);
    createWorkspace->setProperty("DataY", y);
    createWorkspace->setProperty("UnitX", xUnit);
    createWorkspace->setProperty("NSpec", nSpec);
    createWorkspace->setProperty("OutputWorkspace", name);
    createWorkspace->execute();

    auto convertToHistogram = AlgorithmManager::Instance().create("ConvertToHistogram");
    convertToHistogram->initialize();
    convertToHistogram->setProperty("InputWorkspace", name);
    convertToHistogram->setProperty("OutputWorkspace", name);
    convertToHistogram->execute();

    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name);
    return ws;
  }

  WorkspaceGroup_sptr groupWorkspaces(const std::string &name, const std::vector<MatrixWorkspace_sptr> &wsToGroup) {
    auto groupWorkspace = AlgorithmManager::Instance().create("GroupWorkspaces");
    groupWorkspace->initialize();
    std::vector<std::string> wsToGroupNames(wsToGroup.size());
    std::transform(wsToGroup.cbegin(), wsToGroup.cend(), wsToGroupNames.begin(),
                   [](MatrixWorkspace_sptr w) { return w->getName(); });
    groupWorkspace->setProperty("InputWorkspaces", wsToGroupNames);
    groupWorkspace->setProperty("OutputWorkspace", name);
    groupWorkspace->execute();
    WorkspaceGroup_sptr group = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(name);
    return group;
  }

  MatrixWorkspace_sptr generateFunctionDefinedWorkspace(const std::string &name, const std::string &func) {
    auto createSampleWorkspace = AlgorithmManager::Instance().create("CreateSampleWorkspace");
    createSampleWorkspace->initialize();
    createSampleWorkspace->setProperty("WorkspaceType", "Histogram");
    createSampleWorkspace->setProperty("OutputWorkspace", name);
    createSampleWorkspace->setProperty("Function", "User Defined");
    createSampleWorkspace->setProperty("UserDefinedFunction", "name=UserFunction,Formula=" + func);
    createSampleWorkspace->setProperty("XUnit", "Wavelength");
    createSampleWorkspace->setProperty("XMin", "1");
    createSampleWorkspace->setProperty("XMax", "8");
    createSampleWorkspace->setProperty("BinWidth", "1");
    createSampleWorkspace->execute();

    MatrixWorkspace_sptr result = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name);
    return result;
  }

  void compareOutputValues(const double pdError, const MantidVec &expectedErrorValues) {
    MantidVec expectedEfficiencies;
    auto wsGrp = createExampleGroupWorkspace("wsGrp", expectedEfficiencies);
    auto heliumAnalyserEfficiency = createHeliumAnalyserEfficiencyAlgorithm(wsGrp, "E");
    heliumAnalyserEfficiency->setProperty("PXDError", pdError);
    heliumAnalyserEfficiency->execute();

    TS_ASSERT(heliumAnalyserEfficiency->isExecuted());

    MatrixWorkspace_sptr efficiency = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        heliumAnalyserEfficiency->getPropertyValue("OutputWorkspace"));
    const auto efficiencies = efficiency->dataY(0);
    const auto error = efficiency->dataE(0);

    TS_ASSERT_EQUALS(expectedEfficiencies.size(), efficiencies.size());
    TS_ASSERT_EQUALS(expectedErrorValues.size(), error.size());
    for (size_t i = 0; i < error.size(); ++i) {
      TS_ASSERT_DELTA(expectedEfficiencies[i], efficiencies[i], 1e-7);
      TS_ASSERT_DELTA(expectedErrorValues[i], error[i], 1e-7);
    }
  }
};
