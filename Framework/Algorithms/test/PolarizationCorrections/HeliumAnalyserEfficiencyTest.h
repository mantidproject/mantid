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

    ASSERT_TRUE(AnalysisDataService::Instance().doesExist(t));
    AnalysisDataService::Instance().remove(t);
    ASSERT_TRUE(AnalysisDataService::Instance().doesExist(pHe));
    AnalysisDataService::Instance().remove(pHe);
    ASSERT_TRUE(AnalysisDataService::Instance().doesExist(tPara));
    AnalysisDataService::Instance().remove(tPara);
    ASSERT_TRUE(AnalysisDataService::Instance().doesExist(tAnti));
    AnalysisDataService::Instance().remove(tAnti);

    auto members = wsGrp->getNames();
    AnalysisDataService::Instance().remove(wsGrp->getName());
    for (size_t i = 0; i < members.size(); ++i) {
      AnalysisDataService::Instance().remove(members[i]);
    }

    AnalysisDataService::Instance().remove(heliumAnalyserEfficiency->getProperty("OutputWorkspace"));

    TS_ASSERT_EQUALS(0, AnalysisDataService::Instance().size());
  }

  void testSpinStates() {
    auto heliumAnalyserEfficiency = AlgorithmManager::Instance().create("HeliumAnalyserEfficiency");
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->setProperty("SpinStates", "bad"), std::invalid_argument &);
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->setProperty("SpinStates", "10,01"), std::invalid_argument &);
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->setProperty("SpinStates", "00,00,11,11"), std::invalid_argument &);
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->setProperty("SpinStates", "02,20,22,00"), std::invalid_argument &);
    TS_ASSERT_THROWS_NOTHING(heliumAnalyserEfficiency->setProperty("SpinStates", "00,11,01,10"));
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

  ITableWorkspace_sptr createBlankCovarianceMatrix() {
    ITableWorkspace_sptr covariance = WorkspaceFactory::Instance().createTable();

    covariance->addColumn("str", "Name");
    covariance->addColumn("double", "a");
    covariance->addColumn("double", "b");
    covariance->appendRow();
    return covariance;
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

  WorkspaceGroup_sptr createExampleGroupWorkspace(const std::string &name, const std::string &xUnit = "Wavelength",
                                                  const size_t numBins = 5) {
    std::vector<double> x(numBins);
    std::vector<double> y(numBins);
    for (size_t i = 0; i < numBins; ++i) {
      x[i] = static_cast<double>(i) + 1.0;
      y[i] = x[i] * x[i];
    }
    std::vector<MatrixWorkspace_sptr> wsVec(4);
    for (size_t i = 0; i < 4; ++i) {
      wsVec[i] = generateWorkspace("ws" + std::to_string(i), x, y, xUnit);
    }
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