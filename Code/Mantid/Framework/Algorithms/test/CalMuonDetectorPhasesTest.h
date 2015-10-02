#ifndef CALMUONDETECTORPHASESTEST_H_
#define CALMUONDETECTORPHASESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include <boost/assign/list_of.hpp>

using namespace Mantid::API;
using Mantid::MantidVec;

const std::string outputName = "MuonRemoveExpDecay_Output";

class CalMuonDetectorPhasesTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalMuonDetectorPhasesTest *createSuite() { return new CalMuonDetectorPhasesTest(); }
  static void destroySuite( CalMuonDetectorPhasesTest *suite ) { delete suite; }

  CalMuonDetectorPhasesTest() {
    FrameworkManager::Instance();
  }


  void testInit()
  {
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("CalMuonDetectorPhases");
    alg->initialize();
    TS_ASSERT(alg->isInitialized())
  }

  void testExecute()
  {
    auto ws = createWorkspace("Microseconds");
    auto calc = AlgorithmManager::Instance().create("CalMuonDetectorPhases");
    calc->initialize();
    calc->setChild(true);
    calc->setProperty("InputWorkspace", ws);
    calc->setProperty("Frequency", 25.);
    calc->setPropertyValue("DataFitted", "fit");
    calc->setPropertyValue("DetectorTable", "tab");

    TS_ASSERT_THROWS_NOTHING(calc->execute());

    WorkspaceGroup_sptr fitResults = calc->getProperty("DataFitted");
    ITableWorkspace_sptr detTab = calc->getProperty("DetectorTable");
  }

  MatrixWorkspace_sptr createWorkspace(std::string units) {

    // Create a fake muon dataset
    double a = 0.1; // Amplitude of the oscillations
    double w = 25.; // Frequency of the oscillations
    double tau = 2.2; // Muon life time
    size_t maxt = 100; // Maximum time

    MantidVec X(maxt);
    MantidVec Y(maxt);
    for (size_t s = 0; s < 4; s++) {
      for (size_t t = 0; t < maxt; t++) {
        double x = static_cast<double>(t) / maxt;
        double e = exp(-x / tau);
        X.push_back(x);
        Y.push_back(a * sin(w * t + s * M_PI / 4.) * e + e);
      }
    }
    auto createWS = AlgorithmManager::Instance().create("CreateWorkspace");
    createWS->initialize();
    createWS->setChild(true);
    createWS->setProperty("UnitX", units);
    createWS->setProperty("DataX", X);
    createWS->setProperty("DataY", Y);
    createWS->setProperty("NSpec", 4);
    createWS->setPropertyValue("OutputWorkspace", "ws");
    createWS->execute();
    MatrixWorkspace_sptr ws = createWS->getProperty("OutputWorkspace");

    return ws;
  }
};

#endif /*CALMUONDETECTORPHASESTEST_H_*/