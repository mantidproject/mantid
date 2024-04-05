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

  void testInputWorkspaceFormat() {
    // Should accept a group workspace containing four workspaces, corresponding to the four spin configurations
    std::vector<double> x{1, 2, 3, 4, 5};
    std::vector<double> y{1, 4, 9, 16, 25};

    MatrixWorkspace_sptr ws1 = generateWorkspace("ws1", x, y);

    auto heliumAnalyserEfficiency = AlgorithmManager::Instance().create("HeliumAnalyserEfficiency");
    heliumAnalyserEfficiency->initialize();
    heliumAnalyserEfficiency->setProperty("InputWorkspace", ws1->getName());
    heliumAnalyserEfficiency->setProperty("OutputWorkspace", "P");
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->execute(), const std::runtime_error &);

    MatrixWorkspace_sptr ws2 = generateWorkspace("ws2", x, y);
    WorkspaceGroup_sptr groupWs = groupWorkspaces("grp", std::vector<MatrixWorkspace_sptr>{ws1, ws2});
    heliumAnalyserEfficiency->initialize();
    heliumAnalyserEfficiency->setProperty("InputWorkspace", groupWs->getName());
    heliumAnalyserEfficiency->setProperty("OutputWorkspace", "P");
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->execute(), const std::runtime_error &);

    MatrixWorkspace_sptr ws3 = generateWorkspace("ws3", x, y);
    groupWs = groupWorkspaces("grp", std::vector<MatrixWorkspace_sptr>{ws1, ws2, ws3});
    heliumAnalyserEfficiency->initialize();
    heliumAnalyserEfficiency->setProperty("InputWorkspace", groupWs->getName());
    heliumAnalyserEfficiency->setProperty("OutputWorkspace", "P");
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->execute(), const std::runtime_error &);

    MatrixWorkspace_sptr ws4 = generateWorkspace("ws4", x, y);
    groupWs = groupWorkspaces("grp", std::vector<MatrixWorkspace_sptr>{ws1, ws2, ws3, ws4});
    heliumAnalyserEfficiency->initialize();
    heliumAnalyserEfficiency->setProperty("InputWorkspace", groupWs->getName());
    heliumAnalyserEfficiency->setProperty("OutputWorkspace", "P");
    TS_ASSERT_THROWS_NOTHING(heliumAnalyserEfficiency->execute());
  }

  void testOutputs() {
    auto wsGrp = createExampleGroupWorkspace("wsGrp");

    auto heliumAnalyserEfficiency = AlgorithmManager::Instance().create("HeliumAnalyserEfficiency");
    heliumAnalyserEfficiency->initialize();
    heliumAnalyserEfficiency->setProperty("InputWorkspace", wsGrp->getName());
    heliumAnalyserEfficiency->setProperty("OutputWorkspace", "P");
    heliumAnalyserEfficiency->execute();

    const std::string t = "T";
    const std::string pHe = "p_He";
    const std::string tPara = "T_para";
    const std::string tAnti = "T_anti";

    ASSERT_FALSE(AnalysisDataService::Instance().doesExist(t));
    ASSERT_FALSE(AnalysisDataService::Instance().doesExist(pHe));
    ASSERT_FALSE(AnalysisDataService::Instance().doesExist(tPara));
    ASSERT_FALSE(AnalysisDataService::Instance().doesExist(tAnti));

    heliumAnalyserEfficiency->initialize();
    heliumAnalyserEfficiency->setProperty("InputWorkspace", wsGrp->getName());
    heliumAnalyserEfficiency->setProperty("OutputWorkspace", "P");
    heliumAnalyserEfficiency->setProperty("OutputTransmissionWorkspace", t);
    heliumAnalyserEfficiency->setProperty("HeliumAtomsPolarization", pHe);
    heliumAnalyserEfficiency->setProperty("OutputTransmissionParaWorkspace", tPara);
    heliumAnalyserEfficiency->setProperty("OutputTransmissionAntiWorkspace", tAnti);
    heliumAnalyserEfficiency->execute();

    const auto workspacesInAds = AnalysisDataService::Instance().getObjectNames();

    ASSERT_TRUE(std::find(workspacesInAds.cbegin(), workspacesInAds.cend(), t) != workspacesInAds.cend());
    ASSERT_TRUE(std::find(workspacesInAds.cbegin(), workspacesInAds.cend(), pHe) != workspacesInAds.cend());
    ASSERT_TRUE(std::find(workspacesInAds.cbegin(), workspacesInAds.cend(), tPara) != workspacesInAds.cend());
    ASSERT_TRUE(std::find(workspacesInAds.cbegin(), workspacesInAds.cend(), tAnti) != workspacesInAds.cend());
    const std::string outputWorkspace = heliumAnalyserEfficiency->getProperty("OutputWorkspace");
    ASSERT_TRUE(std::find(workspacesInAds.cbegin(), workspacesInAds.cend(), outputWorkspace) != workspacesInAds.cend());
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
    auto wsGrp = createExampleGroupWorkspace("wsGrp", "TOF");
    auto heliumAnalyserEfficiency = AlgorithmManager::Instance().create("HeliumAnalyserEfficiency");
    heliumAnalyserEfficiency->initialize();
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->setProperty("InputWorkspace", wsGrp->getName()),
                     std::invalid_argument &);
  }

  void testSampleFit() {
    // For a p of e.g. p=0.1, we're going to generate some spin flipped and non-spin flippped
    // data and fit to it. We should recover the original p.
    const double p = 0.1;
    const double pxd = 12;
    const double mu = pxd * 0.0733;
    const double te = 1;

    const boost::format nsfFunc = boost::format("0.5*%1%*exp(-x*%2%*(1-%3%))") % te % mu % p;
    const boost::format sfFunc = boost::format("0.5*%1%*exp(-x*%2%*(1+%3%))") % te % mu % p;

    auto nsfWs1 = generateFunctionDefinedWorkspace("nsfWs1", nsfFunc.str());
    auto nsfWs2 = generateFunctionDefinedWorkspace("nsfWs2", nsfFunc.str());
    auto sfWs1 = generateFunctionDefinedWorkspace("sfWs1", sfFunc.str());
    auto sfWs2 = generateFunctionDefinedWorkspace("sfWs2", sfFunc.str());
    auto grpWs = groupWorkspaces("grpWs", {nsfWs1, sfWs1, nsfWs2, sfWs2});

    auto heliumAnalyserEfficiency = AlgorithmManager::Instance().create("HeliumAnalyserEfficiency");
    heliumAnalyserEfficiency->initialize();
    heliumAnalyserEfficiency->setProperty("InputWorkspace", grpWs->getName());
    heliumAnalyserEfficiency->setProperty("OutputWorkspace", "P");
    heliumAnalyserEfficiency->setProperty("SpinStates", "11,10,00,01");
    heliumAnalyserEfficiency->setProperty("TransmissionEmptyCell", te);
    heliumAnalyserEfficiency->setProperty("GasPressureTimesCellLength", pxd);
    heliumAnalyserEfficiency->setProperty("HeliumAtomsPolarization", "p_He");
    heliumAnalyserEfficiency->execute();

    MatrixWorkspace_sptr pHeWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("p_He");
    auto pFromFit = pHeWs->y(0)[0];
    TS_ASSERT_DELTA(pFromFit, p, 1e-5);
  }

  void testIncorrectCovarianceMatrix() {
    auto wsGrp = createExampleGroupWorkspace("wsGrp");
    ITableWorkspace_sptr covariance = createBlankCovarianceMatrix();

    // Test covariance matrix with incorrect size
    auto heliumAnalyserEfficiency = AlgorithmManager::Instance().create("HeliumAnalyserEfficiency");
    heliumAnalyserEfficiency->initialize();
    heliumAnalyserEfficiency->setProperty("InputWorkspace", wsGrp->getName());
    heliumAnalyserEfficiency->setProperty("OutputWorkspace", "P");
    heliumAnalyserEfficiency->setProperty("Covariance", covariance);
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->execute(), std::runtime_error &);
  }

  void testCorrectCovarianceMatrix() {
    auto wsGrp = createExampleGroupWorkspace("wsGrp");
    ITableWorkspace_sptr covariance = createBlankCovarianceMatrix();

    // Test with correctly sized covariance matrix
    covariance->appendRow();
    for (size_t i = 0; i < 2; ++i)
      for (size_t j = 1; j < 3; ++j) {
        covariance->cell<double>(i, j) = 5;
      }

    auto heliumAnalyserEfficiency = AlgorithmManager::Instance().create("HeliumAnalyserEfficiency");
    heliumAnalyserEfficiency->initialize();
    heliumAnalyserEfficiency->setProperty("InputWorkspace", wsGrp->getName());
    heliumAnalyserEfficiency->setProperty("OutputWorkspace", "P");
    heliumAnalyserEfficiency->setProperty("Covariance", covariance);
    heliumAnalyserEfficiency->execute();
    TS_ASSERT_EQUALS(true, heliumAnalyserEfficiency->isExecuted());
  }

  void testZeroCovarianceMatrix() {
    auto wsGrp = createExampleGroupWorkspace("wsGrp");
    ITableWorkspace_sptr covariance = createBlankCovarianceMatrix();
    covariance->appendRow();
    for (size_t i = 0; i < 2; ++i)
      for (size_t j = 1; j < 3; ++j) {
        covariance->cell<double>(i, j) = 0;
      }

    auto heliumAnalyserEfficiency = AlgorithmManager::Instance().create("HeliumAnalyserEfficiency");
    heliumAnalyserEfficiency->initialize();
    heliumAnalyserEfficiency->setProperty("InputWorkspace", wsGrp->getName());
    heliumAnalyserEfficiency->setProperty("OutputWorkspace", "e");
    heliumAnalyserEfficiency->setProperty("AnalyserPolarization", "P");
    heliumAnalyserEfficiency->setProperty("Covariance", covariance);
    heliumAnalyserEfficiency->execute();
    TS_ASSERT_EQUALS(true, heliumAnalyserEfficiency->isExecuted());

    MatrixWorkspace_sptr p = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        heliumAnalyserEfficiency->getProperty("AnalyserPolarization"));
    const auto pErrors = p->dataE(0);
    const MantidVec expectedPErrors = {0.67084648224713839, 0.77009548972929842, 0.69973631914219236,
                                       0.55523214443723223};
    TS_ASSERT_EQUALS(expectedPErrors.size(), pErrors.size());
    for (size_t i = 0; i < pErrors.size(); ++i) {
      TS_ASSERT_DELTA(expectedPErrors[i], pErrors[i], 1e-8);
    }
  }

  void testNonZeroCovarianceMatrix() {
    auto wsGrp = createExampleGroupWorkspace("wsGrp");
    ITableWorkspace_sptr covariance = createBlankCovarianceMatrix();
    covariance->appendRow();
    for (size_t i = 0; i < 2; ++i)
      for (size_t j = 1; j < 3; ++j) {
        covariance->cell<double>(i, j) = 1000;
      }

    auto heliumAnalyserEfficiency = AlgorithmManager::Instance().create("HeliumAnalyserEfficiency");
    heliumAnalyserEfficiency->initialize();
    heliumAnalyserEfficiency->setProperty("InputWorkspace", wsGrp->getName());
    heliumAnalyserEfficiency->setProperty("OutputWorkspace", "e");
    heliumAnalyserEfficiency->setProperty("AnalyserPolarization", "P");
    heliumAnalyserEfficiency->setProperty("Covariance", covariance);
    heliumAnalyserEfficiency->execute();
    TS_ASSERT_EQUALS(true, heliumAnalyserEfficiency->isExecuted());

    MatrixWorkspace_sptr p = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        heliumAnalyserEfficiency->getProperty("AnalyserPolarization"));
    const auto pErrors = p->dataE(0);
    const MantidVec expectedPErrors = {1.5147094711224480, 1.7388045742661027, 1.5799400577352583, 1.2536629618054320};
    TS_ASSERT_EQUALS(expectedPErrors.size(), pErrors.size());
    for (size_t i = 0; i < pErrors.size(); ++i) {
      TS_ASSERT_DELTA(expectedPErrors[i], pErrors[i], 1e-8);
    }
  }

  void testSmallNumberOfBins() {
    // With less than 4 bins it's not possible to perform the error calculation correctly, because the
    // number of parameters exceeds the number of data points.
    auto wsGrp = createExampleGroupWorkspace("wsGrp", "Wavelength", 3);

    ITableWorkspace_sptr covariance = WorkspaceFactory::Instance().createTable();

    covariance->addColumn("str", "Name");
    covariance->addColumn("double", "a");
    covariance->addColumn("double", "b");
    covariance->appendRow();
    covariance->appendRow();

    auto heliumAnalyserEfficiency = AlgorithmManager::Instance().create("HeliumAnalyserEfficiency");
    heliumAnalyserEfficiency->initialize();
    heliumAnalyserEfficiency->setProperty("InputWorkspace", wsGrp->getName());
    heliumAnalyserEfficiency->setProperty("OutputWorkspace", "P");
    heliumAnalyserEfficiency->setProperty("Covariance", covariance);
    heliumAnalyserEfficiency->execute();
    TS_ASSERT_EQUALS(true, heliumAnalyserEfficiency->isExecuted());
  }

  void testEfficiencyOrder() {
    auto wsGrp = createExampleGroupWorkspace("wsGrp");
    auto heliumAnalyserEfficiency = AlgorithmManager::Instance().create("HeliumAnalyserEfficiency");
    heliumAnalyserEfficiency->initialize();
    heliumAnalyserEfficiency->setProperty("InputWorkspace", wsGrp->getName());
    heliumAnalyserEfficiency->setProperty("OutputWorkspace", "efficiencies");
    heliumAnalyserEfficiency->setProperty("SpinStates", "00,01,10,11");
    heliumAnalyserEfficiency->execute();
    WorkspaceGroup_sptr efficiencies = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
        heliumAnalyserEfficiency->getProperty("OutputWorkspace"));
    TS_ASSERT_EQUALS("efficiencies00", efficiencies->getItem(0)->getName());
    TS_ASSERT_EQUALS("efficiencies01", efficiencies->getItem(1)->getName());
    TS_ASSERT_EQUALS("efficiencies10", efficiencies->getItem(2)->getName());
    TS_ASSERT_EQUALS("efficiencies11", efficiencies->getItem(3)->getName());
  }

  void testParallelAndAntiEfficienciesAreSame() {
    auto wsGrp = createExampleGroupWorkspace("wsGrp");
    auto heliumAnalyserEfficiency = AlgorithmManager::Instance().create("HeliumAnalyserEfficiency");
    heliumAnalyserEfficiency->initialize();
    heliumAnalyserEfficiency->setProperty("InputWorkspace", wsGrp->getName());
    heliumAnalyserEfficiency->setProperty("OutputWorkspace", "efficiencies");
    heliumAnalyserEfficiency->execute();
    WorkspaceGroup_sptr efficiencies = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
        heliumAnalyserEfficiency->getProperty("OutputWorkspace"));
    // Default spin state order is 11,10,01,00
    auto ws11 = std::dynamic_pointer_cast<MatrixWorkspace>(efficiencies->getItem(0));
    auto ws10 = std::dynamic_pointer_cast<MatrixWorkspace>(efficiencies->getItem(1));
    auto ws01 = std::dynamic_pointer_cast<MatrixWorkspace>(efficiencies->getItem(2));
    auto ws00 = std::dynamic_pointer_cast<MatrixWorkspace>(efficiencies->getItem(3));

    TS_ASSERT_EQUALS(ws11->dataX(0).size(), ws00->dataX(0).size());
    TS_ASSERT_EQUALS(ws10->dataX(0).size(), ws01->dataX(0).size());

    for (size_t i = 0; i < ws11->dataY(0).size(); ++i) {
      TS_ASSERT_DELTA(ws11->dataY(0)[i], ws00->dataY(0)[i], 1e-8);
      TS_ASSERT_DELTA(ws10->dataY(0)[i], ws01->dataY(0)[i], 1e-8);
    }
  }

  void testEfficiencyNumbers() {
    auto wsGrp = createExampleGroupWorkspace("wsGrp");
    auto heliumAnalyserEfficiency = AlgorithmManager::Instance().create("HeliumAnalyserEfficiency");
    heliumAnalyserEfficiency->initialize();
    heliumAnalyserEfficiency->setProperty("InputWorkspace", wsGrp->getName());
    heliumAnalyserEfficiency->setProperty("OutputWorkspace", "efficiencies");
    heliumAnalyserEfficiency->execute();
    WorkspaceGroup_sptr efficiencies = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
        heliumAnalyserEfficiency->getProperty("OutputWorkspace"));
    // Default spin state order is 11,10,01,00
    auto ws11 = std::dynamic_pointer_cast<MatrixWorkspace>(efficiencies->getItem(0));
    auto ws10 = std::dynamic_pointer_cast<MatrixWorkspace>(efficiencies->getItem(1));
    const auto yPara = ws11->dataY(0);
    const auto yAnti = ws10->dataY(0);
    const auto expectedYPara =
        std::vector<double>{0.78016428467325061, 0.86171025529258294, 0.91625452097905313, 0.95052072166210300};
    const auto expectedYAnti =
        std::vector<double>{0.21983571532674945, 0.13828974470741706, 0.083745479020946867, 0.049479278337896948};
    for (size_t i = 0; i < 5; ++i) {
      TS_ASSERT_DELTA(expectedYPara[i], yPara[i], 1e-8);
      TS_ASSERT_DELTA(expectedYAnti[i], yAnti[i], 1e-8);
    }
  }

