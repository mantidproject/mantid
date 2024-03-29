// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAlgorithms/CreateWorkspace.h"
#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidAlgorithms/Stitch1D.h"
#include "MantidAlgorithms/Stitch1DMany.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidHistogramData/Counts.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/Points.h"
#include "MantidKernel/UnitFactory.h"
#include <math.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::Algorithms::CreateWorkspace;
using Mantid::Algorithms::GroupWorkspaces;
using Mantid::Algorithms::Stitch1DMany;

class Stitch1DManyTest : public CxxTest::TestSuite {
private:
  /** Create a histogram workspace with two spectra and 10 bins. This can also
   * be run using the CreateWorkspace algorithm which leaves the output
   * workspace in the ADS as well.
   * @param xstart :: the first X value (common to both spectra)
   * @param deltax :: the bin width
   * @param value1 :: the Y counts in the first spectrum (constant for all X)
   * @param value2 :: the Y counts in the second spectrum (constant for all X)
   * @param runAlg :: set true to run the CreateWorkspace algorithm
   * @param outWSName :: output workspace name used if running CreateWorkspace
   */
  void createUniformWorkspace(double xstart, double deltax, double value1, double value2, const std::string &outWSName,
                              bool runAlg = false) {

    const int nbins = 10;
    std::vector<double> xData1(nbins + 1);
    std::vector<double> yData1(nbins);
    std::vector<double> eData1(nbins);
    std::vector<double> xData2(nbins + 1);
    std::vector<double> yData2(nbins);
    std::vector<double> eData2(nbins);

    for (int i = 0; i < nbins; i++) {
      // First spectrum
      xData1[i] = xstart + i * deltax;
      yData1[i] = value1;
      eData1[i] = std::sqrt(value1);
      // Second spectrum
      xData2[i] = xstart + i * deltax;
      yData2[i] = value2;
      eData2[i] = std::sqrt(value2);
    }
    xData1[nbins] = xData1[nbins - 1] + deltax;
    xData2[nbins] = xData2[nbins - 1] + deltax;

    MatrixWorkspace_sptr ws;

    if (!runAlg) {
      ws = WorkspaceFactory::Instance().create("Workspace2D", 2, nbins + 1, nbins);
      ws->dataX(0) = xData1;
      ws->dataX(1) = xData2;
      ws->dataY(0) = yData1;
      ws->dataY(1) = yData2;
      ws->dataE(0) = eData1;
      ws->dataE(1) = eData2;
      ws->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
    } else {
      // Concatenate data vectors into one vector
      xData1.insert(xData1.end(), xData2.begin(), xData2.end());
      yData1.insert(yData1.end(), yData2.begin(), yData2.end());
      eData1.insert(eData1.end(), eData2.begin(), eData2.end());

      CreateWorkspace cw;
      cw.initialize();
      cw.setProperty("DataX", xData1);
      cw.setProperty("DataY", yData1);
      cw.setProperty("DataE", eData1);
      cw.setProperty("NSpec", 2);
      cw.setProperty("UnitX", "Wavelength");
      cw.setProperty("OutputWorkspace", outWSName);
      cw.execute();

      ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    }
    AnalysisDataService::Instance().addOrReplace(outWSName, ws);
  }

