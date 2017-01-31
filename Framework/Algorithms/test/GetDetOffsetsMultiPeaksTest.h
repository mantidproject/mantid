#ifndef GetDetOffsetsMultiPeaksTEST_H_
#define GetDetOffsetsMultiPeaksTEST_H_

#include "MantidAlgorithms/GetDetOffsetsMultiPeaks.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::Algorithms::GetDetOffsetsMultiPeaks;
using Mantid::DataObjects::OffsetsWorkspace_sptr;

class GetDetOffsetsMultiPeaksTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GetDetOffsetsMultiPeaksTest *createSuite() {
    return new GetDetOffsetsMultiPeaksTest();
  }
  static void destroySuite(GetDetOffsetsMultiPeaksTest *suite) { delete suite; }

  GetDetOffsetsMultiPeaksTest() { Mantid::API::FrameworkManager::Instance(); }

  void testTheBasics() {
    TS_ASSERT_EQUALS(offsets.name(), "GetDetOffsetsMultiPeaks");
    TS_ASSERT_EQUALS(offsets.version(), 1);
  }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(offsets.initialize());
    TS_ASSERT(offsets.isInitialized());
  }

  void testExec() {
    // ---- Create the simple workspace -------
    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 200);
    AnalysisDataService::Instance().addOrReplace("temp_event_ws", WS);
    WS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("dSpacing");

    auto xvals = WS->points(0);
    std::transform(xvals.cbegin(), xvals.cend(), WS->mutableY(0).begin(),
                   [](const double x) {
                     return 5.1 * exp(-0.5 * pow((x - 10) / 1.0, 2));
                   });

    auto &E = WS->mutableE(0);
    E.assign(E.size(), 0.001);

    // ---- Run algo -----
    if (!offsets.isInitialized())
      offsets.initialize();
    TS_ASSERT_THROWS_NOTHING(
        offsets.setProperty("InputWorkspace", "temp_event_ws"));
    std::string outputWS("offsetsped");
    std::string maskWS("masksped");
    TS_ASSERT_THROWS_NOTHING(
        offsets.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", maskWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DReference", "9.98040"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue(
        "SpectraFitInfoTableWorkspace", "FitInfoTable"));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWS));
    if (!output)
      return;

    TS_ASSERT_DELTA(output->y(0)[0], -0.002, 0.0002);

    AnalysisDataService::Instance().remove(outputWS);

    MatrixWorkspace_const_sptr mask;
    TS_ASSERT_THROWS_NOTHING(
        mask = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            maskWS));
    if (!mask)
      return;
    TS_ASSERT(!mask->detectorInfo().isMasked(0));
  }

  //----------------------------------------------------------------------------------------------
  /** Test the feature to ... ...
    */
  void testExecWithGroup() {
    // --------- Workspace with summed spectra -------
    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::createGroupedWorkspace2D(3, 200, 1.0);
    WS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("dSpacing");

    auto xvals = WS->points(0);
    std::transform(
        xvals.cbegin(), xvals.cend(), WS->mutableY(0).begin(),
        [](const double x) { return exp(-0.5 * pow((x - 10) / 1.0, 2)); });

    auto &E = WS->mutableE(0);
    E.assign(E.size(), 0.001);

    AnalysisDataService::Instance().addOrReplace("temp_event_ws3", WS);

    // ---- Run algo -----
    GetDetOffsetsMultiPeaks offsets;
    offsets.initialize();
    TS_ASSERT_THROWS_NOTHING(
        offsets.setProperty("InputWorkspace", "temp_event_ws3"));
    std::string outputWS("offsetsped");
    std::string maskWS("masksped");
    TS_ASSERT_THROWS_NOTHING(
        offsets.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", maskWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DReference", "9.98040"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue(
        "SpectraFitInfoTableWorkspace", "FitInfoTable"));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    OffsetsWorkspace_sptr output = offsets.getProperty("OutputWorkspace");
    if (!output)
      return;

    TS_ASSERT_DELTA(output->getValue(1), -0.00196, 0.0002);
    TS_ASSERT_EQUALS(output->getValue(1), output->getValue(2));
    TS_ASSERT_EQUALS(output->getValue(1), output->getValue(3));

    AnalysisDataService::Instance().remove(outputWS);

    MatrixWorkspace_const_sptr mask;
    TS_ASSERT_THROWS_NOTHING(
        mask = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            maskWS));
    if (!mask)
      return;
    TS_ASSERT(!mask->detectorInfo().isMasked(0));
  }

  //----------------------------------------------------------------------------------------------
  /** Test the feature to import fit windows for each spectrum from table
   * workspace
    */
  void testExecFitWindowTable() {
    // ---- (Re-)Create the simple workspace -------
    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 200);
    AnalysisDataService::Instance().addOrReplace("temp_event_ws", WS);
    WS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("dSpacing");

    auto xvals = WS->points(0);
    std::transform(xvals.cbegin(), xvals.cend(), WS->mutableY(0).begin(),
                   [](const double x) {
                     return 5.1 * exp(-0.5 * pow((x - 10) / 1.0, 2));
                   });

    auto &E = WS->mutableE(0);
    E.assign(E.size(), 0.001);

    // Create table workspace
    TableWorkspace_sptr fitWindowWS = boost::make_shared<TableWorkspace>();
    fitWindowWS->addColumn("int", "spectrum");
    fitWindowWS->addColumn("double", "peak0_left");
    fitWindowWS->addColumn("double", "peak0_right");

    TableRow newrow = fitWindowWS->appendRow();
    newrow << 0 << 9.9 << 11.0;

    AnalysisDataService::Instance().addOrReplace("PeakFitRangeTableWS",
                                                 fitWindowWS);

    // ---- Run algo -----
    offsets.initialize();
    TS_ASSERT_THROWS_NOTHING(
        offsets.setProperty("InputWorkspace", "temp_event_ws"));
    TS_ASSERT_THROWS_NOTHING(
        offsets.setProperty("FitwindowTableWorkspace", fitWindowWS));
    std::string outputWS("offsetsped");
    std::string maskWS("masksped");
    TS_ASSERT_THROWS_NOTHING(
        offsets.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", maskWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaxOffset", "3.0"));
    TS_ASSERT_THROWS_NOTHING(
        offsets.setPropertyValue("DReference", "30.98040"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue(
        "SpectraFitInfoTableWorkspace", "FitInfoTable"));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWS));
    if (!output)
      return;

    AnalysisDataService::Instance().remove(outputWS);
    AnalysisDataService::Instance().remove("PeakFitRangeTableWS");

    MatrixWorkspace_const_sptr mask;
    TS_ASSERT_THROWS_NOTHING(
        mask = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            maskWS));
    if (!mask)
      return;
    TS_ASSERT(!mask->detectorInfo().isMasked(0));
  }

  //----------------------------------------------------------------------------------------------
  /** Test the feature to import fit windows with univeral spectrum from table
   * workspace
    */
  void testExecFitWindowTableUniversal() {
    // ---- (Re-)Create the simple workspace -------
    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 200);
    AnalysisDataService::Instance().addOrReplace("temp_event_ws", WS);
    WS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("dSpacing");

    auto xvals = WS->points(0);
    std::transform(xvals.cbegin(), xvals.cend(), WS->mutableY(0).begin(),
                   [](const double x) {
                     return 5.1 * exp(-0.5 * pow((x - 10) / 1.0, 2));
                   });

    auto &E = WS->mutableE(0);
    E.assign(E.size(), 0.001);

    // Create table workspace
    TableWorkspace_sptr fitWindowWS = boost::make_shared<TableWorkspace>();
    fitWindowWS->addColumn("int", "spectrum");
    fitWindowWS->addColumn("double", "peak0_left");
    fitWindowWS->addColumn("double", "peak0_right");

    TableRow newrow = fitWindowWS->appendRow();
    newrow << -1 << 9.9 << 11.0;

    AnalysisDataService::Instance().addOrReplace("PeakFitRangeTableWS",
                                                 fitWindowWS);

    // ---- Run algo -----
    offsets.initialize();
    TS_ASSERT_THROWS_NOTHING(
        offsets.setProperty("InputWorkspace", "temp_event_ws"));
    TS_ASSERT_THROWS_NOTHING(
        offsets.setProperty("FitwindowTableWorkspace", fitWindowWS));
    std::string outputWS("offsetsped");
    std::string maskWS("masksped");
    TS_ASSERT_THROWS_NOTHING(
        offsets.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", maskWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaxOffset", "3.0"));
    TS_ASSERT_THROWS_NOTHING(
        offsets.setPropertyValue("DReference", "30.98040"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue(
        "SpectraFitInfoTableWorkspace", "FitInfoTable"));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWS));
    if (!output)
      return;

    AnalysisDataService::Instance().remove(outputWS);
    AnalysisDataService::Instance().remove("PeakFitRangeTableWS");

    MatrixWorkspace_const_sptr mask;
    TS_ASSERT_THROWS_NOTHING(
        mask = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            maskWS));
    if (!mask)
      return;
    TS_ASSERT(!mask->detectorInfo().isMasked(0));
  }

  //----------------------------------------------------------------------------------------------
  /** Test using the resolution workspace as input
   */
  void testExecInputResolutionWS() {
    // ---- Create the simple workspace -------
    // Data workspace
    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 200);
    AnalysisDataService::Instance().addOrReplace("temp_event_ws", WS);
    WS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("dSpacing");

    auto xvals = WS->points(0);
    std::transform(xvals.cbegin(), xvals.cend(), WS->mutableY(0).begin(),
                   [](const double x) {
                     return 5.1 * exp(-0.5 * pow((x - 10) / 1.0, 2));
                   });

    auto &E = WS->mutableE(0);
    E.assign(E.size(), 0.001);

    // Resolution workspace
    MatrixWorkspace_sptr resWS =
        WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
    resWS->mutableY(0)[0] = 0.2;
    AnalysisDataService::Instance().addOrReplace("temp_res_ws", resWS);

    // ---- Run algo -----
    GetDetOffsetsMultiPeaks offsets;
    offsets.initialize();
    TS_ASSERT_THROWS_NOTHING(
        offsets.setProperty("InputWorkspace", "temp_event_ws"));
    TS_ASSERT_THROWS_NOTHING(
        offsets.setProperty("InputResolutionWorkspace", "temp_res_ws"));
    TS_ASSERT_THROWS_NOTHING(
        offsets.setProperty("MinimumResolutionFactor", 0.8));
    TS_ASSERT_THROWS_NOTHING(
        offsets.setProperty("MaximumResolutionFactor", 1.2));

    std::string outputWS("offsetsped");
    std::string maskWS("masksped");
    TS_ASSERT_THROWS_NOTHING(
        offsets.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", maskWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DReference", "9.98040"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue(
        "SpectraFitInfoTableWorkspace", "FitInfoTable"));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWS));
    if (!output)
      return;

    TS_ASSERT_DELTA(output->y(0)[0], -0.002, 0.0002);

    AnalysisDataService::Instance().remove(outputWS);

    MatrixWorkspace_const_sptr mask;
    TS_ASSERT_THROWS_NOTHING(
        mask = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            maskWS));
    if (!mask)
      return;
    TS_ASSERT(!mask->detectorInfo().isMasked(0));
  }

  //----------------------------------------------------------------------------------------------
  /** Test using the resolution workspace as input with a failure case
   *  in which the data is noisy and not valid peak can be found
   */
  void testFailInputResolutionWS() {
    // ---- Create the simple workspace -------
    // Data workspace
    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 200);
    AnalysisDataService::Instance().addOrReplace("temp_event_ws", WS);
    WS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("dSpacing");
    generateNoisyData(WS);

    // Resolution workspace
    MatrixWorkspace_sptr resWS =
        WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
    resWS->mutableY(0)[0] = 0.2;
    AnalysisDataService::Instance().addOrReplace("temp_res_ws", resWS);

    // ---- Run algo -----
    GetDetOffsetsMultiPeaks offsets;
    offsets.initialize();
    TS_ASSERT_THROWS_NOTHING(
        offsets.setProperty("InputWorkspace", "temp_event_ws"));
    TS_ASSERT_THROWS_NOTHING(
        offsets.setProperty("InputResolutionWorkspace", "temp_res_ws"));
    TS_ASSERT_THROWS_NOTHING(
        offsets.setProperty("MinimumResolutionFactor", 0.8));
    TS_ASSERT_THROWS_NOTHING(
        offsets.setProperty("MaximumResolutionFactor", 1.2));

    std::string outputWS("offsetsped");
    std::string maskWS("masksped");
    TS_ASSERT_THROWS_NOTHING(
        offsets.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", maskWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DReference", "9.98040"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue(
        "SpectraFitInfoTableWorkspace", "FitInfoTable"));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWS));
    if (!output)
      return;

    TS_ASSERT_DELTA(output->y(0)[0], 0.0, 1.0E-20);

    AnalysisDataService::Instance().remove(outputWS);

    MatrixWorkspace_const_sptr mask;
    TS_ASSERT_THROWS_NOTHING(
        mask = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            maskWS));
    TS_ASSERT(mask->detectorInfo().isMasked(0));

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate noisy data in a workspace
   */
  void generateNoisyData(MatrixWorkspace_sptr WS) {

    auto &Y = WS->mutableY(0);
    Y.assign(Y.size(), static_cast<double>(rand() % 5));
    auto &E = WS->mutableE(0);
    E.assign(E.size(), 0.01);
  }

