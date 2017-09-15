#ifndef MANTID_ALGORITHMS_STITCH1DMANYTEST_H_
#define MANTID_ALGORITHMS_STITCH1DMANYTEST_H_

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
#include "MantidKernel/UnitFactory.h"
#include <math.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::Algorithms::Stitch1DMany;
using Mantid::Algorithms::CreateWorkspace;
using Mantid::Algorithms::GroupWorkspaces;

class Stitch1DManyTest : public CxxTest::TestSuite {
private:
  /** Create a histogram workspace with two spectra and 10 bins. This can also
  * be run using the CreateWorkspace algorithm which leaves the output workspace
  * in the ADS as well.
  * @param xstart :: the first X value (common to both spectra)
  * @param deltax :: the bin width
  * @param value1 :: the Y counts in the first spectrum (constant for all X)
  * @param value2 :: the Y counts in the second spectrum (constant for all X)
  * @param runAlg :: set true to run the CreateWorkspace algorithm
  * @oaram outWSName :: output workspace name used if running CreateWorkspace
  */
  MatrixWorkspace_sptr createUniformWorkspace(double xstart, double deltax,
                                              double value1, double value2,
                                              bool runAlg = false,
                                              std::string outWSName = "") {

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
      ws = WorkspaceFactory::Instance().create("Workspace2D", 2, nbins + 1,
                                               nbins);
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
      cw.setPropertyValue("OutputWorkspace", outWSName);
      cw.execute();

      ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          outWSName);
    }

    return ws;
  }

  /** Groups workspaces using GroupWorkspaces algorithm. The output workpace is
  * left in the ADS as well.
  * @param inputWSNames :: input workspaces names
  * @param outputWSName :: output workspace name
  */
  WorkspaceGroup_sptr doGroupWorkspaces(std::string inputWSNames,
                                        std::string outWSName) {
    GroupWorkspaces gw;
    gw.initialize();
    gw.setProperty("InputWorkspaces", inputWSNames);
    gw.setProperty("OutputWorkspace", outWSName);
    gw.execute();

    WorkspaceGroup_sptr ws =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outWSName);
    return ws;
  }

  /** Obtain all algorithm histories from a workspace
  * @param inputWS :: the input workspace
  * @return vector of names of algorithm histories
  */
  std::vector<std::string> getHistory(MatrixWorkspace_sptr inputWS) {
    std::vector<std::string> histNames;
    auto histories = inputWS->history().getAlgorithmHistories();
    for (auto &hist : histories) {
      histNames.push_back(hist->name());
    }
    return histNames;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static Stitch1DManyTest *createSuite() { return new Stitch1DManyTest(); }
  static void destroySuite(Stitch1DManyTest *suite) { delete suite; }

  Stitch1DManyTest() {
    auto ws1 = createUniformWorkspace(0.1, 0.1, 1., 2.);
    auto ws2 = createUniformWorkspace(0.8, 0.1, 1.1, 2.1);
    AnalysisDataService::Instance().addOrReplace("ws1", ws1);
    AnalysisDataService::Instance().addOrReplace("ws2", ws2);
  }

  void test_init() {
    Stitch1DMany alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_throws_with_too_few_workspaces() {
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1");
    alg.setProperty("Params", "0.1, 0.1, 1.8");
    alg.setPropertyValue("OutputWorkspace", "outws");
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
  }

  void test_throws_with_wrong_number_of_start_overlaps() {
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2");
    alg.setProperty("Params", "0.1");
    alg.setProperty("StartOverlaps", "-0.5, -0.6");
    alg.setProperty("EndOverlaps", "0.5");
    alg.setPropertyValue("OutputWorkspace", "outws");
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
  }

  void test_throws_with_wrong_number_of_end_overlaps() {
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2");
    alg.setProperty("Params", "0.1");
    alg.setProperty("StartOverlaps", "-0.5");
    alg.setProperty("EndOverlaps", "0.5, 0.6");
    alg.setPropertyValue("OutputWorkspace", "outws");
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
  }

  void test_throws_with_wrong_number_of_given_scale_factors() {
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2");
    alg.setProperty("Params", "0.1");
    alg.setProperty("UseManualScaleFactors", "1");
    alg.setProperty("ManualScaleFactors", "0.5, 0.7");
    alg.setPropertyValue("OutputWorkspace", "outws");
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
  }

  void test_matrix_and_non_matrix_workspace_types_throws() {
    // One matrix workspace, one table workspace
    auto ws1 = createUniformWorkspace(0.1, 0.1, 1., 2.);
    auto ws2 = WorkspaceFactory::Instance().createTable();
    AnalysisDataService::Instance().addOrReplace("ws1", ws1);
    AnalysisDataService::Instance().addOrReplace("ws2", ws2);
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2");
    alg.setProperty("Params", "0.1");
    alg.setPropertyValue("OutputWorkspace", "outws");
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
  }

  void test_group_and_non_group_workspace_types_throws() {
    // One group workspace, one matrix workspace
    auto ws1 = createUniformWorkspace(0.1, 0.1, 1., 2.);
    auto ws2 = createUniformWorkspace(0.8, 0.1, 1.1, 2.1);
    WorkspaceGroup_sptr group1 = boost::make_shared<WorkspaceGroup>();
    group1->addWorkspace(ws1);
    AnalysisDataService::Instance().addOrReplace("group1", group1);
    AnalysisDataService::Instance().addOrReplace("ws1", ws1);
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "group1, ws1");
    alg.setProperty("Params", "0.1");
    alg.setPropertyValue("OutputWorkspace", "outws");
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
  }

  void test_group_containing_non_matrix_workspace_types_throws() {
    // One group workspace, one group workspace of non-matrix workspace types
    auto ws1 = createUniformWorkspace(0.1, 0.1, 1., 2.);
    auto ws2 = WorkspaceFactory::Instance().createTable();
    WorkspaceGroup_sptr group1 = boost::make_shared<WorkspaceGroup>();
    group1->addWorkspace(ws1);
    WorkspaceGroup_sptr group2 = boost::make_shared<WorkspaceGroup>();
    group2->addWorkspace(ws2);
    AnalysisDataService::Instance().addOrReplace("group1", group1);
    AnalysisDataService::Instance().addOrReplace("group2", group2);
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "group1, group2");
    alg.setProperty("Params", "0.1");
    alg.setPropertyValue("OutputWorkspace", "outws");
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
  }

  void test_workspace_group_size_differ_throws() {

    auto ws1 = createUniformWorkspace(0.1, 0.1, 1., 2.);
    auto ws2 = createUniformWorkspace(0.8, 0.1, 1.1, 2.1);
    auto ws3 = createUniformWorkspace(1.6, 0.1, 1.5, 2.5);
    WorkspaceGroup_sptr group1 = boost::make_shared<WorkspaceGroup>();
    group1->addWorkspace(ws1);
    group1->addWorkspace(ws2);
    WorkspaceGroup_sptr group2 = boost::make_shared<WorkspaceGroup>();
    group2->addWorkspace(ws3);
    // The algorithm needs the workspaces to be in the ADS
    AnalysisDataService::Instance().addOrReplace("group1", group1);
    AnalysisDataService::Instance().addOrReplace("group2", group2);

    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "group1, group2");
    alg.setProperty("Params", "0.1, 0.1, 2.6");
    alg.setPropertyValue("OutputWorkspace", "outws");
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
  }

  void test_scale_factor_from_period_out_of_range_throws() {

    // First group
    auto ws1 = createUniformWorkspace(0.1, 0.1, 1., 2.);
    auto ws2 = createUniformWorkspace(0.1, 0.1, 1.5, 2.5);
    WorkspaceGroup_sptr group1 = boost::make_shared<WorkspaceGroup>();
    group1->addWorkspace(ws1);
    group1->addWorkspace(ws2);
    // Second group
    auto ws3 = createUniformWorkspace(0.8, 0.1, 1.1, 2.1);
    auto ws4 = createUniformWorkspace(0.8, 0.1, 1.6, 2.6);
    WorkspaceGroup_sptr group2 = boost::make_shared<WorkspaceGroup>();
    group2->addWorkspace(ws3);
    group2->addWorkspace(ws4);
    // Third group
    auto ws5 = createUniformWorkspace(1.6, 0.1, 1.5, 2.5);
    auto ws6 = createUniformWorkspace(1.6, 0.1, 1.6, 3.0);
    WorkspaceGroup_sptr group3 = boost::make_shared<WorkspaceGroup>();
    group3->addWorkspace(ws5);
    group3->addWorkspace(ws6);

    // The algorithm needs the workspaces to be in the ADS
    AnalysisDataService::Instance().addOrReplace("group1", group1);
    AnalysisDataService::Instance().addOrReplace("group2", group2);
    AnalysisDataService::Instance().addOrReplace("group3", group3);

    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "group1, group2, group3");
    alg.setProperty("Params", "0.1, 0.1, 2.6");
    alg.setProperty("ScaleFactorFromPeriod", 4);
    alg.setPropertyValue("OutputWorkspace", "outws");
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
  }

  void test_two_workspaces() {
    // Two matrix workspaces with two spectra each

    auto ws1 = createUniformWorkspace(0.1, 0.1, 1., 2.);
    auto ws2 = createUniformWorkspace(0.8, 0.1, 1.1, 2.1);
    // The algorithm needs the workspaces to be in the ADS
    AnalysisDataService::Instance().addOrReplace("ws1", ws1);
    AnalysisDataService::Instance().addOrReplace("ws2", ws2);

    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2");
    alg.setProperty("Params", "0.1, 0.1, 1.8");
    alg.setProperty("StartOverlaps", "0.8");
    alg.setProperty("EndOverlaps", "1.1");
    alg.setPropertyValue("OutputWorkspace", "outws");
    alg.execute();

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    auto stitched = boost::dynamic_pointer_cast<MatrixWorkspace>(outws);
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
    alg2.setProperty("LHSWorkspace", ws1);
    alg2.setProperty("RHSWorkspace", ws2);
    alg2.setProperty("Params", "0.1, 0.1, 1.8");
    alg2.setProperty("StartOverlap", "0.8");
    alg2.setProperty("EndOverlap", "1.1");
    alg2.setPropertyValue("OutputWorkspace", "outws");
    alg2.execute();
    MatrixWorkspace_sptr stitched2 = alg2.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(stitched->x(0).rawData(), stitched2->x(0).rawData());
    TS_ASSERT_EQUALS(stitched->y(0).rawData(), stitched2->y(0).rawData());
    TS_ASSERT_EQUALS(stitched->e(0).rawData(), stitched2->e(0).rawData());

    // Remove workspaces from ADS
    AnalysisDataService::Instance().remove("ws1");
    AnalysisDataService::Instance().remove("ws2");
  }

  void test_three_workspaces() {
    // Three matrix workspaces with two spectra each

    auto ws1 = createUniformWorkspace(0.1, 0.1, 1., 2.);
    auto ws2 = createUniformWorkspace(0.8, 0.1, 1.1, 2.1);
    auto ws3 = createUniformWorkspace(1.6, 0.1, 1.5, 2.5);
    // The algorithm needs the workspaces to be in the ADS
    AnalysisDataService::Instance().addOrReplace("ws1", ws1);
    AnalysisDataService::Instance().addOrReplace("ws2", ws2);
    AnalysisDataService::Instance().addOrReplace("ws3", ws3);

    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2, ws3");
    alg.setProperty("Params", "0.1, 0.1, 2.6");
    alg.setProperty("StartOverlaps", "0.8, 1.6");
    alg.setProperty("EndOverlaps", "1.1, 1.8");
    alg.setPropertyValue("OutputWorkspace", "outws");
    alg.execute();

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    auto stitched = boost::dynamic_pointer_cast<MatrixWorkspace>(outws);
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

    // Remove workspaces from ADS
    AnalysisDataService::Instance().remove("ws1");
    AnalysisDataService::Instance().remove("ws2");
    AnalysisDataService::Instance().remove("ws3");
  }

  void test_stitches_three_no_overlaps_specified_should_still_work() {

    auto ws1 = createUniformWorkspace(0.1, 0.1, 1., 2.);
    auto ws2 = createUniformWorkspace(0.8, 0.1, 1.1, 2.1);
    auto ws3 = createUniformWorkspace(1.6, 0.1, 1.5, 2.5);
    // The algorithm needs the workspaces to be in the ADS
    AnalysisDataService::Instance().addOrReplace("ws1", ws1);
    AnalysisDataService::Instance().addOrReplace("ws2", ws2);
    AnalysisDataService::Instance().addOrReplace("ws3", ws3);

    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2, ws3");
    alg.setProperty("Params", "0.1, 0.1, 2.6");
    alg.setPropertyValue("OutputWorkspace", "outws");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

  void test_three_workspaces_single_scale_factor_given() {
    // Three matrix workspaces with two spectra each

    auto ws1 = createUniformWorkspace(0.1, 0.1, 1., 2.);
    auto ws2 = createUniformWorkspace(0.8, 0.1, 1.1, 2.1);
    auto ws3 = createUniformWorkspace(1.6, 0.1, 1.5, 2.5);
    // The algorithm needs the workspaces to be in the ADS
    AnalysisDataService::Instance().addOrReplace("ws1", ws1);
    AnalysisDataService::Instance().addOrReplace("ws2", ws2);
    AnalysisDataService::Instance().addOrReplace("ws3", ws3);

    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2, ws3");
    alg.setProperty("Params", "0.1, 0.1, 2.6");
    alg.setProperty("StartOverlaps", "0.8, 1.6");
    alg.setProperty("EndOverlaps", "1.1, 1.8");
    alg.setPropertyValue("OutputWorkspace", "outws");
    alg.setProperty("UseManualScaleFactors", "1");
    alg.setProperty("ManualScaleFactors", "0.5");
    alg.execute();

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    auto stitched = boost::dynamic_pointer_cast<MatrixWorkspace>(outws);
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

    // Remove workspaces from ADS
    AnalysisDataService::Instance().remove("ws1");
    AnalysisDataService::Instance().remove("ws2");
    AnalysisDataService::Instance().remove("ws3");
  }

  void test_three_workspaces_multiple_scale_factors_given() {
    // Three matrix workspaces with two spectra each

    auto ws1 = createUniformWorkspace(0.1, 0.1, 1., 2.);
    auto ws2 = createUniformWorkspace(0.8, 0.1, 1.1, 2.1);
    auto ws3 = createUniformWorkspace(1.6, 0.1, 1.5, 2.5);
    // The algorithm needs the workspaces to be in the ADS
    AnalysisDataService::Instance().addOrReplace("ws1", ws1);
    AnalysisDataService::Instance().addOrReplace("ws2", ws2);
    AnalysisDataService::Instance().addOrReplace("ws3", ws3);

    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2, ws3");
    alg.setProperty("Params", "0.1, 0.1, 2.6");
    alg.setProperty("StartOverlaps", "0.8, 1.6");
    alg.setProperty("EndOverlaps", "1.1, 1.8");
    alg.setPropertyValue("OutputWorkspace", "outws");
    alg.setProperty("UseManualScaleFactors", "1");
    alg.setProperty("ManualScaleFactors", "0.5, 0.7");
    alg.execute();

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    auto stitched = boost::dynamic_pointer_cast<MatrixWorkspace>(outws);

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

    // Remove workspaces from ADS
    AnalysisDataService::Instance().remove("ws1");
    AnalysisDataService::Instance().remove("ws2");
    AnalysisDataService::Instance().remove("ws3");
  }

  void test_one_group_two_workspaces() {
    // One group with two workspaces
    // Wrong: this algorithm can't stitch workspaces within a group

    auto ws1 = createUniformWorkspace(0.1, 0.1, 1., 2.);
    auto ws2 = createUniformWorkspace(0.8, 0.1, 1.1, 2.1);
    WorkspaceGroup_sptr group = boost::make_shared<WorkspaceGroup>();
    group->addWorkspace(ws1);
    group->addWorkspace(ws2);
    // The algorithm needs the workspaces to be in the ADS
    AnalysisDataService::Instance().addOrReplace("group1", group);

    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "group1");
    alg.setProperty("Params", "0.1");
    alg.setProperty("StartOverlaps", "0.8");
    alg.setProperty("EndOverlaps", "1.1");
    alg.setPropertyValue("OutputWorkspace", "outws");
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);

    AnalysisDataService::Instance().clear();
  }

  void test_groups_with_single_workspace() {
    // Three groups with a single matrix workspace each. Each matrix workspace
    // has two spectra.

    auto ws1 = createUniformWorkspace(0.1, 0.1, 1., 2.);
    auto ws2 = createUniformWorkspace(0.8, 0.1, 1.1, 2.1);
    auto ws3 = createUniformWorkspace(1.6, 0.1, 1.5, 2.5);
    WorkspaceGroup_sptr group1 = boost::make_shared<WorkspaceGroup>();
    group1->addWorkspace(ws1);
    WorkspaceGroup_sptr group2 = boost::make_shared<WorkspaceGroup>();
    group2->addWorkspace(ws2);
    WorkspaceGroup_sptr group3 = boost::make_shared<WorkspaceGroup>();
    group3->addWorkspace(ws3);
    // The algorithm needs the workspaces to be in the ADS
    AnalysisDataService::Instance().addOrReplace("group1", group1);
    AnalysisDataService::Instance().addOrReplace("group2", group2);
    AnalysisDataService::Instance().addOrReplace("group3", group3);

    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "group1, group2, group3");
    alg.setProperty("Params", "0.1, 0.1, 2.6");
    alg.setProperty("StartOverlaps", "0.8, 1.6");
    alg.setProperty("EndOverlaps", "1.1, 1.8");
    alg.setPropertyValue("OutputWorkspace", "outws");
    alg.execute();

    // The above is equivalent to what we've done in test_three_workspaces()
    // so we should get the same values in the output workspace
    // the only difference is that output will be a group

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(outws);
    TS_ASSERT_EQUALS(group->getNumberOfEntries(), 1);
    auto stitched =
        boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
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

    // Clear the ADS
    AnalysisDataService::Instance().clear();
  }

  void test_two_groups_with_two_workspaces_each() {
    // Two groups with two matrix workspaces each.
    // Each matrix workspace has two spectra.

    // First group
    auto ws1 = createUniformWorkspace(0.1, 0.1, 1., 2.);
    auto ws2 = createUniformWorkspace(0.1, 0.1, 1.5, 2.5);
    WorkspaceGroup_sptr group1 = boost::make_shared<WorkspaceGroup>();
    group1->addWorkspace(ws1);
    group1->addWorkspace(ws2);
    // Second group
    auto ws3 = createUniformWorkspace(0.8, 0.1, 1.1, 2.1);
    auto ws4 = createUniformWorkspace(0.8, 0.1, 1.6, 2.6);
    WorkspaceGroup_sptr group2 = boost::make_shared<WorkspaceGroup>();
    group2->addWorkspace(ws3);
    group2->addWorkspace(ws4);

    // The algorithm needs the workspaces to be in the ADS
    AnalysisDataService::Instance().addOrReplace("group1", group1);
    AnalysisDataService::Instance().addOrReplace("group2", group2);

    // ws1 will be stitched with ws3
    // ws2 will be stitched with ws4

    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "group1, group2");
    alg.setProperty("Params", "0.1");
    alg.setProperty("StartOverlaps", "0.8");
    alg.setProperty("EndOverlaps", "1.1");
    alg.setPropertyValue("OutputWorkspace", "outws");
    alg.execute();

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(outws);
    TS_ASSERT_EQUALS(group->getNumberOfEntries(), 2);

    // First item in the output group
    auto stitched =
        boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
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
    stitched = boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(1));
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
    TS_ASSERT_DELTA(scales.front(), 0.9090, 0.0001); // 1.0/1.1
    TS_ASSERT_DELTA(scales.back(), 0.9375, 0.0001);  // 1.5/1.6

    // Clear the ADS
    AnalysisDataService::Instance().clear();
  }

  void test_two_groups_with_two_workspaces_single_scale_factor_given() {
    // Two groups with two matrix workspaces each.
    // Each matrix workspace has two spectra.

    // First group
    auto ws1 = createUniformWorkspace(0.1, 0.1, 1., 2.);
    auto ws2 = createUniformWorkspace(0.1, 0.1, 1.5, 2.5);
    WorkspaceGroup_sptr group1 = boost::make_shared<WorkspaceGroup>();
    group1->addWorkspace(ws1);
    group1->addWorkspace(ws2);
    // Second group
    auto ws3 = createUniformWorkspace(0.8, 0.1, 1.1, 2.1);
    auto ws4 = createUniformWorkspace(0.8, 0.1, 1.6, 2.6);
    WorkspaceGroup_sptr group2 = boost::make_shared<WorkspaceGroup>();
    group2->addWorkspace(ws3);
    group2->addWorkspace(ws4);

    // The algorithm needs the workspaces to be in the ADS
    AnalysisDataService::Instance().addOrReplace("group1", group1);
    AnalysisDataService::Instance().addOrReplace("group2", group2);

    // ws1 will be stitched with ws3
    // ws2 will be stitched with ws4

    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "group1, group2");
    alg.setProperty("Params", "0.1");
    alg.setProperty("StartOverlaps", "0.8");
    alg.setProperty("EndOverlaps", "1.1");
    alg.setProperty("UseManualScaleFactors", "1");
    alg.setProperty("ManualScaleFactors", "0.5");
    alg.setPropertyValue("OutputWorkspace", "outws");
    alg.execute();

    // The above is equivalent to what we've done in test_three_workspaces()
    // so we should get the same values in the output workspace
    // the only difference is that output will be a group

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(outws);
    TS_ASSERT_EQUALS(group->getNumberOfEntries(), 2);

    // First item in the output group
    auto stitched =
        boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
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
    stitched = boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(1));
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

    // Clear the ADS
    AnalysisDataService::Instance().clear();
  }

  void test_two_groups_with_three_workspaces_multiple_scale_factors_given() {
    // Three groups with two matrix workspaces each.
    // Each matrix workspace has two spectra.

    // First group
    auto ws1 = createUniformWorkspace(0.1, 0.1, 1., 2.);
    auto ws2 = createUniformWorkspace(0.1, 0.1, 1.5, 2.5);
    WorkspaceGroup_sptr group1 = boost::make_shared<WorkspaceGroup>();
    group1->addWorkspace(ws1);
    group1->addWorkspace(ws2);
    // Second group
    auto ws3 = createUniformWorkspace(0.8, 0.1, 1.1, 2.1);
    auto ws4 = createUniformWorkspace(0.8, 0.1, 1.6, 2.6);
    WorkspaceGroup_sptr group2 = boost::make_shared<WorkspaceGroup>();
    group2->addWorkspace(ws3);
    group2->addWorkspace(ws4);
    // Third group
    auto ws5 = createUniformWorkspace(1.6, 0.1, 1.5, 2.5);
    auto ws6 = createUniformWorkspace(1.6, 0.1, 1.6, 3.0);
    WorkspaceGroup_sptr group3 = boost::make_shared<WorkspaceGroup>();
    group3->addWorkspace(ws5);
    group3->addWorkspace(ws6);

    // The algorithm needs the workspaces to be in the ADS
    AnalysisDataService::Instance().addOrReplace("group1", group1);
    AnalysisDataService::Instance().addOrReplace("group2", group2);
    AnalysisDataService::Instance().addOrReplace("group3", group3);

    // ws1 will be stitched with ws3 and ws5
    // ws2 will be stitched with ws4 and ws6

    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "group1, group2, group3");
    alg.setProperty("Params", "0.1, 0.1, 2.6");
    alg.setProperty("StartOverlaps", "0.8, 1.6");
    alg.setProperty("EndOverlaps", "1.1, 1.9");
    alg.setProperty("UseManualScaleFactors", "1");
    alg.setProperty("ManualScaleFactors", "0.5, 0.7");
    alg.setPropertyValue("OutputWorkspace", "outws");
    alg.execute();

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(outws);
    TS_ASSERT_EQUALS(group->getNumberOfEntries(), 2);

    // First item in the output group
    auto stitched =
        boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));

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
    stitched = boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(1));

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

    // Clear the ADS
    AnalysisDataService::Instance().clear();
  }

  void test_two_groups_with_three_workspaces_scale_factor_from_period() {
    // Three groups with two matrix workspaces each.
    // Each matrix workspace has two spectra.

    // First group
    auto ws1 = createUniformWorkspace(0.1, 0.1, 1., 2.);
    auto ws2 = createUniformWorkspace(0.1, 0.1, 1.5, 2.5);
    WorkspaceGroup_sptr group1 = boost::make_shared<WorkspaceGroup>();
    group1->addWorkspace(ws1);
    group1->addWorkspace(ws2);
    // Second group
    auto ws3 = createUniformWorkspace(0.8, 0.1, 1.1, 2.1);
    auto ws4 = createUniformWorkspace(0.8, 0.1, 1.6, 2.6);
    WorkspaceGroup_sptr group2 = boost::make_shared<WorkspaceGroup>();
    group2->addWorkspace(ws3);
    group2->addWorkspace(ws4);
    // Third group
    auto ws5 = createUniformWorkspace(1.6, 0.1, 1.5, 2.5);
    auto ws6 = createUniformWorkspace(1.6, 0.1, 1.6, 3.0);
    WorkspaceGroup_sptr group3 = boost::make_shared<WorkspaceGroup>();
    group3->addWorkspace(ws5);
    group3->addWorkspace(ws6);

    // The algorithm needs the workspaces to be in the ADS
    AnalysisDataService::Instance().addOrReplace("group1", group1);
    AnalysisDataService::Instance().addOrReplace("group2", group2);
    AnalysisDataService::Instance().addOrReplace("group3", group3);

    // ws1 will be stitched with ws3 and ws5
    // ws2 will be stitched with ws4 and ws6

    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "group1, group2, group3");
    alg.setProperty("Params", "0.1, 0.1, 2.6");
    alg.setProperty("StartOverlaps", "0.8, 1.6");
    alg.setProperty("EndOverlaps", "1.1, 1.9");
    alg.setProperty("UseManualScaleFactors", "1");
    alg.setProperty("ScaleFactorFromPeriod", 2);
    alg.setPropertyValue("OutputWorkspace", "outws");
    alg.execute();

    // By keeping ManualScaleFactors empty (default value) it allows workspaces
    // in other periods to be scaled by scale factors from a specific period.
    // Periods 0 and 2 workspaces will be scaled by scale factors from period 1.

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(outws);
    TS_ASSERT_EQUALS(group->getNumberOfEntries(), 2);

    // First item in the output group
    auto stitched =
        boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
    TS_ASSERT_EQUALS(stitched->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(stitched->blocksize(), 25);
    // First spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(0)[0], 1, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[9], 1.01589, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[16], 0.97288, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[24], 0.9375, 0.00001);
    // Second spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(1)[0], 2, 0.00001);
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
    stitched = boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(1));

    TS_ASSERT_EQUALS(stitched->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(stitched->blocksize(), 25);
    // First spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(0)[0], 1.5, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[9], 1.5, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[16], 1.15385, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[24], 1, 0.00001);
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

    // Clear the ADS
    AnalysisDataService::Instance().clear();
  }

  void test_two_workspaces_history() {
    // This test is functionally similar to test_two_workspaces

    // Two matrix workspaces with two spectra each
    auto ws1 = createUniformWorkspace(0.1, 0.1, 1., 2., true, "ws1");
    auto ws2 = createUniformWorkspace(0.8, 0.1, 1.1, 2.1, true, "ws2");

    Stitch1DMany alg;
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2");
    alg.setProperty("Params", "0.1, 0.1, 1.8");
    alg.setProperty("StartOverlaps", "0.8");
    alg.setProperty("EndOverlaps", "1.1");
    alg.setPropertyValue("OutputWorkspace", "outws");
    alg.execute();

    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    auto stitched =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outws");

    // Test the algorithm histories
    std::vector<std::string> histNames;
    auto histories = stitched->history().getAlgorithmHistories();
    for (auto &hist : histories) {
      histNames.push_back(hist->name());
    }

    const std::string createWsName = "CreateWorkspace";
    const std::string s1dmName = "Stitch1DMany";

    TS_ASSERT_EQUALS(histNames[0], createWsName);
    TS_ASSERT_EQUALS(histNames[1], createWsName);
    TS_ASSERT_EQUALS(histNames[2], s1dmName);

    // Remove workspaces from ADS
    AnalysisDataService::Instance().clear();
  }

  void test_two_groups_history() {
    // This test is functionally similar to
    // test_two_groups_with_two_workspaces_each

    // Two groups with two matrix workspaces each.
    // Each matrix workspace has two spectra.

    // First group
    auto ws1 = createUniformWorkspace(0.1, 0.1, 1., 2., true, "ws1");
    auto ws2 = createUniformWorkspace(0.1, 0.1, 1.5, 2.5, true, "ws2");
    WorkspaceGroup_sptr group1 = doGroupWorkspaces("ws1, ws2", "group1");
    // Second group
    auto ws3 = createUniformWorkspace(0.8, 0.1, 1.1, 2.1, true, "ws3");
    auto ws4 = createUniformWorkspace(0.8, 0.1, 1.6, 2.6, true, "ws4");
    WorkspaceGroup_sptr group2 = doGroupWorkspaces("ws3, ws4", "group2");

    Stitch1DMany alg;
    alg.initialize();
    alg.setProperty("InputWorkspaces", "group1, group2");
    alg.setProperty("Params", "0.1");
    alg.setProperty("StartOverlaps", "0.8");
    alg.setProperty("EndOverlaps", "1.1");
    alg.setPropertyValue("OutputWorkspace", "outws");
    alg.execute();

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    auto group =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("outws");
    TS_ASSERT_EQUALS(group->getNumberOfEntries(), 2);

    // First item in the output group
    auto stitched =
        boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));

    // Test the algorithm histories
    std::vector<std::string> histNames;
    auto histories = stitched->history().getAlgorithmHistories();
    for (auto &hist : histories) {
      histNames.push_back(hist->name());
    }

    const std::string createWsName = "CreateWorkspace";
    const std::string groupWsName = "GroupWorkspaces";
    const std::string s1dmName = "Stitch1DMany";

    TS_ASSERT_EQUALS(histNames[0], createWsName);
    TS_ASSERT_EQUALS(histNames[1], groupWsName);
    TS_ASSERT_EQUALS(histNames[2], createWsName);
    TS_ASSERT_EQUALS(histNames[3], groupWsName);
    TS_ASSERT_EQUALS(histNames[4], s1dmName);

    // Remove workspaces from ADS
    AnalysisDataService::Instance().clear();
  }

  void test_two_groups_scale_factor_from_period_history() {
    // This test is functionally similar to
    // test_two_groups_with_three_workspaces_scale_factor_from_period

    // Three groups with two matrix workspaces each.
    // Each matrix workspace has two spectra.

    // First group
    auto ws1 = createUniformWorkspace(0.1, 0.1, 1., 2., true, "ws1");
    auto ws2 = createUniformWorkspace(0.1, 0.1, 1.5, 2.5, true, "ws2");
    WorkspaceGroup_sptr group1 = doGroupWorkspaces("ws1, ws2", "group1");
    // Second group
    auto ws3 = createUniformWorkspace(0.8, 0.1, 1.1, 2.1, true, "ws3");
    auto ws4 = createUniformWorkspace(0.8, 0.1, 1.6, 2.6, true, "ws4");
    WorkspaceGroup_sptr group2 = doGroupWorkspaces("ws3, ws4", "group2");
    // Third group
    auto ws5 = createUniformWorkspace(1.6, 0.1, 1.5, 2.5, true, "ws5");
    auto ws6 = createUniformWorkspace(1.6, 0.1, 1.6, 3.0, true, "ws6");
    WorkspaceGroup_sptr group3 = doGroupWorkspaces("ws5, ws6", "group3");

    Stitch1DMany alg;
    alg.initialize();
    alg.setProperty("InputWorkspaces", "group1, group2, group3");
    alg.setProperty("Params", "0.1, 0.1, 2.6");
    alg.setProperty("StartOverlaps", "0.8, 1.6");
    alg.setProperty("EndOverlaps", "1.1, 1.9");
    alg.setProperty("UseManualScaleFactors", "1");
    alg.setProperty("ScaleFactorFromPeriod", 2);
    alg.setPropertyValue("OutputWorkspace", "outws");
    alg.execute();

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    auto group =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("outws");
    auto stitched =
        boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));

    // Test the algorithm histories
    std::vector<std::string> histNames;
    auto histories = stitched->history().getAlgorithmHistories();
    for (auto &hist : histories) {
      histNames.push_back(hist->name());
    }

    const std::string createWsName = "CreateWorkspace";
    const std::string groupWsName = "GroupWorkspaces";

    TS_ASSERT_EQUALS(histNames[0], createWsName);
    TS_ASSERT_EQUALS(histNames[1], groupWsName);
    TS_ASSERT_EQUALS(histNames[2], createWsName);
    TS_ASSERT_EQUALS(histNames[3], groupWsName);
    TS_ASSERT_EQUALS(histNames[4], createWsName);
    TS_ASSERT_EQUALS(histNames[5], groupWsName);

    // Clear the ADS
    AnalysisDataService::Instance().clear();
  }
};

#endif /* MANTID_ALGORITHMS_STITCH1DMANYTEST_H_ */
