#ifndef MANTID_ALGORITHMS_FINDEPPTEST_H_
#define MANTID_ALGORITHMS_FINDEPPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FindEPP.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;

namespace {
enum WorkspaceType : size_t {
  NegativeMaximum = 0,
  NarrowPeak = 1,
  FitFailed = 2,
  Success = 3,
  Performance = 4
};

MatrixWorkspace_sptr _create_test_workspace(WorkspaceType type) {

  CreateSampleWorkspace createAlg;

  if (type != NegativeMaximum) {
    createAlg.initialize();
    createAlg.setProperty("BankPixelWidth", 1);
    createAlg.setPropertyValue("OutputWorkspace", "__ws");
    createAlg.setLogging(false);
    createAlg.setChild(true);
  }

  switch (type) {

  case NegativeMaximum: {
    size_t nBins = 5;
    MatrixWorkspace_sptr result =
        WorkspaceFactory::Instance().create("Workspace2D", 1, nBins, nBins);
    for (size_t bin = 0; bin < nBins; ++bin) {
      result->mutableY(0)[bin] = -1.;
      result->mutableX(0)[bin] = double(bin);
    }
    return result;
  }

  case NarrowPeak: {
    createAlg.setPropertyValue("Function", "User Defined");
    createAlg.setPropertyValue(
        "UserDefinedFunction",
        "name=Gaussian, PeakCentre=5, Height=1, Sigma=0.05");
    createAlg.setProperty("XMin", 0.);
    createAlg.setProperty("XMax", 10.);
    createAlg.setProperty("BinWidth", 0.1);
    createAlg.setProperty("NumBanks", 1);
    break;
  }

  case FitFailed: {
    createAlg.setPropertyValue("Function", "Exp Decay");
    createAlg.setProperty("XMin", 0.);
    createAlg.setProperty("XMax", 100.);
    createAlg.setProperty("BinWidth", 1.);
    createAlg.setProperty("NumBanks", 1);
    break;
  }

  case Success: {
    createAlg.setPropertyValue("Function", "User Defined");
    createAlg.setPropertyValue("UserDefinedFunction",
                               "name=LinearBackground,A0=0.3;"
                               "name=Gaussian,"
                               "PeakCentre=6000, Height=5, Sigma=75");
    createAlg.setProperty("XMin", 4005.75);
    createAlg.setProperty("XMax", 7995.75);
    createAlg.setProperty("BinWidth", 10.5);
    createAlg.setProperty("NumBanks", 2);
    break;
  }

  case Performance: {
    createAlg.setPropertyValue("Function", "User Defined");
    createAlg.setPropertyValue("UserDefinedFunction",
                               "name=LinearBackground,A0=0.3,A1=0.001;"
                               "name=Gaussian,"
                               "PeakCentre=6000, Height=5, Sigma=75");
    createAlg.setProperty("XMin", 4005.75);
    createAlg.setProperty("XMax", 7995.75);
    createAlg.setProperty("BinWidth", 5.01);
    createAlg.setProperty("NumBanks", 100);
    createAlg.setProperty("BankPixelWidth", 10);
    createAlg.setProperty("Random", true);
    break;
  }
  }

  createAlg.execute();
  return createAlg.getProperty("OutputWorkspace");
}
}

class FindEPPTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FindEPPTest *createSuite() { return new FindEPPTest(); }
  static void destroySuite(FindEPPTest *suite) { delete suite; }

  FindEPPTest()
      : m_columnNames({"WorkspaceIndex", "PeakCentre", "PeakCentreError",
                       "Sigma", "SigmaError", "Height", "HeightError", "chiSq",
                       "FitStatus"}),
        m_delta(1E-4) {
    FrameworkManager::Instance();
  }

  void test_init() {
    FindEPP alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_success() {
    MatrixWorkspace_sptr inputWS = _create_test_workspace(Success);

    FindEPP alg;
    alg.setChild(true);
    alg.setLogging(false);

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "__unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    _check_table(outputWS, 2);

    for (size_t row = 0; row < 2; ++row) {
      TS_ASSERT_EQUALS(outputWS->cell<std::string>(row, 8), "success");
      TS_ASSERT_DELTA(outputWS->cell<double>(row, 1), 6005.25, m_delta);
      TS_ASSERT_DELTA(outputWS->cell<double>(row, 2), 8.817, m_delta);
      TS_ASSERT_DELTA(outputWS->cell<double>(row, 3), 89.3248, m_delta);
      TS_ASSERT_DELTA(outputWS->cell<double>(row, 4), 7.2306, m_delta);
      TS_ASSERT_DELTA(outputWS->cell<double>(row, 5), 4.8384, m_delta);
      TS_ASSERT_DELTA(outputWS->cell<double>(row, 6), 0.6161, m_delta);
      TS_ASSERT_DELTA(outputWS->cell<double>(row, 7), 0.1643, m_delta);
    }
  }

  void test_negativeMaximum() {
    MatrixWorkspace_sptr inputWS = _create_test_workspace(NegativeMaximum);

    FindEPP alg;
    alg.setChild(true);
    alg.setLogging(false);

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "__unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    _check_table(outputWS, 1);

    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 8), "negativeMaximum");
    TS_ASSERT_DELTA(outputWS->cell<double>(0, 1), 0., m_delta);
  }

  void test_narrowPeak() {
    MatrixWorkspace_sptr inputWS = _create_test_workspace(NarrowPeak);

    FindEPP alg;
    alg.setChild(true);
    alg.setLogging(false);

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "__unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    _check_table(outputWS, 1);

    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 8), "narrowPeak");
    TS_ASSERT_DELTA(outputWS->cell<double>(0, 1), 5., m_delta);
  }

  void test_fitFailed() {
    MatrixWorkspace_sptr inputWS = _create_test_workspace(FitFailed);

    FindEPP alg;
    alg.setChild(true);
    alg.setLogging(false);

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "__unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    _check_table(outputWS, 1);

    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 8), "fitFailed");
    TS_ASSERT_DELTA(outputWS->cell<double>(0, 1), 0., m_delta);
  }

private:
  void _check_table(ITableWorkspace_sptr ws, size_t nSpectra) {
    TS_ASSERT_EQUALS(ws->rowCount(), nSpectra);
    TS_ASSERT_EQUALS(ws->columnCount(), 9);
    TS_ASSERT_EQUALS(ws->getColumnNames(), m_columnNames);
  }
  std::vector<std::string> m_columnNames;
  double m_delta;
};

class FindEPPTestPerformance : public CxxTest::TestSuite {
public:
  static FindEPPTestPerformance *createSuite() {
    return new FindEPPTestPerformance();
  }
  static void destroySuite(FindEPPTestPerformance *suite) { delete suite; }

  FindEPPTestPerformance() {}

  void setUp() override {
    FrameworkManager::Instance();
    MatrixWorkspace_sptr in = _create_test_workspace(Performance);
    m_alg.initialize();
    m_alg.setProperty("InputWorkspace", in);
    m_alg.setProperty("OutputWorkspace", "__out_ws");
  }

  void tearDown() override {
    AnalysisDataService::Instance().remove("__out_ws");
  }

  void test_performance() { m_alg.execute(); }

private:
  FindEPP m_alg;
};

#endif /* MANTID_ALGORITHMS_FINDEPPTEST_H_ */
