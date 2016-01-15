#ifndef CALMUONDETECTORPHASESTEST_H_
#define CALMUONDETECTORPHASESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include <boost/assign/list_of.hpp>

using namespace Mantid::API;
using Mantid::MantidVec;

const std::string outputName = "MuonRemoveExpDecay_Output";

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
    auto calc = AlgorithmManager::Instance().create("CalMuonDetectorPhases");
    calc->initialize();
    calc->setChild(true);
    calc->setProperty("InputWorkspace", ws);
    calc->setPropertyValue("Frequency", "25");
    calc->setPropertyValue("DataFitted", "fit");
    calc->setPropertyValue("DetectorTable", "tab");

    TS_ASSERT_THROWS_NOTHING(calc->execute());

    WorkspaceGroup_sptr fitResults = calc->getProperty("DataFitted");
    ITableWorkspace_sptr tab = calc->getProperty("DetectorTable");

    // Check the table workspace
    TS_ASSERT_EQUALS(tab->rowCount(), 4);
    TS_ASSERT_EQUALS(tab->columnCount(), 3);
    // Test asymmetries
    TS_ASSERT_DELTA(tab->Double(0, 1), 0.099, 0.001);
    TS_ASSERT_DELTA(tab->Double(1, 1), 0.099, 0.001);
    TS_ASSERT_DELTA(tab->Double(2, 1), 0.099, 0.001);
    TS_ASSERT_DELTA(tab->Double(3, 1), 0.100, 0.001);
    // Test phases
    TS_ASSERT_DELTA(tab->Double(0, 2), 6.281, 0.001);
    TS_ASSERT_DELTA(tab->Double(1, 2), 0.785, 0.001);
    TS_ASSERT_DELTA(tab->Double(2, 2), 1.570, 0.001);
    TS_ASSERT_DELTA(tab->Double(3, 2), 2.354, 0.001);
  }

  void testBadWorkspaceUnits() {

    auto ws = createWorkspace(1, 4, "Wavelength");
    auto calc = AlgorithmManager::Instance().create("CalMuonDetectorPhases");
    calc->initialize();
    calc->setChild(true);
    calc->setProperty("InputWorkspace", ws);
    calc->setPropertyValue("Frequency", "25");
    calc->setPropertyValue("DataFitted", "fit");
    calc->setPropertyValue("DetectorTable", "tab");

    TS_ASSERT_THROWS(calc->execute(), std::runtime_error);
    TS_ASSERT(!calc->isExecuted());
  }

  void testNoFrequencySupplied() {

    auto ws = createWorkspace(1, 4, "Microseconds");
    auto calc = AlgorithmManager::Instance().create("CalMuonDetectorPhases");
    calc->initialize();
    calc->setChild(true);
    calc->setProperty("InputWorkspace", ws);
    calc->setPropertyValue("DataFitted", "fit");
    calc->setPropertyValue("DetectorTable", "tab");

    TS_ASSERT_THROWS(calc->execute(), std::runtime_error);
    TS_ASSERT(!calc->isExecuted());
  }

  MatrixWorkspace_sptr createWorkspace(size_t nspec, size_t maxt,
                                       std::string units) {

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
        Y.push_back(a * sin(w * x +
                            static_cast<double>(s) * M_PI /
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
};

#endif /*CALMUONDETECTORPHASESTEST_H_*/