private:
  GetDetOffsetsMultiPeaks offsets;
};

class GetDetOffsetsMultiPeaksTestPerformance : public CxxTest::TestSuite {
  MatrixWorkspace_sptr WS;
  int numpixels;

public:
  static GetDetOffsetsMultiPeaksTestPerformance *createSuite() {
    return new GetDetOffsetsMultiPeaksTestPerformance();
  }
  static void destroySuite(GetDetOffsetsMultiPeaksTestPerformance *suite) {
    delete suite;
  }

  GetDetOffsetsMultiPeaksTestPerformance() { FrameworkManager::Instance(); }

  void setUp() override {
    numpixels = 10000;
    WS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        numpixels, 200, false);
    WS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("dSpacing");
    for (size_t wi = 0; wi < WS->getNumberHistograms(); wi++) {

      auto xvals = WS->points(0);
      std::transform(
          xvals.cbegin(), xvals.cend(), WS->mutableY(0).begin(),
          [](const double x) { return exp(-0.5 * pow((x - 10) / 1.0, 2)); });

      auto &E = WS->mutableE(0);
      E.assign(E.size(), 0.001);
    }
    AnalysisDataService::Instance().addOrReplace("temp_event_ws", WS);
  }

  void test_performance() {
    AlgorithmManager::Instance(); // Initialize here to avoid an odd ABORT
    GetDetOffsetsMultiPeaks offsets;
    if (!offsets.isInitialized())
      offsets.initialize();
    TS_ASSERT_THROWS_NOTHING(
        offsets.setProperty("InputWorkspace", "temp_event_ws"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DReference", "9.98040"));
    TS_ASSERT_THROWS_NOTHING(
        offsets.setPropertyValue("OutputWorkspace", "dummyname"));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());
    OffsetsWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = offsets.getProperty("OutputWorkspace"));
    if (!output)
      return;
    TS_ASSERT_DELTA(output->mutableY(0)[0], -0.00196, 0.0002);
  }
};

#endif /*GetDetOffsetsMultiPeaksTEST_H_*/
