// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CALMUONDETECTORPHASESTEST_H_
#define CALMUONDETECTORPHASESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidMuon/CalMuonDetectorPhases.h"

using namespace Mantid::API;
using Mantid::MantidVec;

const std::string outputName = "MuonRemoveExpDecay_Output";

/**
 * This is a test class that exists to test the method validateInputs()
 */
class TestCalMuonDetectorPhases
    : public Mantid::Algorithms::CalMuonDetectorPhases {
public:
  std::map<std::string, std::string> wrapValidateInputs() {
    return this->validateInputs();
  }
};

class CalMuonDetectorPhasesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalMuonDetectorPhasesTest *createSuite() {
    return new CalMuonDetectorPhasesTest();
  }
  static void destroySuite(CalMuonDetectorPhasesTest *suite) { delete suite; }

  CalMuonDetectorPhasesTest() { FrameworkManager::Instance(); }

  void testInit() {
    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create("CalMuonDetectorPhases");
    alg->initialize();
    TS_ASSERT(alg->isInitialized())
  }

  void testExecute() {
    auto ws = createWorkspace(4, 100, "Microseconds");
    runExecutionTest(ws);
  }

  void testBadWorkspaceUnits() {

    auto ws = createWorkspace(2, 4, "Wavelength");
    auto calc = AlgorithmManager::Instance().create("CalMuonDetectorPhases");
    calc->initialize();
    calc->setChild(true);
    calc->setProperty("InputWorkspace", ws);
    calc->setPropertyValue("Frequency", "4");
    calc->setPropertyValue("DataFitted", "fit");
    calc->setPropertyValue("DetectorTable", "tab");
    calc->setProperty("ForwardSpectra", std::vector<int>{1});
    calc->setProperty("BackwardSpectra", std::vector<int>{2});

    TS_ASSERT_THROWS(calc->execute(), const std::runtime_error &);
    TS_ASSERT(!calc->isExecuted());
  }

  void testNoFrequencySupplied() {

    auto ws = createWorkspace(2, 4, "Microseconds");
    auto calc = AlgorithmManager::Instance().create("CalMuonDetectorPhases");
    calc->initialize();
    calc->setChild(true);
    calc->setProperty("InputWorkspace", ws);
    calc->setPropertyValue("DataFitted", "fit");
    calc->setPropertyValue("DetectorTable", "tab");
    calc->setProperty("ForwardSpectra", std::vector<int>{1});
    calc->setProperty("BackwardSpectra", std::vector<int>{2});

    TS_ASSERT_THROWS(calc->execute(), const std::runtime_error &);
    TS_ASSERT(!calc->isExecuted());
  }

  /**
   * Test that the algorithm can handle a WorkspaceGroup as input without
   * crashing
   * We have to use the ADS to test WorkspaceGroups
   */
  void testValidateInputsWithWSGroup() {
    auto ws1 = boost::static_pointer_cast<Workspace>(
        createWorkspace(2, 4, "Microseconds"));
    auto ws2 = boost::static_pointer_cast<Workspace>(
        createWorkspace(2, 4, "Microseconds"));
    AnalysisDataService::Instance().add("workspace1", ws1);
    AnalysisDataService::Instance().add("workspace2", ws2);
    auto group = boost::make_shared<WorkspaceGroup>();
    AnalysisDataService::Instance().add("group", group);
    group->add("workspace1");
    group->add("workspace2");
    TestCalMuonDetectorPhases calc;
    calc.initialize();
    calc.setChild(true);
    TS_ASSERT_THROWS_NOTHING(calc.setPropertyValue("InputWorkspace", "group"));
    calc.setPropertyValue("DataFitted", "fit");
    calc.setPropertyValue("DetectorTable", "tab");
    calc.setProperty("ForwardSpectra", std::vector<int>{1});
    calc.setProperty("BackwardSpectra", std::vector<int>{2});
    TS_ASSERT_THROWS_NOTHING(calc.wrapValidateInputs());
    AnalysisDataService::Instance().clear();
  }

  void testWithMUSRWorkspaceLongitudinal() {
    auto ws = createWorkspace(4, 100, "Microseconds", "MUSR", "Longitudinal");
    runExecutionTest(ws);
  }

  void testWithMUSRWorkspaceTransverse() {
    auto ws = createWorkspace(4, 100, "Microseconds", "MUSR", "Transverse");
    runExecutionTest(ws);
  }

