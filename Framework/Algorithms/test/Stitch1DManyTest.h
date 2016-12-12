#ifndef MANTID_ALGORITHMS_STITCH1DMANYTEST_H_
#define MANTID_ALGORITHMS_STITCH1DMANYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/Stitch1D.h"
#include "MantidAlgorithms/Stitch1DMany.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/UnitFactory.h"
#include <math.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::Algorithms::Stitch1DMany;

class Stitch1DManyTest : public CxxTest::TestSuite {
private:
  /** Create a histogram workspace with two spectra and 10 bins
  * @param xstart :: the first X value (common to both spectra)
  * @param deltax :: the bin width
  * @param value1 :: the Y counts in the first spectrum (constant for all X)
  * @param value2 :: the Y counts in the second spectrum (constant for all X)
  */
  MatrixWorkspace_sptr createUniformWorkspace(double xstart, double deltax,
                                              double value1, double value2) {

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

    MatrixWorkspace_sptr ws =
        WorkspaceFactory::Instance().create("Workspace2D", 2, nbins + 1, nbins);
    ws->mutableX(0) = xData1;
    ws->mutableX(1) = xData2;
    ws->mutableY(0) = yData1;
    ws->mutableY(1) = yData2;
    ws->mutableE(0) = eData1;
    ws->mutableE(1) = eData2;
    ws->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");

    return ws;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static Stitch1DManyTest *createSuite() { return new Stitch1DManyTest(); }
  static void destroySuite(Stitch1DManyTest *suite) { delete suite; }

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
    alg.setProperty("StartOverlaps", "0.8");
    alg.setProperty("EndOverlaps", "1.1");
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

  void test_throws_if_no_params() {
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2");
    alg.setProperty("StartOverlaps", "-0.5, -0.6");
    alg.setProperty("EndOverlaps", "0.5, 0.6");
    alg.setPropertyValue("OutputWorkspace", "outws");
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
  }

  void test_workspace_types_differ_throws() {
    // One table workspace, one matrix workspace
    auto ws1 = createUniformWorkspace(0.1, 0.1, 1., 2.);
    auto ws2 = WorkspaceFactory::Instance().createTable();
    AnalysisDataService::Instance().add("ws1", ws1);
    AnalysisDataService::Instance().add("ws2", ws2);
    Stitch1DMany alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", "ws1, ws2");
    alg.setProperty("Params", "0.1");
    alg.setProperty("StartOverlaps", "-0.5, -0.6");
    alg.setProperty("EndOverlaps", "0.5, 0.6");
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
    alg.setProperty("StartOverlaps", "0.8, 1.6");
    alg.setProperty("EndOverlaps", "1.1, 1.8");
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

    // Test out sclae factors
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
    TS_ASSERT_EQUALS(stitched->readX(0), stitched2->readX(0));
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

    // Test out sclae factors
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

  void test_three_workspaces_scale_factor_given() {
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
    alg.setProperty("UseManualScaleFactor", "1");
    alg.setProperty("ManualScaleFactor", 0.5);
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

    // Test out sclae factors
    std::vector<double> scales = alg.getProperty("OutScaleFactors");
    TS_ASSERT_EQUALS(scales.size(), 2);
    TS_ASSERT_EQUALS(scales.front(), 0.5);
    TS_ASSERT_EQUALS(scales.back(), 0.5);

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
    auto results = alg.validateInputs();
    TS_ASSERT_EQUALS(results["InputWorkspaces"],
                     "At least 2 input workspaces required.")

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

    // Test out sclae factors
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

    // Test out sclae factors
    std::vector<double> scales = alg.getProperty("OutScaleFactors");
    TS_ASSERT_EQUALS(scales.size(), 2);
    TS_ASSERT_DELTA(scales.front(), 0.9090, 0.0001); // 1.0/1.1
    TS_ASSERT_DELTA(scales.back(), 0.9375, 0.0001);  // 1.5/1.6

    // Clear the ADS
    AnalysisDataService::Instance().clear();
  }

  void test_two_groups_with_two_workspaces_scale_factor_given() {
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
    alg.setProperty("UseManualScaleFactor", "1");
    alg.setProperty("ManualScaleFactor", 0.5);
    alg.setPropertyValue("OutputWorkspace", "outws");
    alg.execute();

    // The above is equivalent to what we've don in test_three_workspaces()
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

    // Test out sclae factors
    std::vector<double> scales = alg.getProperty("OutScaleFactors");
    TS_ASSERT_EQUALS(scales.size(), 2);
    TS_ASSERT_DELTA(scales.front(), 0.5000, 0.0001);
    TS_ASSERT_DELTA(scales.back(), 0.5000, 0.0001);

    // Clear the ADS
    AnalysisDataService::Instance().clear();
  }

  void test_three_workspaces_scale_LHS_workspace() {
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
    alg.setProperty("Params", "0.1");
    alg.setPropertyValue("ScaleRHSWorkspace", "0");
    alg.setPropertyValue("OutputWorkspace", "outws");
    alg.execute();

    // Test output ws
    Workspace_sptr outws = alg.getProperty("OutputWorkspace");
    auto stitched = boost::dynamic_pointer_cast<MatrixWorkspace>(outws);
    TS_ASSERT_EQUALS(stitched->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(stitched->blocksize(), 25);
    // First spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(0)[0], 1.5, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[10], 1.5, 0.00001);
    TS_ASSERT_DELTA(stitched->y(0)[18], 1.5, 0.00001);
    // Second spectrum, Y values
    TS_ASSERT_DELTA(stitched->y(1)[0], 2.5, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[10], 2.5, 0.00001);
    TS_ASSERT_DELTA(stitched->y(1)[18], 2.5, 0.00001);

    // Test out sclae factors
    std::vector<double> scales = alg.getProperty("OutScaleFactors");
    TS_ASSERT_EQUALS(scales.size(), 2);
    TS_ASSERT_DELTA(scales.front(), 1.0999, 0.0001);
    TS_ASSERT_DELTA(scales.back(), 1.3636, 0.0001);

    // Remove workspaces from ADS
    AnalysisDataService::Instance().remove("ws1");
    AnalysisDataService::Instance().remove("ws2");
    AnalysisDataService::Instance().remove("ws3");
  }
};

#endif /* MANTID_ALGORITHMS_STITCH1DMANYTEST_H_ */
