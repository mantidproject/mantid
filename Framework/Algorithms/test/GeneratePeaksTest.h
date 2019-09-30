// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_GENERATEPEAKSTEST_H_
#define MANTID_ALGORITHMS_GENERATEPEAKSTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/GeneratePeaks.h"
#include "MantidDataObjects/TableWorkspace.h"

#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidHistogramData/LinearGenerator.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::HistogramData;
namespace {
//----------------------------------------------------------------------------------------------
/** Generate a TableWorkspace containing 3 peaks on 2 spectra by using
 * effective parameters
 *  spectra 0:  center = 2.0, width = 0.2, height = 5,  a0 = 1.0, a1 = 2.0, a2
 * = 0
 *  spectra 0:  center = 8.0, width = 0.1, height = 10, a0 = 2.0, a1 = 1.0, a2
 * = 0
 *  spectra 2:  center = 4.0, width = 0.4, height = 20, a0 = 4.0, a1 = 0.0, a2
 * = 0
 */
DataObjects::TableWorkspace_sptr createTestEffectiveFuncParameters() {
  // 1. Build a TableWorkspace
  DataObjects::TableWorkspace_sptr peakparms =
      boost::shared_ptr<DataObjects::TableWorkspace>(
          new DataObjects::TableWorkspace);
  peakparms->addColumn("int", "spectrum");
  peakparms->addColumn("double", "centre");
  peakparms->addColumn("double", "width");
  peakparms->addColumn("double", "height");
  peakparms->addColumn("double", "backgroundintercept");
  peakparms->addColumn("double", "backgroundslope");
  peakparms->addColumn("double", "A2");
  peakparms->addColumn("double", "chi2");

  // 2. Add value
  API::TableRow row0 = peakparms->appendRow();
  row0 << 0 << 2.0 << 0.2 << 5.0 << 1.0 << 2.0 << 0.0 << 0.1;
  API::TableRow row1 = peakparms->appendRow();
  row1 << 0 << 8.0 << 0.1 << 10.0 << 2.0 << 1.0 << 0.0 << 0.2;
  API::TableRow row2 = peakparms->appendRow();
  row2 << 2 << 4.0 << 0.4 << 20.0 << 4.0 << 0.0 << 0.0 << 0.2;
  API::TableRow row3 = peakparms->appendRow();
  row3 << 2 << 4.5 << 0.4 << 20.0 << 1.0 << 9.0 << 0.0 << 1000.2;

  return peakparms;
}

//----------------------------------------------------------------------------------------------
/** Generate a TableWorkspace containing 3 peaks on 2 spectra by using raw
 * parameters
 *  spectra 0:  center = 2.0, width = 0.2, height = 5,  a0 = 1.0, a1 = 2.0, a2
 * = 0
 *  spectra 0:  center = 8.0, width = 0.1, height = 10, a0 = 2.0, a1 = 1.0, a2
 * = 0
 *  spectra 2:  center = 4.0, width = 0.4, height = 20, a0 = 4.0, a1 = 0.0, a2
 * = 0
 */
DataObjects::TableWorkspace_sptr createTestPeakParameters2() {
  // 1. Build a TableWorkspace
  DataObjects::TableWorkspace_sptr peakparms =
      boost::shared_ptr<DataObjects::TableWorkspace>(
          new DataObjects::TableWorkspace);
  peakparms->addColumn("int", "spectrum");
  peakparms->addColumn("double", "PeakCentre");
  peakparms->addColumn("double", "Sigma");
  peakparms->addColumn("double", "Height");
  peakparms->addColumn("double", "A0");
  peakparms->addColumn("double", "A1");
  peakparms->addColumn("double", "A2");
  peakparms->addColumn("double", "chi2");

  // 2. Add value
  API::TableRow row0 = peakparms->appendRow();
  row0 << 0 << 2.0 << 0.0849322 << 5.0 << 1.0 << 2.0 << 0.0 << 0.1;
  API::TableRow row1 = peakparms->appendRow();
  row1 << 0 << 8.0 << 0.0424661 << 10.0 << 2.0 << 1.0 << 0.0 << 0.2;
  API::TableRow row2 = peakparms->appendRow();
  row2 << 2 << 4.0 << 0.169864 << 20.0 << 4.0 << 0.0 << 0.0 << 0.2;
  API::TableRow row3 = peakparms->appendRow();
  row3 << 2 << 4.5 << 0.4 << 20.0 << 1.0 << 9.0 << 0.0 << 1000.2;

  return peakparms;
}

//----------------------------------------------------------------------------------------------
/** Generate a TableWorkspace containing 3 peaks on 2 spectra by using
 * effective parameters
 * of old style f0., f1.
 *  spectra 0:  center = 2.0, width = 0.2, height = 5,  a0 = 1.0, a1 = 2.0, a2
 * = 0
 *  spectra 0:  center = 8.0, width = 0.1, height = 10, a0 = 2.0, a1 = 1.0, a2
 * = 0
 *  spectra 2:  center = 4.0, width = 0.4, height = 20, a0 = 4.0, a1 = 0.0, a2
 * = 0
 */
DataObjects::TableWorkspace_sptr createTestPeakParameters3() {
  // 1. Build a TableWorkspace
  DataObjects::TableWorkspace_sptr peakparms =
      boost::shared_ptr<DataObjects::TableWorkspace>(
          new DataObjects::TableWorkspace);
  peakparms->addColumn("int", "spectrum");
  peakparms->addColumn("double", "f0.centre");
  peakparms->addColumn("double", "f0.width");
  peakparms->addColumn("double", "f0.height");
  peakparms->addColumn("double", "f1.backgroundintercept");
  peakparms->addColumn("double", "f1.backgroundslope");
  peakparms->addColumn("double", "f1.A2");
  peakparms->addColumn("double", "chi2");

  // 2. Add value
  API::TableRow row0 = peakparms->appendRow();
  row0 << 0 << 2.0 << 0.2 << 5.0 << 1.0 << 2.0 << 0.0 << 0.1;
  API::TableRow row1 = peakparms->appendRow();
  row1 << 0 << 8.0 << 0.1 << 10.0 << 2.0 << 1.0 << 0.0 << 0.2;
  API::TableRow row2 = peakparms->appendRow();
  row2 << 2 << 4.0 << 0.4 << 20.0 << 4.0 << 0.0 << 0.0 << 0.2;
  API::TableRow row3 = peakparms->appendRow();
  row3 << 2 << 4.5 << 0.4 << 20.0 << 1.0 << 9.0 << 0.0 << 1000.2;

  return peakparms;
}

//----------------------------------------------------------------------------------------------
/** Create a MatrixWorkspace containing 5 spectra
 *  Binning parameter = 1.0, 0.02, 9.0
 */
API::MatrixWorkspace_sptr createTestInputWorkspace() {
  // 1. Create empty workspace
  double minx = 1.0;
  double maxx = 9.0;
  double dx = 0.02;
  size_t size = static_cast<size_t>((maxx - minx) / dx) + 1;
  API::MatrixWorkspace_sptr inpWS = API::WorkspaceFactory::Instance().create(
      "Workspace2D", 5, size, size - 1);

  // 1.5 Generate shared Copy-On-Write x values
  BinEdges x(size, LinearGenerator(minx, dx));
  // 2. Put x values and y values
  for (size_t iw = 0; iw < inpWS->getNumberHistograms(); iw++) {
    inpWS->setBinEdges(iw, x);
    inpWS->mutableY(iw).assign(size - 1, 100.0);
  }

  return inpWS;
}
} // namespace
class GeneratePeaksTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GeneratePeaksTest *createSuite() { return new GeneratePeaksTest(); }
  static void destroySuite(GeneratePeaksTest *suite) { delete suite; }

  GeneratePeaksTest() { FrameworkManager::Instance(); }

  void test_Init() {
    GeneratePeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    DataObjects::TableWorkspace_sptr peakparmsws =
        createTestEffectiveFuncParameters();

    TS_ASSERT_EQUALS(peakparmsws->rowCount(), 4);

    API::Column_sptr col0 = peakparmsws->getColumn("spectrum");
    API::Column_sptr col1 = peakparmsws->getColumn("centre");

    TS_ASSERT_EQUALS((*col0)[2], 2);
    TS_ASSERT_DELTA((*col1)[1], 8.0, 1.0E-8);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test to use user-provided binning parameters and effective function
   * parameters
   */
  void test_UserBinningParameters() {
    // Create input parameter table workspace
    DataObjects::TableWorkspace_sptr peakparmsws =
        createTestEffectiveFuncParameters();
    AnalysisDataService::Instance().addOrReplace("TestPeakParameterTable",
                                                 peakparmsws);

    // Initialize algorithm GenertePeaks
    GeneratePeaks alg;
    alg.initialize();

    // Set value
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("PeakParametersWorkspace", peakparmsws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakType", "Gaussian"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundType", "Auto"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("BinningParameters", "0.0, 0.01, 10.0"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "Test01WS"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GenerateBackground", false));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IsRawParameter", false));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxAllowedChi2", 100.0));

    // Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get result/output workspace
    API::MatrixWorkspace_const_sptr peaksws =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("Test01WS"));
    TS_ASSERT(peaksws);

    // Check result
    TS_ASSERT_EQUALS(peaksws->getNumberHistograms(), 2);

    // peak 0:
    const auto &p0_x = peaksws->x(0);
    const auto &p0_y = peaksws->y(0);
    TS_ASSERT_DELTA(p0_x[200], 2.0, 1.0E-8);
    TS_ASSERT_DELTA(p0_y[200], 5.0, 1.0E-4);

    TS_ASSERT_DELTA(p0_x[201], 2.01, 1.0E-8);
    TS_ASSERT_DELTA(p0_y[201], 4.96546, 1.0E-4);

    // peak 1:
    TS_ASSERT_DELTA(p0_x[800], 8.0, 1.0E-8);
    TS_ASSERT_DELTA(p0_y[800], 10.0, 1.0E-4);

    // peak 2:
    const auto &p1_x = peaksws->x(1);
    const auto &p1_y = peaksws->y(1);
    TS_ASSERT_DELTA(p1_x[400], 4.0, 1.0E-8);
    TS_ASSERT_DELTA(p1_y[400], 20.0, 1.0E-4);

    // spectrum map
    spec2index_map themap = peaksws->getSpectrumToWorkspaceIndexMap();
    size_t index0 = themap[0];
    size_t index2 = themap[2];
    TS_ASSERT_EQUALS(index0, 0);
    TS_ASSERT_EQUALS(index2, 1);

    // Clearn
    AnalysisDataService::Instance().remove("Test01WS");
    AnalysisDataService::Instance().remove("TestPeakParameterTable");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test algorithm by using an existing input workspace as X-values
   */
  void test_FromInputWorkspace() {
    // Create input
    DataObjects::TableWorkspace_sptr peakparmsws = createTestPeakParameters2();
    AnalysisDataService::Instance().addOrReplace("TestParameterTable2",
                                                 peakparmsws);
    API::MatrixWorkspace_sptr inputws = createTestInputWorkspace();
    AnalysisDataService::Instance().addOrReplace("RawSampleBinWS", inputws);

    // Initialize algorithm class
    GeneratePeaks alg;
    alg.initialize();

    // Set value
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("PeakParametersWorkspace", peakparmsws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakType", "Gaussian"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundType", "Quadratic"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputws));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("BinningParameters", "0.0, 0.01, 10.0"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "Test02WS"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GenerateBackground", false));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxAllowedChi2", 100.0));

    // Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get result
    API::MatrixWorkspace_const_sptr peaksws =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("Test02WS"));
    TS_ASSERT(peaksws);

    // Check result
    TS_ASSERT_EQUALS(peaksws->getNumberHistograms(), 5);

    // Peak 0:
    const auto &p0_x = peaksws->x(0);
    const auto &p0_y = peaksws->y(0);
    TS_ASSERT_DELTA(p0_x[50], 2.0, 1.0E-8);
    TS_ASSERT_DELTA(p0_y[50], 5.0, 1.0E-4);

    TS_ASSERT_DELTA(p0_x[51], 2.02, 1.0E-8);
    TS_ASSERT_DELTA(p0_y[51], 4.86327, 1.0E-4);

    // Peak 1:
    TS_ASSERT_DELTA(p0_x[350], 8.0, 1.0E-8);
    TS_ASSERT_DELTA(p0_y[350], 10.0, 1.0E-4);

    // Peak 2:
    const auto &p1_x = peaksws->x(2);
    const auto &p1_y = peaksws->y(2);
    TS_ASSERT_DELTA(p1_x[150], 4.0, 1.0E-8);
    TS_ASSERT_DELTA(p1_y[150], 20.0, 1.0E-4);

    // Spectrum map
    spec2index_map themap = peaksws->getSpectrumToWorkspaceIndexMap();
    TS_ASSERT_EQUALS(themap.size(), 5);
    size_t index0 = themap[0];
    size_t index2 = themap[2];
    TS_ASSERT_EQUALS(index0, 0);
    TS_ASSERT_EQUALS(index2, 1);

    // Clearn
    AnalysisDataService::Instance().remove("TestParameterTable2");
    AnalysisDataService::Instance().remove("RawSampleBinWS");
    AnalysisDataService::Instance().remove("Test02WS");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test to use user-provided binning parameters
   */
  void test_Background() {
    // Create input
    DataObjects::TableWorkspace_sptr peakparmsws = createTestPeakParameters3();
    AnalysisDataService::Instance().addOrReplace("TestParameterTable3",
                                                 peakparmsws);

    // Init algorithm
    GeneratePeaks alg;
    alg.initialize();

    // Set value
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("PeakParametersWorkspace", peakparmsws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakType", "Gaussian"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundType", "Auto"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("BinningParameters", "0.0, 0.01, 10.0"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "Test03WS"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GenerateBackground", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxAllowedChi2", 100.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IsRawParameter", false));

    // Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get result
    API::MatrixWorkspace_const_sptr peaksws =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("Test03WS"));
    TS_ASSERT(peaksws);

    // Check result
    TS_ASSERT_EQUALS(peaksws->getNumberHistograms(), 2);

    // peak 0:
    const auto &p0_x = peaksws->x(0);
    const auto &p0_y = peaksws->y(0);
    TS_ASSERT_DELTA(p0_x[200], 2.0, 1.0E-8);
    TS_ASSERT_DELTA(p0_y[200], 10.0, 1.0E-4);

    // peak 1:
    TS_ASSERT_DELTA(p0_x[800], 8.0, 1.0E-8);
    TS_ASSERT_DELTA(p0_y[800], 20.0, 1.0E-4);

    // peak 2:
    const auto &p1_x = peaksws->x(1);
    const auto &p1_y = peaksws->y(1);
    TS_ASSERT_DELTA(p1_x[400], 4.0, 1.0E-8);
    TS_ASSERT_DELTA(p1_y[400], 24.0, 1.0E-4);

    // spectrum map
    spec2index_map themap = peaksws->getSpectrumToWorkspaceIndexMap();
    size_t index0 = themap[0];
    size_t index2 = themap[2];
    TS_ASSERT_EQUALS(index0, 0);
    TS_ASSERT_EQUALS(index2, 1);

    // Clean
    AnalysisDataService::Instance().remove("Test03WS");
    AnalysisDataService::Instance().remove("TestParameterTable3");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test to input parameter values by vectors user-provided binning parameters
   */
  void test_InputValueViaVector() {
    // Create vectors for peak and background parameters
    std::string vecpeakvalue("5.0, 2.0, 0.0849322");
    std::string vecbkgdvalue("1.0, 2.0, 0.0");

    // Initialize algorithm GenertePeaks
    GeneratePeaks alg;
    alg.initialize();

    // Set value
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("PeakParameterValues", vecpeakvalue));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("BackgroundParameterValues", vecbkgdvalue));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakType", "Gaussian"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundType", "Auto"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("BinningParameters", "0.0, 0.01, 10.0"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "Test04WS"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GenerateBackground", false));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IsRawParameter", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxAllowedChi2", 100.0));

    // Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get result/output workspace
    API::MatrixWorkspace_const_sptr peaksws =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("Test04WS"));
    TS_ASSERT(peaksws);

    // Check result
    TS_ASSERT_EQUALS(peaksws->getNumberHistograms(), 1);

    // peak 0:
    const auto &p0_x = peaksws->x(0);
    const auto &p0_y = peaksws->y(0);
    TS_ASSERT_DELTA(p0_x[200], 2.0, 1.0E-8);
    TS_ASSERT_DELTA(p0_y[200], 5.0, 1.0E-4);

    TS_ASSERT_DELTA(p0_x[201], 2.01, 1.0E-8);
    TS_ASSERT_DELTA(p0_y[201], 4.96546, 1.0E-4);

    // spectrum map
    spec2index_map themap = peaksws->getSpectrumToWorkspaceIndexMap();
    size_t index0 = themap[0];
    TS_ASSERT_EQUALS(index0, 0);

    AnalysisDataService::Instance().remove("Test04WS");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test to input parameter values by vectors user-provided binning parameters
   */
  void test_InputValueViaVectorEffective() {
    // Create vectors for peak and background parameters
    std::string vecpeakvalue("2.0, 5.0, 0.2");
    std::string vecbkgdvalue("1.0, 2.0, 0.0");

    // Initialize algorithm GenertePeaks
    GeneratePeaks alg;
    alg.initialize();

    // Set value
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("PeakParameterValues", vecpeakvalue));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("BackgroundParameterValues", vecbkgdvalue));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakType", "Gaussian"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundType", "Auto"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("BinningParameters", "0.0, 0.01, 10.0"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "Test01WS"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GenerateBackground", false));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IsRawParameter", false));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxAllowedChi2", 100.0));

    // Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get result/output workspace
    API::MatrixWorkspace_const_sptr peaksws =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("Test01WS"));
    TS_ASSERT(peaksws);

    // Check result
    TS_ASSERT_EQUALS(peaksws->getNumberHistograms(), 1);

    // peak 0:
    const auto &p0_x = peaksws->x(0);
    const auto &p0_y = peaksws->y(0);
    TS_ASSERT_DELTA(p0_x[200], 2.0, 1.0E-8);
    TS_ASSERT_DELTA(p0_y[200], 5.0, 1.0E-4);

    TS_ASSERT_DELTA(p0_x[201], 2.01, 1.0E-8);
    TS_ASSERT_DELTA(p0_y[201], 4.96546, 1.0E-4);

    // spectrum map
    spec2index_map themap = peaksws->getSpectrumToWorkspaceIndexMap();
    size_t index0 = themap[0];
    TS_ASSERT_EQUALS(index0, 0);

    AnalysisDataService::Instance().remove("Test01WS");

    return;
  }
};

class GeneratePeaksTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GeneratePeaksTestPerformance *createSuite() {
    return new GeneratePeaksTestPerformance();
  }
  static void destroySuite(GeneratePeaksTestPerformance *suite) {
    delete suite;
  }

  void setUp() override {
    FrameworkManager::Instance();
    peakparmsws = createTestPeakParameters2();
    AnalysisDataService::Instance().addOrReplace("TestParameterTable2",
                                                 peakparmsws);
    inputws = createTestInputWorkspace();
    AnalysisDataService::Instance().addOrReplace("RawSampleBinWS", inputws);
  }

  void tearDown() override {
    AnalysisDataService::Instance().remove("TestParameterTable2");
    AnalysisDataService::Instance().remove("RawSampleBinWS");
    AnalysisDataService::Instance().remove("Test02WS");
  }

  void testPerformance() {
    GeneratePeaks alg;
    alg.initialize();

    alg.setProperty("PeakParametersWorkspace", peakparmsws);
    alg.setProperty("PeakType", "Gaussian");
    alg.setProperty("BackgroundType", "Quadratic");
    alg.setProperty("InputWorkspace", inputws);

    alg.setPropertyValue("BinningParameters", "0.0, 0.01, 10.0");
    alg.setPropertyValue("OutputWorkspace", "Test02WS");
    alg.setProperty("GenerateBackground", false);
    alg.setProperty("MaxAllowedChi2", 100.0);

    alg.execute();
    TS_ASSERT(alg.isExecuted());
  }

private:
  DataObjects::TableWorkspace_sptr peakparmsws;
  API::MatrixWorkspace_sptr inputws;
};

#endif /* MANTID_ALGORITHMS_GENERATEPEAKSTEST_H_ */