private:
  MatrixWorkspace_sptr createWorkspace(size_t nspec, size_t maxt,
                                       const std::string &units) {

    // Create a fake muon dataset
    double a = 0.1; // Amplitude of the oscillations
    double w = 25.; // Frequency of the oscillations
    double tau = Mantid::PhysicalConstants::MuonLifetime *
                 1e6; // Muon life time in microseconds

    MantidVec X;
    MantidVec Y;
    MantidVec E;
    for (size_t s = 0; s < nspec; s++) {
      for (size_t t = 0; t < maxt; t++) {
        double x = static_cast<double>(t) / static_cast<double>(maxt);
        double e = exp(-x / tau);
        X.push_back(x);
        Y.push_back(a *
                        sin(w * x + static_cast<double>(s) * M_PI /
                                        static_cast<double>(nspec)) *
                        e +
                    e);
        E.push_back(0.005);
      }
    }

    auto createWS = AlgorithmManager::Instance().create("CreateWorkspace");
    createWS->initialize();
    createWS->setChild(true);
    createWS->setProperty("UnitX", units);
    createWS->setProperty("DataX", X);
    createWS->setProperty("DataY", Y);
    createWS->setProperty("DataE", E);
    createWS->setProperty("NSpec", static_cast<int>(nspec));
    createWS->setPropertyValue("OutputWorkspace", "ws");
    createWS->execute();
    MatrixWorkspace_sptr ws = createWS->getProperty("OutputWorkspace");

    return ws;
  }

  /// overload that adds instrument and main field direction log to workspace
  MatrixWorkspace_sptr createWorkspace(size_t nspec, size_t maxt,
                                       const std::string &units,
                                       const std::string &instrumentName,
                                       const std::string &mainFieldDirection) {
    auto ws = createWorkspace(nspec, maxt, units);
    auto instrument =
        boost::make_shared<Mantid::Geometry::Instrument>(instrumentName);
    ws->setInstrument(instrument);
    ws->mutableRun().addProperty("main_field_direction", mainFieldDirection);
    return ws;
  }

  /// Runs test of execution on the given workspace
  void runExecutionTest(const MatrixWorkspace_sptr workspace) {
    auto calc = AlgorithmManager::Instance().create("CalMuonDetectorPhases");
    calc->initialize();
    calc->setChild(true);
    calc->setProperty("InputWorkspace", workspace);
    calc->setPropertyValue("Frequency", "4");
    calc->setPropertyValue("DataFitted", "fit");
    calc->setPropertyValue("DetectorTable", "tab");
    calc->setProperty("ForwardSpectra", std::vector<int>{1, 2});
    calc->setProperty("BackwardSpectra", std::vector<int>{3, 4});

    TS_ASSERT_THROWS_NOTHING(calc->execute());

    ITableWorkspace_sptr tab = calc->getProperty("DetectorTable");

    // Check the table workspace
    TS_ASSERT_EQUALS(tab->rowCount(), 4);
    TS_ASSERT_EQUALS(tab->columnCount(), 3);
    // Test detector IDs
    TS_ASSERT_EQUALS(tab->Int(0, 0), 1);
    TS_ASSERT_EQUALS(tab->Int(1, 0), 2);
    TS_ASSERT_EQUALS(tab->Int(2, 0), 3);
    TS_ASSERT_EQUALS(tab->Int(3, 0), 4);
    // Test asymmetries
    TS_ASSERT_DELTA(tab->Double(0, 1), 0.099, 0.001);
    TS_ASSERT_DELTA(tab->Double(1, 1), 0.100, 0.001);
    TS_ASSERT_DELTA(tab->Double(2, 1), 0.100, 0.001);
    TS_ASSERT_DELTA(tab->Double(3, 1), 0.100, 0.001);
    // Test phases
    TS_ASSERT_DELTA(tab->Double(0, 2), 1.576, 0.001);
    TS_ASSERT_DELTA(tab->Double(1, 2), 0.789, 0.001);
    TS_ASSERT_DELTA(tab->Double(2, 2), 0.005, 0.001);
    TS_ASSERT_DELTA(tab->Double(3, 2), 5.504, 0.001);
  }
};

#endif /*CALMUONDETECTORPHASESTEST_H_*/