  /** Groups workspaces using GroupWorkspaces algorithm. The output workpace is
   * left in the ADS as well.
   * @param inputWSNames :: input workspaces names
   * @param outputWSName :: output workspace name
   */
  void doGroupWorkspaces(const std::string &inputWSNames, const std::string &outWSName) {
    GroupWorkspaces gw;
    gw.initialize();
    gw.setProperty("InputWorkspaces", inputWSNames);
    gw.setProperty("OutputWorkspace", outWSName);
    gw.execute();
    auto ws = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outWSName);
    AnalysisDataService::Instance().addOrReplace(outWSName, ws);
  }

  /** Obtain all algorithm histories from a workspace
   * @param inputWS :: the input workspace
   * @return vector of names of algorithm histories
   */
  std::vector<std::string> getHistory(const MatrixWorkspace_sptr &inputWS) {
    std::vector<std::string> histNames;
    auto histories = inputWS->history().getAlgorithmHistories();
    for (auto &hist : histories) {
      histNames.emplace_back(hist->name());
    }
    return histNames;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static Stitch1DManyTest *createSuite() { return new Stitch1DManyTest(); }
  static void destroySuite(Stitch1DManyTest *suite) { delete suite; }

  Stitch1DManyTest() {}

  void test_testWorkspaces() {
    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws2");
    TS_ASSERT(AnalysisDataService::Instance().doesExist("ws1"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("ws2"));
    doGroupWorkspaces("ws1, ws2", "out");
    TS_ASSERT(AnalysisDataService::Instance().doesExist("out"));
    AnalysisDataService::Instance().clear();
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("out"));
  }

  void test_init() {
    Stitch1DMany alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_throws_with_too_few_workspaces() {
    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", "ws1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Params", "0.1, 0.1, 1.8"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "outws"));
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    AnalysisDataService::Instance().clear();
  }

  void test_throws_with_wrong_number_of_start_overlaps() {
    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws2");
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", "ws1, ws2"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Params", "0.1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StartOverlaps", "-0.5, -0.6"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EndOverlaps", "0.5"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "outws"));
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    AnalysisDataService::Instance().clear();
  }

  void test_throws_with_wrong_number_of_end_overlaps() {
    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws2");
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2");
    alg.setProperty("Params", "0.1");
    alg.setProperty("StartOverlaps", "-0.5");
    alg.setProperty("EndOverlaps", "0.5, 0.6");
    alg.setProperty("OutputWorkspace", "outws");
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    AnalysisDataService::Instance().clear();
  }

  void test_throws_with_wrong_number_of_given_scale_factors() {
    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws2");
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2");
    alg.setProperty("Params", "0.1");
    alg.setProperty("UseManualScaleFactors", "1");
    alg.setProperty("ManualScaleFactors", "0.5, 0.7");
    alg.setProperty("OutputWorkspace", "outws");
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    AnalysisDataService::Instance().clear();
  }

  void test_matrix_and_non_matrix_workspace_types_throws() {
    // One matrix workspace, one table workspace
    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    auto ws2 = WorkspaceFactory::Instance().createTable();
    AnalysisDataService::Instance().addOrReplace("ws2", ws2);
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2");
    alg.setProperty("Params", "0.1");
    alg.setProperty("OutputWorkspace", "outws");
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    AnalysisDataService::Instance().clear();
  }

  void test_group_and_non_group_workspace_types_throws() {
    // One group workspace, one matrix workspace
    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws2");
    doGroupWorkspaces("ws2", "group1");
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "group1, ws1");
    alg.setProperty("Params", "0.1");
    alg.setProperty("OutputWorkspace", "outws");
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    Stitch1DMany alg2;
    alg2.setChild(true);
    alg2.initialize();
    alg2.setProperty("InputWorkspaces", "ws1, group1");
    alg2.setProperty("Params", "0.1");
    alg2.setProperty("OutputWorkspace", "outws");
    TS_ASSERT_THROWS(alg2.execute(), const std::runtime_error &);
    AnalysisDataService::Instance().clear();
  }

  void test_group_containing_non_matrix_workspace_types_throws() {
    // One group workspace, one group workspace of non-matrix workspace types
    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    auto ws2 = WorkspaceFactory::Instance().createTable();
    AnalysisDataService::Instance().addOrReplace("ws2", ws2);
    doGroupWorkspaces("ws1", "group1");
    doGroupWorkspaces("ws2", "group2");
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "group1, group2");
    alg.setProperty("Params", "0.1");
    alg.setProperty("OutputWorkspace", "outws");
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    AnalysisDataService::Instance().clear();
  }

  void test_workspace_group_size_differ_throws() {

    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws2");
    createUniformWorkspace(1.6, 0.1, 1.5, 2.5, "ws3");
    doGroupWorkspaces("ws1, ws2", "group1");
    doGroupWorkspaces("ws3", "group2");
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "group1, group2");
    alg.setProperty("Params", "0.1, 0.1, 2.6");
    alg.setProperty("OutputWorkspace", "outws");
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    AnalysisDataService::Instance().clear();
  }

  void test_scale_factor_from_period_out_of_range_throws() {

    // First group
    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    createUniformWorkspace(0.1, 0.1, 1.5, 2.5, "ws2");
    doGroupWorkspaces("ws1, ws2", "group1");
    // Second group
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws3");
    createUniformWorkspace(0.8, 0.1, 1.6, 2.6, "ws4");
    doGroupWorkspaces("ws3, ws4", "group2");
    // Third group
    createUniformWorkspace(1.6, 0.1, 1.5, 2.5, "ws5");
    createUniformWorkspace(1.6, 0.1, 1.6, 3.0, "ws6");
    doGroupWorkspaces("ws5, ws6", "group3");
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "group1, group2, group3");
    alg.setProperty("Params", "0.1, 0.1, 2.6");
    alg.setProperty("ScaleFactorFromPeriod", 4);
    alg.setProperty("OutputWorkspace", "outws");
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    AnalysisDataService::Instance().clear();
  }

  void test_two_workspaces() {
    // Two matrix workspaces with two spectra each
    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws2");
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2");
    alg.setProperty("Params", "0.1, 0.1, 1.8");
    alg.setProperty("StartOverlaps", "0.8");
    alg.setProperty("EndOverlaps", "1.1");
    alg.setProperty("OutputWorkspace", "outws");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outws);
    auto stitched = std::dynamic_pointer_cast<MatrixWorkspace>(outws);
    TS_ASSERT_EQUALS(stitched->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(stitched->blocksize(), 17);
    TS_ASSERT_DELTA(stitched->y(0)[0], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[9], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[16], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[0], 2, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[9], 2, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[16], 2, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[0], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[9], 0.77919, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[16], 1.24316, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[0], 1.41421, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[9], 1.10982, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[16], 1.79063, 0.00001);
    // Test out scale factors
    std::vector<double> scales = alg.getProperty("OutScaleFactors");
    TS_ASSERT_EQUALS(scales.size(), 1);
    // Only scale factor for first spectrum is returned
    TS_ASSERT_DELTA(scales.front(), 0.90909, 0.00001);
    // If scale factor for second spectrum was returned it should be 0.952381

    // Cross-check that the result of using Stitch1DMany with two workspaces
    // is the same as using Stitch1D
    Mantid::Algorithms::Stitch1D alg2;
    alg2.setChild(true);
    alg2.initialize();
    alg2.setProperty("LHSWorkspace", "ws1");
    alg2.setProperty("RHSWorkspace", "ws2");
    alg2.setProperty("Params", "0.1, 0.1, 1.8");
    alg2.setProperty("StartOverlap", "0.8");
    alg2.setProperty("EndOverlap", "1.1");
    alg2.setProperty("OutputWorkspace", "outws");
    alg2.execute();
    MatrixWorkspace_sptr stitched2 = alg2.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(stitched->x(0).rawData(), stitched2->x(0).rawData());
    TS_ASSERT_EQUALS(stitched->y(0).rawData(), stitched2->y(0).rawData());
    TS_ASSERT_EQUALS(stitched->e(0).rawData(), stitched2->e(0).rawData());

    // Check workspaces in ADS
    auto wsInADS = AnalysisDataService::Instance().getObjectNames();
    // In ADS: ws1, ws2
    TS_ASSERT_EQUALS(wsInADS.size(), 2)
    // Remove workspaces from ADS
    AnalysisDataService::Instance().clear();
  }

  void test_three_workspaces() {
    // Three matrix workspaces with two spectra each

    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws2");
    createUniformWorkspace(1.6, 0.1, 1.5, 2.5, "ws3");

    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2, ws3");
    alg.setProperty("Params", "0.1, 0.1, 2.6");
    alg.setProperty("StartOverlaps", "0.8, 1.6");
    alg.setProperty("EndOverlaps", "1.1, 1.8");
    alg.setProperty("OutputWorkspace", "outws");
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outws);
    auto stitched = std::dynamic_pointer_cast<MatrixWorkspace>(outws);
    TS_ASSERT_EQUALS(stitched->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(stitched->blocksize(), 25);
    // First spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(0)[0], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[9], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[16], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[24], 1, 0.00001);
    // Second spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(1)[0], 2, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[9], 2, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[16], 2, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[24], 2, 0.00001);
    // First spectrum, E values
    TS_ASSERT_DELTA(stitched->e(0)[0], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[9], 0.77919, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[16], 0.90865, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[24], 1.33144, 0.00001);
    // Second spectrum, E values
    TS_ASSERT_DELTA(stitched->e(1)[0], 1.41421, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[9], 1.10982, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[16], 1.33430, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[24], 2.00079, 0.00001);

    // Test out scale factors
    std::vector<double> scales = alg.getProperty("OutScaleFactors");
    TS_ASSERT_EQUALS(scales.size(), 2);
    TS_ASSERT_DELTA(scales.front(), 0.9090, 0.0001);
    TS_ASSERT_DELTA(scales.back(), 0.6666, 0.0001);

    // Check workspaces in ADS
    auto wsInADS = AnalysisDataService::Instance().getObjectNames();
    // In ADS: ws1, ws2, ws3
    TS_ASSERT_EQUALS(wsInADS.size(), 3)
    // Remove workspaces from ADS
    AnalysisDataService::Instance().clear();
  }

  void test_stitches_three_no_overlaps_specified_should_still_work() {

    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws2");
    createUniformWorkspace(1.6, 0.1, 1.5, 2.5, "ws3");

    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2, ws3");
    alg.setProperty("Params", "0.1, 0.1, 2.6");
    alg.setProperty("OutputWorkspace", "outws");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    // Check workspaces in ADS
    auto wsInADS = AnalysisDataService::Instance().getObjectNames();
    // In ADS: ws1, ws2, ws3
    TS_ASSERT_EQUALS(wsInADS.size(), 3)
    // Remove workspaces from ADS
    AnalysisDataService::Instance().clear();
  }

  void test_three_workspaces_single_scale_factor_given() {
    // Three matrix workspaces with two spectra each

    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws2");
    createUniformWorkspace(1.6, 0.1, 1.5, 2.5, "ws3");

    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2, ws3");
    alg.setProperty("Params", "0.1, 0.1, 2.6");
    alg.setProperty("StartOverlaps", "0.8, 1.6");
    alg.setProperty("EndOverlaps", "1.1, 1.8");
    alg.setProperty("OutputWorkspace", "outws");
    alg.setProperty("UseManualScaleFactors", "1");
    alg.setProperty("ManualScaleFactors", "0.5");
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outws);
    auto stitched = std::dynamic_pointer_cast<MatrixWorkspace>(outws);
    TS_ASSERT_EQUALS(stitched->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(stitched->blocksize(), 25);
    // First spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(0)[0], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[10], 0.55000, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[18], 0.75000, 0.00001);
    // Second spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(1)[0], 2, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[10], 1.05000, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[18], 1.25000, 0.00001);
    // First spectrum, E values
    TS_ASSERT_DELTA(stitched->e(0)[0], 1.00000, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[10], 0.52440, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[18], 0.61237, 0.00001);
    // Second spectrum, E values
    TS_ASSERT_DELTA(stitched->e(1)[0], 1.41421, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[10], 0.72457, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[18], 0.79057, 0.00001);

    // Test out scale factors
    std::vector<double> scales = alg.getProperty("OutScaleFactors");
    TS_ASSERT_EQUALS(scales.size(), 2);
    TS_ASSERT_EQUALS(scales[0], 0.5);
    TS_ASSERT_EQUALS(scales[1], 0.5);

    auto wsInADS = AnalysisDataService::Instance().getObjectNames();
    // In ADS: ws1, ws2, ws3
    TS_ASSERT_EQUALS(wsInADS.size(), 3)
    // Remove workspaces from ADS
    AnalysisDataService::Instance().clear();
  }

  void test_three_workspaces_multiple_scale_factors_given() {
    // Three matrix workspaces with two spectra each

    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws2");
    createUniformWorkspace(1.6, 0.1, 1.5, 2.5, "ws3");

    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2, ws3");
    alg.setProperty("Params", "0.1, 0.1, 2.6");
    alg.setProperty("StartOverlaps", "0.8, 1.6");
    alg.setProperty("EndOverlaps", "1.1, 1.8");
    alg.setProperty("OutputWorkspace", "outws");
    alg.setProperty("UseManualScaleFactors", "1");
    alg.setProperty("ManualScaleFactors", "0.5, 0.7");
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outws);
    const auto stitched = std::dynamic_pointer_cast<MatrixWorkspace>(outws);

    TS_ASSERT_EQUALS(stitched->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(stitched->blocksize(), 25);
    // First spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(0)[0], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[10], 0.55, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[18], 1.05, 0.00001);
    // Second spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(1)[0], 2, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[10], 1.05, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[18], 1.75, 0.00001);
    // First spectrum, E values
    TS_ASSERT_DELTA(stitched->e(0)[0], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[10], 0.5244, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[18], 0.85732, 0.00001);
    // Second spectrum, E values
    TS_ASSERT_DELTA(stitched->e(1)[0], 1.41421, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[10], 0.72457, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[18], 1.1068, 0.00001);

    // Test out scale factors
    std::vector<double> scales = alg.getProperty("OutScaleFactors");
    TS_ASSERT_EQUALS(scales.size(), 2);
    TS_ASSERT_EQUALS(scales[0], 0.5);
    TS_ASSERT_EQUALS(scales[1], 0.7);

    // Check workspaces in ADS
    auto wsInADS = AnalysisDataService::Instance().getObjectNames();
    // In ADS: ws1, ws2, ws3
    TS_ASSERT_EQUALS(wsInADS.size(), 3)
    // Remove workspaces from ADS
    AnalysisDataService::Instance().clear();
  }

  void test_three_workspaces_scale_to_second() {
    // Three matrix workspaces with two spectra each,
    // scale the stiched output to the second workspace.

    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws2");
    createUniformWorkspace(1.6, 0.1, 1.5, 2.5, "ws3");

    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2, ws3");
    alg.setProperty("Params", "0.1, 0.1, 2.6");
    alg.setProperty("StartOverlaps", "0.8, 1.6");
    alg.setProperty("EndOverlaps", "1.1, 1.8");
    alg.setProperty("OutputWorkspace", "outws");
    alg.setProperty("IndexOfReference", "1");
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outws);
    const auto stitched = std::dynamic_pointer_cast<MatrixWorkspace>(outws);

    TS_ASSERT_EQUALS(stitched->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(stitched->blocksize(), 25);
    // First spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(0)[0], 1.10, 0.01);
    TS_ASSERT_DELTA(stitched->y(0)[10], 1.10, 0.01);
    TS_ASSERT_DELTA(stitched->y(0)[18], 1.10, 0.01);
    // Second spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(1)[0], 2.10, 0.01);
    TS_ASSERT_DELTA(stitched->y(1)[10], 2.10, 0.01);
    TS_ASSERT_DELTA(stitched->y(1)[18], 2.10, 0.01);
    // First spectrum, E values
    TS_ASSERT_DELTA(stitched->e(0)[0], 1.41, 0.01);
    TS_ASSERT_DELTA(stitched->e(0)[10], 1.05, 0.01);
    TS_ASSERT_DELTA(stitched->e(0)[18], 1.33, 0.01);
    // Second spectrum, E values
    TS_ASSERT_DELTA(stitched->e(1)[0], 1.91, 0.01);
    TS_ASSERT_DELTA(stitched->e(1)[10], 1.45, 0.01);
    TS_ASSERT_DELTA(stitched->e(1)[18], 1.92, 0.01);

    // Check workspaces in ADS
    auto wsInADS = AnalysisDataService::Instance().getObjectNames();
    // In ADS: ws1, ws2, ws3
    TS_ASSERT_EQUALS(wsInADS.size(), 3)
    // Remove workspaces from ADS
    AnalysisDataService::Instance().clear();
  }

  void test_one_group_two_workspaces() {
    // One group with two workspaces
    // Wrong: this algorithm can't stitch workspaces within a group

    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws2");
    doGroupWorkspaces("ws1, ws2", "group1");

    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "group1");
    alg.setProperty("Params", "0.1");
    alg.setProperty("StartOverlaps", "0.8");
    alg.setProperty("EndOverlaps", "1.1");
    alg.setProperty("OutputWorkspace", "outws");
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    AnalysisDataService::Instance().clear();
  }

  void test_groups_with_single_workspace() {
    // Three groups with a single matrix workspace each. Each matrix workspace
    // has two spectra.

    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws2");
    createUniformWorkspace(1.6, 0.1, 1.5, 2.5, "ws3");
    doGroupWorkspaces("ws1", "group1");
    doGroupWorkspaces("ws2", "group2");
    doGroupWorkspaces("ws3", "group3");

    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "group1, group2, group3");
    alg.setProperty("Params", "0.1, 0.1, 2.6");
    alg.setProperty("StartOverlaps", "0.8, 1.6");
    alg.setProperty("EndOverlaps", "1.1, 1.8");
    alg.setProperty("OutputWorkspace", "outws");
    alg.execute();
    TS_ASSERT(alg.isExecuted())

    // The above is equivalent to what we've done in test_three_workspaces()
    // so we should get the same values in the output workspace
    // the only difference is that output will be a group

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outws);
    auto group = std::dynamic_pointer_cast<WorkspaceGroup>(outws);
    TS_ASSERT_EQUALS(group->getNumberOfEntries(), 1);
    auto stitched = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
    TS_ASSERT_EQUALS(stitched->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(stitched->blocksize(), 25);
    // First spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(0)[0], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[9], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[16], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[24], 1, 0.00001);
    // Second spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(1)[0], 2, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[9], 2, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[16], 2, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[24], 2, 0.00001);
    // First spectrum, E values
    TS_ASSERT_DELTA(stitched->e(0)[0], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[9], 0.77919, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[16], 0.90865, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[24], 1.33144, 0.00001);
    // Second spectrum, E values
    TS_ASSERT_DELTA(stitched->e(1)[0], 1.41421, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[9], 1.10982, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[16], 1.33430, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[24], 2.00079, 0.00001);

    // Test out scale factors
    std::vector<double> scales = alg.getProperty("OutScaleFactors");
    TS_ASSERT_EQUALS(scales.size(), 2);
    TS_ASSERT_DELTA(scales.front(), 0.9090, 0.0001);
    TS_ASSERT_DELTA(scales.back(), 0.6666, 0.0001);

    // Check workspaces in ADS
    auto wsInADS = AnalysisDataService::Instance().getObjectNames();
    // In ADS: group1, group2, group3, ws1, ws2, ws3 and
    TS_ASSERT_EQUALS(wsInADS.size(), 8)
    TS_ASSERT_EQUALS(wsInADS[3], "outws")
    TS_ASSERT_EQUALS(wsInADS[4], "outws_1")
    // Clear the ADS
    AnalysisDataService::Instance().clear();
  }

  void test_two_groups_with_two_workspaces_each() {
    // Two groups with two matrix workspaces each.
    // Each matrix workspace has two spectra.

    // First group
    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    createUniformWorkspace(0.1, 0.1, 1.5, 2.5, "ws2");
    doGroupWorkspaces("ws1, ws2", "group1");
    // Second group
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws3");
    createUniformWorkspace(0.8, 0.1, 1.6, 2.6, "ws4");
    doGroupWorkspaces("ws3, ws4", "group2");

    // will produce a group outws containing two child workspaces
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "group1, group2");
    alg.setProperty("Params", "0.1");
    alg.setProperty("StartOverlaps", "0.8");
    alg.setProperty("EndOverlaps", "1.1");
    alg.setProperty("OutputWorkspace", "outws");
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outws);
    auto group = std::dynamic_pointer_cast<WorkspaceGroup>(outws);
    TS_ASSERT_EQUALS(group->getNumberOfEntries(), 2);

    // First item in the output group
    auto stitched = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
    TS_ASSERT_EQUALS(stitched->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(stitched->blocksize(), 17);
    // First spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(0)[0], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[9], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[16], 1, 0.00001);
    // Second spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(1)[0], 2, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[9], 2, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[16], 2, 0.00001);
    // First spectrum, E values
    TS_ASSERT_DELTA(stitched->e(0)[0], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[9], 0.77919, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[16], 1.24316, 0.00001);
    // Second spectrum, E values
    TS_ASSERT_DELTA(stitched->e(1)[0], 1.41421, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[9], 1.10982, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[16], 1.79063, 0.00001);

    // Second item in the output group
    stitched = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(1));
    TS_ASSERT_EQUALS(stitched->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(stitched->blocksize(), 17);
    // First spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(0)[0], 1.5, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[9], 1.5, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[16], 1.5, 0.00001);
    // Second spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(1)[0], 2.5, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[9], 2.5, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[16], 2.5, 0.00001);
    // First spectrum, E values
    TS_ASSERT_DELTA(stitched->e(0)[0], 1.22474, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[9], 0.95883, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[16], 1.54110, 0.00001);
    // Second spectrum, E values
    TS_ASSERT_DELTA(stitched->e(1)[0], 1.58114, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[9], 1.24263, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[16], 2.00959, 0.00001);

    // Test out scale factors
    std::vector<double> scales = alg.getProperty("OutScaleFactors");
    TS_ASSERT_EQUALS(scales.size(), 2);
    TS_ASSERT_DELTA(scales.front(), 0.9090, 0.0001);
    TS_ASSERT_DELTA(scales.back(), 0.9375, 0.0001);

    // Check workspaces in ADS
    auto wsInADS = AnalysisDataService::Instance().getObjectNames();
    // In ADS: group1, group2, ws1, ws2, ws3, ws4, and
    TS_ASSERT_EQUALS(wsInADS.size(), 9)
    TS_ASSERT_EQUALS(wsInADS[2], "outws")
    TS_ASSERT_EQUALS(wsInADS[3], "outws_1")
    TS_ASSERT_EQUALS(wsInADS[4], "outws_2")
    // Clear the ADS
    AnalysisDataService::Instance().clear();
  }

  void test_two_groups_with_two_workspaces_single_scale_factor_given() {
    // Two groups with two matrix workspaces each.
    // Each matrix workspace has two spectra.

    // First group
    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    createUniformWorkspace(0.1, 0.1, 1.5, 2.5, "ws2");
    doGroupWorkspaces("ws1, ws2", "group1");
    // Second group
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws3");
    createUniformWorkspace(0.8, 0.1, 1.6, 2.6, "ws4");
    doGroupWorkspaces("ws3, ws4", "group2");

    // Will produce a group outws containing two child workspaces
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "group1, group2");
    alg.setProperty("Params", "0.1");
    alg.setProperty("StartOverlaps", "0.8");
    alg.setProperty("EndOverlaps", "1.1");
    alg.setProperty("UseManualScaleFactors", "1");
    alg.setProperty("ManualScaleFactors", "0.5");
    alg.setProperty("OutputWorkspace", "outws");
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // The above is equivalent to what we've done in test_three_workspaces()
    // so we should get the same values in the output workspace
    // the only difference is that output will be a group

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outws);
    auto group = std::dynamic_pointer_cast<WorkspaceGroup>(outws);
    TS_ASSERT_EQUALS(group->getNumberOfEntries(), 2);

    // First item in the output group
    auto stitched = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
    TS_ASSERT_EQUALS(stitched->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(stitched->blocksize(), 17);
    // First spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(0)[0], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[9], 0.64705, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[16], 0.55000, 0.00001);
    // Second spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(1)[0], 2, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[9], 1.24752, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[16], 1.05000, 0.00001);
    // First spectrum, E values
    TS_ASSERT_DELTA(stitched->e(0)[0], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[9], 0.46442, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[16], 0.52440, 0.00001);
    // Second spectrum, E values
    TS_ASSERT_DELTA(stitched->e(1)[0], 1.41421, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[9], 0.64485, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[16], 0.72456, 0.00001);

    // Second item in the output group
    stitched = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(1));
    TS_ASSERT_EQUALS(stitched->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(stitched->blocksize(), 17);
    // First spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(0)[0], 1.5, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[9], 0.94736, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[16], 0.8, 0.00001);
    // Second spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(1)[0], 2.5, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[9], 1.54762, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[16], 1.3, 0.00001);
    // First spectrum, E values
    TS_ASSERT_DELTA(stitched->e(0)[0], 1.22474, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[9], 0.56195, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[16], 0.63245, 0.00001);
    // Second spectrum, E values
    TS_ASSERT_DELTA(stitched->e(1)[0], 1.58114, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[9], 0.71824, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[16], 0.80622, 0.00001);

    // Test out scale factors
    std::vector<double> scales = alg.getProperty("OutScaleFactors");
    TS_ASSERT_EQUALS(scales.size(), 2);
    TS_ASSERT_DELTA(scales.front(), 0.5000, 0.0001);
    TS_ASSERT_DELTA(scales.back(), 0.5000, 0.0001);

    // Check workspaces in ADS
    auto wsInADS = AnalysisDataService::Instance().getObjectNames();
    // In ADS: group1, group2, ws1, ws2, ws3, ws4 and
    TS_ASSERT_EQUALS(wsInADS.size(), 9)
    TS_ASSERT_EQUALS(wsInADS[2], "outws")
    TS_ASSERT_EQUALS(wsInADS[3], "outws_1")
    TS_ASSERT_EQUALS(wsInADS[4], "outws_2")
    // Clear the ADS
    AnalysisDataService::Instance().clear();
  }

  void test_two_groups_with_three_workspaces_multiple_scale_factors_given() {
    // Three groups with two matrix workspaces each.
    // Each matrix workspace has two spectra.

    // First group
    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    createUniformWorkspace(0.1, 0.1, 1.5, 2.5, "ws2");
    doGroupWorkspaces("ws1, ws2", "group1");
    // Second group
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws3");
    createUniformWorkspace(0.8, 0.1, 1.6, 2.6, "ws4");
    doGroupWorkspaces("ws3, ws4", "group2");
    // Third group
    createUniformWorkspace(1.6, 0.1, 1.5, 2.5, "ws5");
    createUniformWorkspace(1.6, 0.1, 1.6, 3.0, "ws6");
    doGroupWorkspaces("ws5, ws6", "group3");

    // Will produce a group outws containing two child workspaces
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "group1, group2, group3");
    alg.setProperty("Params", "0.1, 0.1, 2.6");
    alg.setProperty("StartOverlaps", "0.8, 1.6");
    alg.setProperty("EndOverlaps", "1.1, 1.9");
    alg.setProperty("UseManualScaleFactors", "1");
    alg.setProperty("ManualScaleFactors", "0.5, 0.7");
    alg.setProperty("OutputWorkspace", "outws");
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outws);
    auto group = std::dynamic_pointer_cast<WorkspaceGroup>(outws);
    TS_ASSERT_EQUALS(group->getNumberOfEntries(), 2);

    // First item in the output group
    auto stitched = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));

    TS_ASSERT_EQUALS(stitched->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(stitched->blocksize(), 25);
    // First spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(0)[0], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[9], 0.64706, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[16], 0.68614, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[24], 1.05, 0.00001);
    // Second spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(1)[0], 2, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[9], 1.24752, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[16], 1.26, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[24], 1.75, 0.00001);
    // First spectrum, E values
    TS_ASSERT_DELTA(stitched->e(0)[0], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[9], 0.46442, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[16], 0.44735, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[24], 0.85732, 0.00001);
    // Second spectrum, E values
    TS_ASSERT_DELTA(stitched->e(1)[0], 1.41421, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[9], 0.64486, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[16], 0.60622, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[24], 1.1068, 0.00001);

    // Second item in the output group
    stitched = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(1));

    TS_ASSERT_EQUALS(stitched->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(stitched->blocksize(), 25);
    // First spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(0)[0], 1.5, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[9], 0.94737, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[16], 0.90811, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[24], 1.12, 0.00001);
    // Second spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(1)[0], 2.5, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[9], 1.54762, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[16], 1.54528, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[24], 2.1, 0.00001);
    // First spectrum, E values
    TS_ASSERT_DELTA(stitched->e(0)[0], 1.22474, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[9], 0.56195, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[16], 0.51465, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[24], 0.88544, 0.00001);
    // Second spectrum, E values
    TS_ASSERT_DELTA(stitched->e(1)[0], 1.58114, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[9], 0.71824, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[16], 0.67135, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[24], 1.21244, 0.00001);

    // Test out scale factors
    std::vector<double> scales = alg.getProperty("OutScaleFactors");

    TS_ASSERT_EQUALS(scales.size(), 4);
    TS_ASSERT_DELTA(scales[0], 0.5, 0.0001);
    TS_ASSERT_DELTA(scales[1], 0.7, 0.0001);
    TS_ASSERT_DELTA(scales[2], 0.5, 0.0001);
    TS_ASSERT_DELTA(scales[3], 0.7, 0.0001);

    // Check workspaces in ADS
    auto wsInADS = AnalysisDataService::Instance().getObjectNames();
    // In ADS: group1, group2, grou3, ws1, ws2, ws3, ws4, ws5, ws6 and
    TS_ASSERT_EQUALS(wsInADS.size(), 12)
    TS_ASSERT_EQUALS(wsInADS[3], "outws")
    TS_ASSERT_EQUALS(wsInADS[4], "outws_1")
    TS_ASSERT_EQUALS(wsInADS[5], "outws_2")
    // Clear the ADS
    AnalysisDataService::Instance().clear();
  }

  void test_two_groups_with_three_workspaces_scale_factor_from_period() {
    // Three groups with two matrix workspaces each.
    // Each matrix workspace has two spectra.

    // First group
    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    createUniformWorkspace(0.1, 0.1, 1.5, 2.5, "ws2");
    doGroupWorkspaces("ws1, ws2", "group1");
    // Second group
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws3");
    createUniformWorkspace(0.8, 0.1, 1.6, 2.6, "ws4");
    doGroupWorkspaces("ws3, ws4", "group2");
    // Third group
    createUniformWorkspace(1.6, 0.1, 1.5, 2.5, "ws5");
    createUniformWorkspace(1.6, 0.1, 1.6, 3.0, "ws6");
    doGroupWorkspaces("ws5, ws6", "group3");

    // Will produce a group outws containing two child workspaces
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", "group1, group2, group3"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Params", "0.1, 0.1, 2.6"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StartOverlaps", "0.8, 1.6"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EndOverlaps", "1.1, 1.9"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UseManualScaleFactors", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ScaleFactorFromPeriod", 2));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "outws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // By keeping ManualScaleFactors empty (default value) it allows workspaces
    // in other periods to be scaled by scale factors from a specific period.
    // Periods 0 and 2 workspaces will be scaled by scale factors from period 1.

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outws);
    auto group = std::dynamic_pointer_cast<WorkspaceGroup>(outws);
    TS_ASSERT_EQUALS(group->getNumberOfEntries(), 2);

    // First item in the output group
    auto stitched = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
    TS_ASSERT_EQUALS(stitched->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(stitched->blocksize(), 25);
    // First spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(0)[0], 1., 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[9], 1.01589, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[16], 0.97288, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[24], 0.9375, 0.00001);
    // Second spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(1)[0], 2., 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[9], 1.98375, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[16], 1.70307, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[24], 1.56250, 0.00001);
    // First spectrum, E values
    TS_ASSERT_DELTA(stitched->e(0)[0], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[9], 0.70111, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[16], 0.60401, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[24], 0.76547, 0.00001);
    // Second spectrum, E values
    TS_ASSERT_DELTA(stitched->e(1)[0], 1.41421, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[9], 0.97973, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[16], 0.79916, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[24], 0.98821, 0.00001);

    // Second item in the output group
    stitched = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(1));

    TS_ASSERT_EQUALS(stitched->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(stitched->blocksize(), 25);
    // First spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(0)[0], 1.5, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[9], 1.5, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[16], 1.15385, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[24], 1., 0.00001);
    // Second spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(1)[0], 2.5, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[9], 2.46735, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[16], 2.06568, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[24], 1.87500, 0.00001);
    // First spectrum, E values
    TS_ASSERT_DELTA(stitched->e(0)[0], 1.22474, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[9], 0.85194, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[16], 0.65779, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[24], 0.79057, 0.00001);
    // Second spectrum, E values
    TS_ASSERT_DELTA(stitched->e(1)[0], 1.58114, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[9], 1.09265, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[16], 0.88013, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[24], 1.08253, 0.00001);

    // Test out scale factors
    std::vector<double> scales = alg.getProperty("OutScaleFactors");
    TS_ASSERT_EQUALS(scales.size(), 4);
    TS_ASSERT_DELTA(scales[0], 0.9375, 0.0001);
    TS_ASSERT_DELTA(scales[1], 0.6249, 0.0001);
    TS_ASSERT_DELTA(scales[2], 0.9375, 0.0001);
    TS_ASSERT_DELTA(scales[3], 0.6249, 0.0001);
    // Check workspaces in ADS
    auto wsInADS = AnalysisDataService::Instance().getObjectNames();
    TS_ASSERT_EQUALS(wsInADS.size(), 12)
    TS_ASSERT_EQUALS(wsInADS[3], "outws")
    TS_ASSERT_EQUALS(wsInADS[4], "outws_1")
    TS_ASSERT_EQUALS(wsInADS[5], "outws_2")
    // Clear the ADS
    AnalysisDataService::Instance().clear();
  }

  void test_groups_containing_single_workspaces_scale_factor_from_period() {
    // Three groups with one matrix workspaces each.
    // Each matrix workspace has two spectra.

    // First group
    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    doGroupWorkspaces("ws1", "group1");
    // Second group
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws3");
    doGroupWorkspaces("ws3", "group2");
    // Third group
    createUniformWorkspace(1.6, 0.1, 1.5, 2.5, "ws5");
    doGroupWorkspaces("ws5", "group3");
    // ws1 will be stitched with ws3 and ws5

    // Perid 2 is out of range, must be one like tested below
    Stitch1DMany alg0;
    alg0.setChild(true);
    alg0.initialize();
    alg0.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg0.setProperty("InputWorkspaces", "group1, group2, group3"))
    TS_ASSERT_THROWS_NOTHING(alg0.setProperty("Params", "0.1, 0.1, 2.6"))
    TS_ASSERT_THROWS_NOTHING(alg0.setProperty("StartOverlaps", "0.8, 1.6"))
    TS_ASSERT_THROWS_NOTHING(alg0.setProperty("EndOverlaps", "1.1, 1.9"))
    TS_ASSERT_THROWS_NOTHING(alg0.setProperty("UseManualScaleFactors", "1"))
    TS_ASSERT_THROWS_NOTHING(alg0.setProperty("ScaleFactorFromPeriod", 2))
    TS_ASSERT_THROWS_NOTHING(alg0.setProperty("OutputWorkspace", "outws"))
    TS_ASSERT_THROWS(alg0.execute(), const std::runtime_error &);
    TS_ASSERT(!alg0.isExecuted())

    // Will produce a group outws containing a single workspace
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", "group1, group2, group3"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Params", "0.1, 0.1, 2.6"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StartOverlaps", "0.8, 1.6"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EndOverlaps", "1.1, 1.9"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UseManualScaleFactors", "1"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ScaleFactorFromPeriod", 1))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "outws"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    // By keeping ManualScaleFactors empty (default value) it allows workspaces
    // in other periods to be scaled by scale factors from a specific period.
    // Periods 0 and 2 workspaces will be scaled by scale factors from period 1.

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outws)
    auto group = std::dynamic_pointer_cast<WorkspaceGroup>(outws);
    TS_ASSERT_EQUALS(group->getNumberOfEntries(), 1)

    // First item in the output group
    auto stitched = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
    TS_ASSERT_EQUALS(stitched->getNumberHistograms(), 2)
    TS_ASSERT_EQUALS(stitched->blocksize(), 25)
    // First spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(0)[0], 0.99999, 1.e-5)
    TS_ASSERT_DELTA(stitched->y(0)[9], 0.99999, 1.e-5)
    TS_ASSERT_DELTA(stitched->y(0)[16], 0.74860, 1.e-5)
    TS_ASSERT_DELTA(stitched->y(0)[24], 0.66666, 1.e-5)
    // Second spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(1)[0], 2., 1.e-5)
    TS_ASSERT_DELTA(stitched->y(1)[9], 1.95132, 1.e-5)
    TS_ASSERT_DELTA(stitched->y(1)[16], 1.28787, 1.e-5)
    TS_ASSERT_DELTA(stitched->y(1)[24], 1.11111, 1.e-5)
    // First spectrum, E values
    TS_ASSERT_DELTA(stitched->e(0)[0], 1., 1.e-5)
    TS_ASSERT_DELTA(stitched->e(0)[9], 0.69006, 1.e-5)
    TS_ASSERT_DELTA(stitched->e(0)[16], 0.47271, 1.e-5)
    TS_ASSERT_DELTA(stitched->e(0)[24], 0.54433, 1.e-5)
    // Second spectrum, E values
    TS_ASSERT_DELTA(stitched->e(1)[0], 1.414213, 1.e-5)
    TS_ASSERT_DELTA(stitched->e(1)[9], 0.963952, 1.e-5)
    TS_ASSERT_DELTA(stitched->e(1)[16], 0.62003, 1.e-5)
    TS_ASSERT_DELTA(stitched->e(1)[24], 0.702728, 1.e-5)

    // Test out scale factors
    std::vector<double> scales = alg.getProperty("OutScaleFactors");
    TS_ASSERT_EQUALS(scales.size(), 2);
    TS_ASSERT_DELTA(scales[0], 0.90909, 1.e-5)
    TS_ASSERT_DELTA(scales[1], 0.44444, 1.e-5)
    // Check workspaces in ADS
    auto wsInADS = AnalysisDataService::Instance().getObjectNames();
    // In ADS: group1, group2, group3, ws1, ws3, ws5
    TS_ASSERT_EQUALS(wsInADS.size(), 8)
    // Clear the ADS
    AnalysisDataService::Instance().clear();
  }

  void test_two_workspaces_history() {
    // This test is functionally similar to test_two_workspaces

    // Two matrix workspaces with two spectra each
    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1", true);
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws2", true);
    Stitch1DMany alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", "ws1, ws2"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Params", "0.1, 0.1, 1.8"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StartOverlaps", "0.8"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EndOverlaps", "1.1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "outws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Test output ws
    auto stitched = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outws");

    // Test the algorithm histories
    auto histNames = getHistory(stitched);
    TS_ASSERT_EQUALS(histNames.size(), 3)
    TS_ASSERT_EQUALS(histNames[0], "CreateWorkspace");
    TS_ASSERT_EQUALS(histNames[1], "CreateWorkspace");
    TS_ASSERT_EQUALS(histNames[2], "Stitch1DMany");

    // Check workspaces in ADS
    auto wsInADS = AnalysisDataService::Instance().getObjectNames();
    // In ADS: outws, ws1, ws2
    TS_ASSERT_EQUALS(wsInADS.size(), 3)
    TS_ASSERT_EQUALS(wsInADS[0], "outws")
    // Remove workspaces from ADS
    AnalysisDataService::Instance().clear();
  }

  void test_two_groups_history() {
    // This test is functionally similar to
    // test_two_groups_with_two_workspaces_each

    // Two groups with two matrix workspaces each.
    // Each matrix workspace has two spectra.

    // First group
    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1", true);
    createUniformWorkspace(0.1, 0.1, 1.5, 2.5, "ws2", true);
    doGroupWorkspaces("ws1, ws2", "group1");
    // Second group
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws3", true);
    createUniformWorkspace(0.8, 0.1, 1.6, 2.6, "ws4", true);
    doGroupWorkspaces("ws3, ws4", "group2");

    Stitch1DMany alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", "group1, group2"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Params", "0.1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StartOverlaps", "0.8"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EndOverlaps", "1.1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "outws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Test output ws
    auto group = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("outws");
    TS_ASSERT_EQUALS(group->getNumberOfEntries(), 2);

    // First item in the output group
    auto stitched = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));

    // Test the algorithm histories
    std::vector<std::string> histNames = getHistory(stitched);
    TS_ASSERT_EQUALS(histNames.size(), 7)
    TS_ASSERT_EQUALS(histNames[0], "CreateWorkspace");
    TS_ASSERT_EQUALS(histNames[1], "CreateWorkspace");
    TS_ASSERT_EQUALS(histNames[2], "GroupWorkspaces");
    TS_ASSERT_EQUALS(histNames[3], "CreateWorkspace");
    TS_ASSERT_EQUALS(histNames[4], "CreateWorkspace");
    TS_ASSERT_EQUALS(histNames[5], "GroupWorkspaces");
    TS_ASSERT_EQUALS(histNames[6], "Stitch1DMany");

    // Check workspaces in ADS
    auto wsInADS = AnalysisDataService::Instance().getObjectNames();
    TS_ASSERT_EQUALS(wsInADS.size(), 9)
    // Remove workspaces from ADS
    AnalysisDataService::Instance().clear();
  }

  void test_two_groups_scale_factor_from_period_history() {
    // This test is functionally similar to
    // test_two_groups_with_three_workspaces_scale_factor_from_period

    // Three groups with two matrix workspaces each.
    // Each matrix workspace has two spectra.

    // First group
    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1", true);
    createUniformWorkspace(0.1, 0.1, 1.5, 2.5, "ws2", true);
    doGroupWorkspaces("ws1, ws2", "group1");
    // Second group
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws3", true);
    createUniformWorkspace(0.8, 0.1, 1.6, 2.6, "ws4", true);
    doGroupWorkspaces("ws3, ws4", "group2");
    // Third group
    createUniformWorkspace(1.6, 0.1, 1.5, 2.5, "ws5", true);
    createUniformWorkspace(1.6, 0.1, 1.6, 3.0, "ws6", true);
    doGroupWorkspaces("ws5, ws6", "group3");

    Stitch1DMany alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", "group1, group2, group3"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Params", "0.1, 0.1, 2.6"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StartOverlaps", "0.8, 1.6"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EndOverlaps", "1.1, 1.9"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UseManualScaleFactors", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ScaleFactorFromPeriod", 2));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "outws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Test output ws
    auto group = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("outws");
    auto stitched = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));

    // Test the algorithm histories
    std::vector<std::string> histNames = getHistory(stitched);
    TS_ASSERT_EQUALS(histNames.size(), 10);
    TS_ASSERT_EQUALS(histNames[0], "CreateWorkspace");
    TS_ASSERT_EQUALS(histNames[1], "CreateWorkspace");
    TS_ASSERT_EQUALS(histNames[2], "GroupWorkspaces");
    TS_ASSERT_EQUALS(histNames[3], "CreateWorkspace");
    TS_ASSERT_EQUALS(histNames[4], "CreateWorkspace");
    TS_ASSERT_EQUALS(histNames[5], "GroupWorkspaces");
    TS_ASSERT_EQUALS(histNames[6], "CreateWorkspace");
    TS_ASSERT_EQUALS(histNames[7], "CreateWorkspace");
    TS_ASSERT_EQUALS(histNames[8], "GroupWorkspaces");
    TS_ASSERT_EQUALS(histNames[9], "Stitch1DMany");

    // Check workspaces in ADS
    auto wsInADS = AnalysisDataService::Instance().getObjectNames();
    // In ADS: group1, group2, group3,
    // ws1, ws2, ws3, ws4, ws5, ws6 and
    TS_ASSERT_EQUALS(wsInADS.size(), 12)
    TS_ASSERT_EQUALS(wsInADS[3], "outws")
    TS_ASSERT_EQUALS(wsInADS[4], "outws_1")
    TS_ASSERT_EQUALS(wsInADS[5], "outws_2")
    TS_ASSERT_EQUALS(wsInADS.size(), 12)
    // Clear the ADS
    AnalysisDataService::Instance().clear();
  }

  void test_two_groups_scale_to_second_entry() {
    // This test is functionally similar to
    // test_three_workspaces_scale_to_second,
    // but applies the logic to groups.

    // Three groups with two matrix workspaces each.
    // Each matrix workspace has two spectra.

    // First group
    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    createUniformWorkspace(0.1, 0.1, 1.5, 2.5, "ws2");
    doGroupWorkspaces("ws1, ws2", "group1");
    // Second group
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws3");
    createUniformWorkspace(0.8, 0.1, 1.6, 2.6, "ws4");
    doGroupWorkspaces("ws3, ws4", "group2");
    // Third group
    createUniformWorkspace(1.6, 0.1, 1.5, 2.5, "ws5");
    createUniformWorkspace(1.6, 0.1, 1.6, 3.0, "ws6");
    doGroupWorkspaces("ws5, ws6", "group3");

    // Will produce a group outws containing two child workspaces
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", "group1, group2, group3"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Params", "0.1, 0.1, 2.6"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StartOverlaps", "0.8, 1.6"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EndOverlaps", "1.1, 1.9"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IndexOfReference", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "outws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outws);
    auto group = std::dynamic_pointer_cast<WorkspaceGroup>(outws);
    TS_ASSERT_EQUALS(group->getNumberOfEntries(), 2);

    // First item in the output group
    auto stitched = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
    TS_ASSERT_EQUALS(stitched->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(stitched->blocksize(), 25);
    // First spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(0)[0], 1.099, 0.001);
    TS_ASSERT_DELTA(stitched->y(0)[9], 1.099, 0.001);
    TS_ASSERT_DELTA(stitched->y(0)[16], 0.878, 0.001);
    TS_ASSERT_DELTA(stitched->y(0)[24], 0.733, 0.001);
    // Second spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(1)[0], 2.099, 0.001);
    TS_ASSERT_DELTA(stitched->y(1)[9], 2.099, 0.001);
    TS_ASSERT_DELTA(stitched->y(1)[16], 1.693, 0.001);
    TS_ASSERT_DELTA(stitched->y(1)[24], 1.399, 0.001);
    // First spectrum, E values
    TS_ASSERT_DELTA(stitched->e(0)[0], 1.407, 0.001);
    TS_ASSERT_DELTA(stitched->e(0)[9], 0.841, 0.001);
    TS_ASSERT_DELTA(stitched->e(0)[16], 0.660, 0.001);
    TS_ASSERT_DELTA(stitched->e(0)[24], 0.850, 0.001);
    // Second spectrum, E values
    TS_ASSERT_DELTA(stitched->e(1)[0], 1.908, 0.001);
    TS_ASSERT_DELTA(stitched->e(1)[9], 1.154, 0.001);
    TS_ASSERT_DELTA(stitched->e(1)[16], 0.938, 0.001);
    TS_ASSERT_DELTA(stitched->e(1)[24], 1.230, 0.001);

    // Second item in the output group
    stitched = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(1));

    TS_ASSERT_EQUALS(stitched->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(stitched->blocksize(), 25);
    // First spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(0)[0], 1.600, 0.001);
    TS_ASSERT_DELTA(stitched->y(0)[9], 1.600, 0.001);
    TS_ASSERT_DELTA(stitched->y(0)[16], 1.306, 0.001);
    TS_ASSERT_DELTA(stitched->y(0)[24], 1.067, 0.001);
    // Second spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(1)[0], 2.600, 0.001);
    TS_ASSERT_DELTA(stitched->y(1)[9], 2.600, 0.001);
    TS_ASSERT_DELTA(stitched->y(1)[16], 2.100, 0.001);
    TS_ASSERT_DELTA(stitched->y(1)[24], 1.733, 0.001);
    // First spectrum, E values
    TS_ASSERT_DELTA(stitched->e(0)[0], 1.676, 0.001);
    TS_ASSERT_DELTA(stitched->e(0)[9], 1.010, 0.001);
    TS_ASSERT_DELTA(stitched->e(0)[16], 0.848, 0.001);
    TS_ASSERT_DELTA(stitched->e(0)[24], 1.142, 0.001);
    // Second spectrum, E values
    TS_ASSERT_DELTA(stitched->e(1)[0], 2.115, 0.001);
    TS_ASSERT_DELTA(stitched->e(1)[9], 1.282, 0.001);
    TS_ASSERT_DELTA(stitched->e(1)[16], 1.050, 0.001);
    TS_ASSERT_DELTA(stitched->e(1)[24], 1.383, 0.001);

    // Check workspaces in ADS
    auto wsInADS = AnalysisDataService::Instance().getObjectNames();
    // In ADS: group1, group2, grou3, ws1, ws2, ws3, ws4, ws5, ws6 and
    TS_ASSERT_EQUALS(wsInADS.size(), 12)
    TS_ASSERT_EQUALS(wsInADS[3], "outws")
    TS_ASSERT_EQUALS(wsInADS[4], "outws_1")
    TS_ASSERT_EQUALS(wsInADS[5], "outws_2")
    // Clear the ADS
    AnalysisDataService::Instance().clear();
  }

  void test_MatrixWorkspace_inputs_manualScaleFactors() {
    // If the input workspaces are MatrixWorkspaces and not WorkspaceGroups, the
    // user must specify ManualScaleFactors if UseManualScaleFactors is true.
    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1", true);
    createUniformWorkspace(0.1, 0.1, 1.5, 2.5, "ws2", true);
    Stitch1DMany alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", "ws1, ws2"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UseManualScaleFactors", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "ws"));
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_three_point_data_workspaces() {
    const auto &x1 = Mantid::HistogramData::Points({0.2, 0.9, 1.6});
    const auto &y1 = Mantid::HistogramData::Counts({56., 77., 48.});
    Mantid::HistogramData::Histogram histogram1(x1, y1);
    histogram1.setPointStandardDeviations(std::vector<double>{2., 1., 3.589});
    auto ws1 = std::make_shared<Mantid::DataObjects::Workspace2D>();
    ws1->initialize(1, std::move(histogram1));
    const auto &x2 = Mantid::HistogramData::Points({0.23, 1.3, 2.6});
    const auto &y2 = Mantid::HistogramData::Counts({1.1, 2., 3.7});
    Mantid::HistogramData::Histogram histogram2(x2, y2);
    histogram2.setPointStandardDeviations(std::vector<double>{1.34, 1.4, 3.1});
    auto ws2 = std::make_shared<Mantid::DataObjects::Workspace2D>();
    ws2->initialize(1, std::move(histogram2));
    Mantid::API::AnalysisDataService::Instance().addOrReplace("ws1", ws1);
    Mantid::API::AnalysisDataService::Instance().addOrReplace("ws2", ws2);
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", "ws1, ws2"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UseManualScaleFactors", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ManualScaleFactors", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "ws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outws);
    const auto stitched = std::dynamic_pointer_cast<MatrixWorkspace>(outws);
    const std::vector<double> x_values{0.2, 0.23, 0.9, 1.3, 1.6, 2.6};
    TS_ASSERT_EQUALS(stitched->x(0).rawData(), x_values);
    const std::vector<double> y_values{56., 1.1, 77., 2., 48., 3.7};
    TS_ASSERT_EQUALS(stitched->y(0).rawData(), y_values);
    const std::vector<double> dx_values{2., 1.34, 1., 1.4, 3.589, 3.1};
    TS_ASSERT_EQUALS(stitched->dx(0).rawData(), dx_values);
    // Check workspaces in ADS
    auto wsInADS = AnalysisDataService::Instance().getObjectNames();
    // In ADS: ws1, ws2
    TS_ASSERT_EQUALS(wsInADS.size(), 2)
    Mantid::API::AnalysisDataService::Instance().clear();
  }

  void test_two_workspaces_scale_to_last_ws() {
    // Two matrix workspaces with two spectra each
    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws2");
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2");
    alg.setProperty("Params", "0.1, 0.1, 1.8");
    alg.setProperty("StartOverlaps", "0.8");
    alg.setProperty("EndOverlaps", "1.1");
    alg.setProperty("IndexOfReference", "-1");
    alg.setProperty("OutputWorkspace", "outws");
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outws);
    auto stitched = std::dynamic_pointer_cast<MatrixWorkspace>(outws);
    TS_ASSERT_EQUALS(stitched->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(stitched->blocksize(), 17);
    TS_ASSERT_DELTA(stitched->y(0)[0], 1.1, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[9], 1.1, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[16], 1.1, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[0], 2.1, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[9], 2.1, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[16], 2.1, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[0], 1.40712, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[9], 0.84091, 0.00001);
    TS_ASSERT_DELTA(stitched->e(0)[16], 1.04880, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[0], 1.90787, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[9], 1.15399, 0.00001);
    TS_ASSERT_DELTA(stitched->e(1)[16], 1.44913, 0.00001);
    // Test out scale factors
    std::vector<double> scales = alg.getProperty("OutScaleFactors");
    TS_ASSERT_EQUALS(scales.size(), 1);
    // Only scale factor for first spectrum is returned
    TS_ASSERT_DELTA(scales.front(), 1.09999, 0.00001);

    // Cross-check that the result of using Stitch1DMany with two workspaces
    // is the same as using Stitch1D
    Mantid::Algorithms::Stitch1D alg2;
    alg2.setChild(true);
    alg2.initialize();
    alg2.setProperty("LHSWorkspace", "ws1");
    alg2.setProperty("RHSWorkspace", "ws2");
    alg2.setProperty("Params", "0.1, 0.1, 1.8");
    alg2.setProperty("StartOverlap", "0.8");
    alg2.setProperty("EndOverlap", "1.1");
    alg2.setProperty("ScaleRHSWorkspace", "0");
    alg2.setProperty("OutputWorkspace", "outws");
    alg2.execute();
    MatrixWorkspace_sptr stitched2 = alg2.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(stitched->x(0).rawData(), stitched2->x(0).rawData());
    TS_ASSERT_EQUALS(stitched->y(0).rawData(), stitched2->y(0).rawData());
    TS_ASSERT_EQUALS(stitched->e(0).rawData(), stitched2->e(0).rawData());

    // Check workspaces in ADS
    auto wsInADS = AnalysisDataService::Instance().getObjectNames();
    // In ADS: ws1, ws2
    TS_ASSERT_EQUALS(wsInADS.size(), 2)
    // Remove workspaces from ADS
    AnalysisDataService::Instance().clear();
  }

  void test_three_workspaces_scale_to_last_ws() {
    // Three matrix workspaces with two spectra each,
    createUniformWorkspace(0.1, 0.1, 1., 2., "ws1");
    createUniformWorkspace(0.8, 0.1, 1.1, 2.1, "ws2");
    createUniformWorkspace(1.6, 0.1, 1.5, 2.5, "ws3");

    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2, ws3");
    alg.setProperty("Params", "0.1, 0.1, 2.6");
    alg.setProperty("StartOverlaps", "0.8, 1.6");
    alg.setProperty("EndOverlaps", "1.1, 1.8");
    alg.setProperty("OutputWorkspace", "outws");
    alg.setProperty("IndexOfReference", "-1");
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outws);
    const auto stitched = std::dynamic_pointer_cast<MatrixWorkspace>(outws);

    TS_ASSERT_EQUALS(stitched->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(stitched->blocksize(), 25);
    // First spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(0)[0], 1.50, 0.01);
    TS_ASSERT_DELTA(stitched->y(0)[10], 1.50, 0.01);
    TS_ASSERT_DELTA(stitched->y(0)[18], 1.50, 0.01);
    // Second spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(1)[0], 2.50, 0.01);
    TS_ASSERT_DELTA(stitched->y(1)[10], 2.50, 0.01);
    TS_ASSERT_DELTA(stitched->y(1)[18], 2.50, 0.01);
    // First spectrum, E values
    TS_ASSERT_DELTA(stitched->e(0)[0], 2.33, 0.01);
    TS_ASSERT_DELTA(stitched->e(0)[10], 1.95, 0.01);
    TS_ASSERT_DELTA(stitched->e(0)[18], 1.22, 0.01);
    // Second spectrum, E values
    TS_ASSERT_DELTA(stitched->e(1)[0], 2.81, 0.01);
    TS_ASSERT_DELTA(stitched->e(1)[10], 2.39, 0.01);
    TS_ASSERT_DELTA(stitched->e(1)[18], 1.58, 0.01);

    // Check workspaces in ADS
    auto wsInADS = AnalysisDataService::Instance().getObjectNames();
    // In ADS: ws1, ws2, ws3
    TS_ASSERT_EQUALS(wsInADS.size(), 3)
    // Remove workspaces from ADS
    AnalysisDataService::Instance().clear();
  }
};
