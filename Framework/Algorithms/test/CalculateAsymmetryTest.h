#ifndef MUONCALCULATEASYMMETRYTEST_H_
#define MUONCALCULATEASYMMETRYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/CalculateAsymmetry.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using Mantid::MantidVec;
using Mantid::Algorithms::CalculateAsymmetry;

const std::string outputName = "CalculateAsymmetry_Output";

namespace {
MatrixWorkspace_sptr createWorkspace(size_t nspec, size_t maxt) {

  // Create a fake muon dataset
  double a = 10.0; // Amplitude of the oscillations
  double w = 5.0; // Frequency of the oscillations
  double tau = Mantid::PhysicalConstants::MuonLifetime *
               1e6; // Muon life time in microseconds
  double phi= 0.1;
  MantidVec X;
  MantidVec Y;
  MantidVec E;
  for (size_t s = 0; s < nspec; s++) {
    for (size_t t = 0; t < maxt; t++) {
      double x = 10.*static_cast<double>(t) / static_cast<double>(maxt);
      double e = exp(-x / tau);
      X.push_back(x);
      Y.push_back(
          20.*(1.0+a*cos(w * x +phi))*e);
      E.push_back(0.005);
    }
  }

  auto createWS = AlgorithmManager::Instance().create("CreateWorkspace");
  createWS->initialize();
  createWS->setChild(true);
  createWS->setProperty("DataX", X);
  createWS->setProperty("DataY", Y);
  createWS->setProperty("DataE", E);
  createWS->setProperty("NSpec", static_cast<int>(nspec));
  createWS->setPropertyValue("OutputWorkspace", "ws");
  createWS->execute();
  MatrixWorkspace_sptr ws = createWS->getProperty("OutputWorkspace");
  // Add  number of good frames
  ws->mutableRun().addProperty("goodfrm", 10);
  return ws;
}
}

class CalculateAsymmetryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateAsymmetryTest *createSuite() { return new CalculateAsymmetryTest(); }
  static void destroySuite(CalculateAsymmetryTest *suite) { delete suite; }

  CalculateAsymmetryTest() { FrameworkManager::Instance(); }

  void testInit() {
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("CalculateAsymmetry");
    alg->initialize();
    TS_ASSERT(alg->isInitialized())
  }

  void test_Execute() {

    auto ws = createWorkspace(1, 50);
    	
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("CalculateAsymmetry");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    alg->setProperty("XStart", 0.1);
    alg->setProperty("XEnd", 10.);
    alg->setProperty("myFunction", "name=UserFunction,Formula=A*cos(omega*x+phi),A=10,omega=3.0,phi=0.0");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
  }
  void test_EmptySpectrumList() {

    auto ws = createWorkspace(2, 50);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("CalculateAsymmetry");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    alg->setProperty("XStart", 0.1);
    alg->setProperty("XEnd",10.);
    alg->setProperty("myFunction", "name=UserFunction,Formula=A*cos(omega*x+phi),A=10,omega=3.0,phi=0.0");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");

    // First spectrum
    // Test some X values
    TS_ASSERT_DELTA(outWS->x(0)[10], 2.000, 0.0001);
    TS_ASSERT_DELTA(outWS->x(0)[19], 3.800, 0.0001);
    TS_ASSERT_DELTA(outWS->x(0)[49], 9.800, 0.0001);
    // Test some Y values
    TS_ASSERT_DELTA(outWS->y(0)[10], -7.8056, 0.0001);
    TS_ASSERT_DELTA(outWS->y(0)[19], 9.6880, 0.0001);
    TS_ASSERT_DELTA(outWS->y(0)[49], 3.9431, 0.0001);
    // Test some E values
    TS_ASSERT_DELTA(outWS->e(0)[10], 0.0006, 0.0001);
    TS_ASSERT_DELTA(outWS->e(0)[19], 0.0014, 0.0001);
    TS_ASSERT_DELTA(outWS->e(0)[49], 0.0216, 0.0001);

    // Second spectrum
    // Test some X values
    TS_ASSERT_DELTA(outWS->x(1)[10], 2.000, 0.0001);
    TS_ASSERT_DELTA(outWS->x(1)[19], 3.800, 0.0001);
    TS_ASSERT_DELTA(outWS->x(1)[49], 9.800, 0.0001);
    // Test some Y values
    TS_ASSERT_DELTA(outWS->y(1)[10], -7.8056, 0.0001);
    TS_ASSERT_DELTA(outWS->y(1)[19], 9.6880, 0.0001);
    TS_ASSERT_DELTA(outWS->y(1)[49], 3.9431, 0.0001);
    // Test some E values
    TS_ASSERT_DELTA(outWS->e(1)[10], 0.0006, 0.0001);
    TS_ASSERT_DELTA(outWS->e(1)[19], 0.0014, 0.0001);
    TS_ASSERT_DELTA(outWS->e(1)[49], 0.0216, 0.0001);
  }
  void test_SpectrumList() {

    auto ws = createWorkspace(2, 50);

    // First, run the algorithm without specifying any spectrum
 
    IAlgorithm_sptr alg1 = AlgorithmManager::Instance().create("CalculateAsymmetry");
    alg1->initialize();
    alg1->setChild(true);
    alg1->setProperty("InputWorkspace", ws);
    alg1->setPropertyValue("OutputWorkspace", outputName);
    alg1->setProperty("XStart", 0.1);
    alg1->setProperty("XEnd", 0.9);
    alg1->setProperty("myFunction", "name=UserFunction,Formula=A*cos(omega*x+phi),A=10,omega=3.0,phi=0.0");
    TS_ASSERT_THROWS_NOTHING(alg1->execute());
    TS_ASSERT(alg1->isExecuted());
 
    MatrixWorkspace_sptr out1 = alg1->getProperty("OutputWorkspace");

    // Then run the algorithm on the second spectrum only
     IAlgorithm_sptr alg2 = AlgorithmManager::Instance().create("CalculateAsymmetry");
    alg2->initialize();
    alg2->setChild(true);
    alg2->setProperty("InputWorkspace", ws);
    alg2->setPropertyValue("OutputWorkspace", outputName);
    alg2->setPropertyValue("Spectra", "1");
    alg2->setProperty("XStart", 0.1);
    alg2->setProperty("XEnd", 0.9);
    alg2->setProperty("myFunction", "name=UserFunction,Formula=A*cos(omega*x+phi),A=10,omega=3.0,phi=0.0");
    TS_ASSERT_THROWS_NOTHING(alg2->execute());
    TS_ASSERT(alg2->isExecuted());
    MatrixWorkspace_sptr out2 = alg2->getProperty("OutputWorkspace");

    // Both output workspaces should have 2 spectra
    TS_ASSERT_EQUALS(out1->getNumberHistograms(), ws->getNumberHistograms());
    TS_ASSERT_EQUALS(out2->getNumberHistograms(), ws->getNumberHistograms());

    // Compare results, they should match for the selected spectrum
    TS_ASSERT_EQUALS(out1->x(1).rawData(), out2->x(1).rawData());
    TS_ASSERT_EQUALS(out1->y(1).rawData(), out2->y(1).rawData());
    TS_ASSERT_EQUALS(out1->e(1).rawData(), out2->e(1).rawData());

    // Compare non-selected spectra, the should match the input ones
    TS_ASSERT_EQUALS(ws->x(0).rawData(), out2->x(0).rawData());
    TS_ASSERT_EQUALS(ws->y(0).rawData(), out2->y(0).rawData());
    TS_ASSERT_EQUALS(ws->e(0).rawData(), out2->e(0).rawData());
  }
  void test_yUnitLabel() {

    auto ws = createWorkspace(4, 50);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("CalculateAsymmetry");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    alg->setProperty("XStart", 0.1);
    alg->setProperty("XEnd", 0.9);
    alg->setProperty("OutputWorkspace", outputName);
    alg->setProperty("myFunction", "name=UserFunction,Formula=A*cos(omega*x+phi),A=10,omega=3.0,phi=0.0");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted())

    MatrixWorkspace_sptr result = alg->getProperty("OutputWorkspace");
    TS_ASSERT(result);
    TS_ASSERT_EQUALS(result->YUnitLabel(), "Asymmetry");
  }
   void test_noLowerBound() {
    auto ws = createWorkspace(4, 50);
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("CalculateAsymmetry");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    alg->setProperty("XEnd", 0.9);
    alg->setProperty("OutputWorkspace", outputName);
    alg->setProperty("myFunction", "name=UserFunction,Formula=A*cos(omega*x+phi),A=10,omega=3.0,phi=0.0");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted())
  }
 
  void test_noRange() {
    auto ws = createWorkspace(4, 50);
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("CalculateAsymmetry");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    alg->setProperty("OutputWorkspace", outputName);
    alg->setProperty("myFunction", "name=UserFunction,Formula=A*cos(omega*x+phi),A=10,omega=3.0,phi=0.0");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted())
  }
   void test_backwardsRange() {
    auto ws = createWorkspace(4, 50);
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("CalculateAsymmetry");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    alg->setProperty("Xstart", 0.9);
    alg->setProperty("XEnd", 0.1);
    alg->setProperty("OutputWorkspace", outputName);
    alg->setProperty("myFunction", "name=UserFunction,Formula=A*cos(omega*x+phi),A=10,omega=3.0,phi=0.0");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted())
  }
  void test_NoMyFunction() {

    auto ws = createWorkspace(1, 50);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("CalculateAsymmetry");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    alg->setProperty("XStart", 0.1);
    alg->setProperty("XEnd",10.);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");

    // First spectrum
    // Test some X values
    TS_ASSERT_DELTA(outWS->x(0)[10], 2.000, 0.0001);
    TS_ASSERT_DELTA(outWS->x(0)[19], 3.800, 0.0001);
    TS_ASSERT_DELTA(outWS->x(0)[49], 9.800, 0.0001);
    // Test some Y values
    TS_ASSERT_DELTA(outWS->y(0)[10], -7.8056, 0.0001);
    TS_ASSERT_DELTA(outWS->y(0)[19], 9.6880, 0.0001);
    TS_ASSERT_DELTA(outWS->y(0)[49], 3.9431, 0.0001);
    // Test some E values
    TS_ASSERT_DELTA(outWS->e(0)[10], 0.0006, 0.0001);
    TS_ASSERT_DELTA(outWS->e(0)[19], 0.0014, 0.0001);
    TS_ASSERT_DELTA(outWS->e(0)[49], 0.0216, 0.0001);
  }
 
  };

class CalculateAsymmetryTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateAsymmetryTestPerformance *createSuite() {
    return new CalculateAsymmetryTestPerformance();
  }
  static void destroySuite(CalculateAsymmetryTestPerformance *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  CalculateAsymmetryTestPerformance() { FrameworkManager::Instance(); }

  void setUp() override { 
	input = createWorkspace(1000, 100); 
	
}

  void testExec2D() {
    CalculateAsymmetry alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", input);
    alg.setPropertyValue("OutputWorkspace", "output");
    alg.setProperty("XStart", 0.1);
    alg.setProperty("XEnd", 10.);
    alg.setProperty("myFunction", "name=UserFunction,Formula=A*cos(omega*x+phi),A=10,omega=3.0,phi=0.0");
 
    alg.execute();
  }

private:
  MatrixWorkspace_sptr input;

};
#endif /*ESTIMATEASYMMETRYFROMCOUNTSTEST_H_*/