private:
  ITableWorkspace_sptr createBlankCovarianceMatrix() {
    ITableWorkspace_sptr covariance = WorkspaceFactory::Instance().createTable();

    covariance->addColumn("str", "Name");
    covariance->addColumn("double", "a");
    covariance->addColumn("double", "b");
    covariance->appendRow();
    return covariance;
  }

  WorkspaceGroup_sptr createExampleGroupWorkspace(const std::string &name, const std::string &xUnit = "Wavelength",
                                                  const size_t numBins = 5, const double examplePHe = 0.2) {
    std::vector<double> x(numBins);
    std::vector<double> yNsf(numBins);
    std::vector<double> ySf(numBins);
    for (size_t i = 0; i < numBins; ++i) {
      x[i] = 2.0 + i * 8.0 / numBins;
      yNsf[i] = 0.9 * std::exp(-0.0733 * x[i] * 12 * (1 - examplePHe));
      ySf[i] = 0.9 * std::exp(-0.0733 * x[i] * 12 * (1 + examplePHe));
    }
    std::vector<MatrixWorkspace_sptr> wsVec(4);
    wsVec[0] = generateWorkspace("ws0", x, yNsf, xUnit);
    wsVec[1] = generateWorkspace("ws1", x, ySf, xUnit);
    wsVec[2] = generateWorkspace("ws2", x, ySf, xUnit);
    wsVec[3] = generateWorkspace("ws3", x, yNsf, xUnit);
    return groupWorkspaces(name, wsVec);
  }

  MatrixWorkspace_sptr generateWorkspace(const std::string &name, const std::vector<double> &x,
                                         const std::vector<double> &y, const std::string &xUnit = "Wavelength") {
    auto createWorkspace = AlgorithmManager::Instance().create("CreateWorkspace");
    createWorkspace->initialize();
    createWorkspace->setProperty("DataX", x);
    createWorkspace->setProperty("DataY", y);
    createWorkspace->setProperty("UnitX", xUnit);
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
};