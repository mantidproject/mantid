#ifndef MANTID_ALGORITHMS_CREATESAMPLEWORKSPACETEST_H_
#define MANTID_ALGORITHMS_CREATESAMPLEWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/Unit.h"

using Mantid::Algorithms::CreateSampleWorkspace;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

class CreateSampleWorkspaceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateSampleWorkspaceTest *createSuite() {
    return new CreateSampleWorkspaceTest();
  }
  static void destroySuite(CreateSampleWorkspaceTest *suite) { delete suite; }

  CreateSampleWorkspaceTest() { FrameworkManager::Instance(); }

  void test_Init() {
    CreateSampleWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  MatrixWorkspace_sptr createSampleWorkspace(
      std::string outWSName, std::string wsType = "", std::string function = "",
      std::string userFunction = "", int numBanks = 2, int bankPixelWidth = 10,
      int numEvents = 1000, bool isRandom = false, std::string xUnit = "TOF",
      double xMin = 0.0, double xMax = 20000.0, double binWidth = 200.0,
      int numScanPoints = 1) {

    CreateSampleWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setPropertyValue("OutputWorkspace", outWSName);
    if (!wsType.empty())
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("WorkspaceType", wsType));
    if (!function.empty())
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Function", function));
    if (!userFunction.empty())
      TS_ASSERT_THROWS_NOTHING(
          alg.setPropertyValue("UserDefinedFunction", userFunction));
    if (numBanks != 2)
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumBanks", numBanks));
    if (bankPixelWidth != 10)
      TS_ASSERT_THROWS_NOTHING(
          alg.setProperty("BankPixelWidth", bankPixelWidth));
    if (numEvents != 1000)
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumEvents", numEvents));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Random", isRandom));
    if (xUnit != "TOF")
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("XUnit", xUnit));
    if (xMin != 0.0)
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMin", xMin));
    if (xMax != 20000.0)
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMax", xMax));
    if (binWidth != 200.0)
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("BinWidth", binWidth));
    if (numScanPoints != 1)
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumScanPoints", numScanPoints))

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outWSName));
    TS_ASSERT(ws);

    // check the basics
    int numBins = static_cast<int>((xMax - xMin) / binWidth);
    int numHist = numBanks * bankPixelWidth * bankPixelWidth;
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), numHist * numScanPoints);
    TS_ASSERT_EQUALS(ws->blocksize(), numBins);

    TS_ASSERT_EQUALS(ws->getAxis(0)->unit()->unitID(), xUnit);
    TS_ASSERT_EQUALS(ws->readX(0)[0], xMin);
    if (ws->blocksize() == static_cast<size_t>(numBins)) {
      TS_ASSERT_DELTA(ws->readX(0)[numBins], xMax, binWidth);
    }

    if (wsType == "Event") {
      EventWorkspace_sptr ews = boost::dynamic_pointer_cast<EventWorkspace>(ws);
      TS_ASSERT(ews);
    }

    return ws;
  }

  void test_default_pixel_spacing() {
    CreateSampleWorkspace alg;
    alg.setChild(true);
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    const auto instrument = outWS->getInstrument();
    auto bank1 = boost::dynamic_pointer_cast<const RectangularDetector>(
        instrument->getComponentByName("bank1"));
    TSM_ASSERT_EQUALS(
        "PixelSpacing on the bank is not the same as the expected default in x",
        0.008, bank1->xstep());
    TSM_ASSERT_EQUALS(
        "PixelSpacing on the bank is not the same as the expected default in y",
        0.008, bank1->ystep());
  }

  void test_apply_pixel_spacing() {
    // Set the spacing we want
    const double pixelSpacing = 0.01;

    CreateSampleWorkspace alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("PixelSpacing", pixelSpacing);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    const auto instrument = outWS->getInstrument();
    auto bank1 = boost::dynamic_pointer_cast<const RectangularDetector>(
        instrument->getComponentByName("bank1"));
    TSM_ASSERT_EQUALS("Not the specified pixel spacing in x", pixelSpacing,
                      bank1->xstep());
    TSM_ASSERT_EQUALS("Not the specified pixel spacing in y", pixelSpacing,
                      bank1->ystep());
  }

  void test_default_instrument_bank_setup() {
    CreateSampleWorkspace alg;
    alg.setChild(true);
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    const auto instrument = outWS->getInstrument();
    const auto bank1 = instrument->getComponentByName("bank1");
    const auto bank2 = instrument->getComponentByName("bank2");
    const auto sample = instrument->getComponentByName("sample");

    const V3D bank1ToSample = bank1->getPos() - sample->getPos();
    const V3D bank2ToSample = bank2->getPos() - sample->getPos();
    auto refFrame = instrument->getReferenceFrame();
    TSM_ASSERT_EQUALS(
        "By default bank1 should be offset from the sample in the beam "
        "direction by 5m",
        5.0, refFrame->vecPointingAlongBeam().scalar_prod(bank1ToSample));
    TSM_ASSERT_EQUALS(
        "By default bank2 should be offset from the sample in the beam "
        "direction by 2 * 5m",
        10.0, refFrame->vecPointingAlongBeam().scalar_prod(bank2ToSample));
  }

  void test_apply_offset_instrument_banks() {
    // Set the offset we want
    const double bankToSampleDistance = 4.0;

    CreateSampleWorkspace alg;
    alg.setChild(true);
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.setProperty("BankDistanceFromSample", bankToSampleDistance);
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    const auto instrument = outWS->getInstrument();
    const auto bank1 = instrument->getComponentByName("bank1");
    const auto bank2 = instrument->getComponentByName("bank2");
    const auto sample = instrument->getComponentByName("sample");

    const V3D bank1ToSample = bank1->getPos() - sample->getPos();
    const V3D bank2ToSample = bank2->getPos() - sample->getPos();
    auto refFrame = instrument->getReferenceFrame();
    TSM_ASSERT_EQUALS(
        "Wrong offset applied for bank1", bankToSampleDistance,
        refFrame->vecPointingAlongBeam().scalar_prod(bank1ToSample));
    TSM_ASSERT_EQUALS(
        "Wrong offset applied for bank2", bankToSampleDistance * 2,
        refFrame->vecPointingAlongBeam().scalar_prod(bank2ToSample));
  }

  void test_histogram_defaults() {
    // Name of the output workspace.
    std::string outWSName("CreateSampleWorkspaceTest_OutputWS");

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    MatrixWorkspace_sptr ws = createSampleWorkspace(outWSName);
    if (!ws)
      return;
    TS_ASSERT_DELTA(ws->readY(0)[20], 0.3, 0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[40], 0.3, 0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[50], 10.3, 0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[60], 0.3, 0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[80], 0.3, 0.0001);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_event_defaults() {
    // Name of the output workspace.
    std::string outWSName("CreateSampleWorkspaceTest_OutputWS_event");

    auto ws = boost::dynamic_pointer_cast<IEventWorkspace>(
        createSampleWorkspace(outWSName, "Event"));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // The number of events per spectrum is not simply nhist*events_per_hist
    // The algorithm computes the number of events required per bin to give the
    // total per spectrum as requested but can only add integer numbers of
    // events
    // so the number added will always be less than expected.
    // In this case the background has 99 bins at 0.3 & 1 bin at 10.3 => total
    // sum=40
    //    signal_scale_factor = events_per_spec/40 = 1000/40=25  =>
    //      background Y = 0.3*25 = 7.5 => int(7.5)=7
    //      peak Y = 10.3*25 = 257.5 => int(257.5)=257
    // Therefore in total we lose 0.5*100*200=10000 events
    TS_ASSERT_EQUALS(ws->getNumberEvents(), 190000);
    TS_ASSERT_DELTA(ws->readY(0)[20], 7, 0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[40], 7, 0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[50], 257, 0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[60], 7, 0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[80], 7, 0.0001);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_event_MoreBanksMoreDetectorsLessEvents() {
    // Name of the output workspace.
    std::string outWSName(
        "CreateSampleWorkspaceTest_OutputWS_MoreBanksMoreDetectors");

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    MatrixWorkspace_sptr ws =
        createSampleWorkspace(outWSName, "Event", "", "", 4, 30, 100);
    if (!ws)
      return;

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_histo_Multiple_Peaks() {
    // Name of the output workspace.
    std::string outWSName("CreateSampleWorkspaceTest_OutputWS_Multiple_Peaks");

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    MatrixWorkspace_sptr ws =
        createSampleWorkspace(outWSName, "Histogram", "Multiple Peaks");
    if (!ws)
      return;
    TS_ASSERT_DELTA(ws->readY(0)[20], 0.3, 0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[40], 0.3, 0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[60], 8.3, 0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[80], 0.3, 0.0001);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_event_Flat_background() {
    // Name of the output workspace.
    std::string outWSName("CreateSampleWorkspaceTest_OutputWS_Flat_background");

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    MatrixWorkspace_sptr ws =
        createSampleWorkspace(outWSName, "Event", "Flat background");
    if (!ws)
      return;
    TS_ASSERT_DELTA(ws->readY(0)[20], 10.0, 0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[40], 10.0, 0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[60], 10.0, 0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[80], 10.0, 0.0001);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_event_Exp_Decay() {
    // Name of the output workspace.
    std::string outWSName("CreateSampleWorkspaceTest_OutputWS_Exp_Decay");

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    MatrixWorkspace_sptr ws =
        createSampleWorkspace(outWSName, "Event", "Exp Decay");
    if (!ws)
      return;
    TS_ASSERT_DELTA(ws->readY(0)[20], 3.0, 0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[40], 0.0, 0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[60], 0.0, 0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[80], 0.0, 0.0001);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_event_User_Defined() {
    // Name of the output workspace.
    std::string outWSName("CreateSampleWorkspaceTest_OutputWS_User_Defined");
    std::string myFunc = "name=LinearBackground, A0=0.5;name=Gaussian, "
                         "PeakCentre=10000, Height=50, "
                         "Sigma=0.5;name=Gaussian, PeakCentre=1000, Height=80, "
                         "Sigma=0.5";
    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    MatrixWorkspace_sptr ws =
        createSampleWorkspace(outWSName, "Histogram", "User Defined", myFunc);
    if (!ws)
      return;
    TS_ASSERT_DELTA(ws->readY(0)[5], 80.5, 0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[20], 0.5, 0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[50], 50.5, 0.0001);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_histogram_random() {
    // Name of the output workspace.
    std::string outWSName("CreateSampleWorkspaceTest_Hist_random");

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    MatrixWorkspace_sptr ws = createSampleWorkspace(outWSName);
    if (!ws)
      return;
    TS_ASSERT_DELTA(ws->readY(0)[20], 0.3, 0.5);
    TS_ASSERT_DELTA(ws->readY(0)[40], 0.3, 0.5);
    TS_ASSERT_DELTA(ws->readY(0)[50], 10.3, 0.5);
    TS_ASSERT_DELTA(ws->readY(0)[60], 0.3, 0.5);
    TS_ASSERT_DELTA(ws->readY(0)[80], 0.3, 0.5);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_event_random() {
    // Name of the output workspace.
    std::string outWSName("CreateSampleWorkspaceTest_event_random");

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    MatrixWorkspace_sptr ws = createSampleWorkspace(outWSName, "Event");
    if (!ws)
      return;
    TS_ASSERT_DELTA(ws->readY(0)[20], 7, 50);
    TS_ASSERT_DELTA(ws->readY(0)[40], 7, 50);
    TS_ASSERT_DELTA(ws->readY(0)[50], 257, 50);
    TS_ASSERT_DELTA(ws->readY(0)[60], 7, 50);
    TS_ASSERT_DELTA(ws->readY(0)[80], 7, 50);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_units() {
    // Name of the output workspace.
    std::string outWSName("CreateSampleWorkspaceTest_units");

    /* Equivalent of this python command:
      ws=CreateSampleWorkspace(WorkspaceType="Event",Function="One Peak",
      NumBanks=1,BankPixelWidth=2,NumEvents=50,Random=True,
      XUnit="dSpacing",XMin=0, XMax=8, BinWidth=0.1)
    */
    MatrixWorkspace_sptr ws =
        createSampleWorkspace(outWSName, "Event", "One Peak", "", 1, 2, 50,
                              true, "dSpacing", 0, 8, 0.1);
    if (!ws)
      return;
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);

    ws = createSampleWorkspace(outWSName, "Event", "One Peak", "", 1, 2, 50,
                               true, "Wavelength", 0, 8, 0.1);
    if (!ws)
      return;
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);

    ws = createSampleWorkspace(outWSName, "Event", "One Peak", "", 1, 2, 50,
                               true, "Energy", 100, 1000, 10);
    if (!ws)
      return;
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);

    ws = createSampleWorkspace(outWSName, "Event", "One Peak", "", 1, 2, 50,
                               true, "QSquared", 0, 800, 10);
    if (!ws)
      return;
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_failure_due_to_bad_bin_width() {
    /* Equivalent of this python command:
      mono_ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4,
      NumEvents=10000,XUnit='DeltaE',XMin=-5,XMax=15)
    */
    std::string outWSName =
        "CreateSampleWorkspaceTest_test_failure_due_to_bad_bin_width";
    CreateSampleWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setPropertyValue("OutputWorkspace", outWSName);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumBanks", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BankPixelWidth", 4));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumEvents", 10000));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("XUnit", "DeltaE"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMin", -5.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMax", 15.0));
    // leave the default bin width of 200, which is inappropriate

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outWSName));
    TS_ASSERT(ws);
    // just one bin
    TS_ASSERT_EQUALS(ws->blocksize(), 1);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_histogram_monitors() {
    // Name of the output workspace.
    CreateSampleWorkspace alg;
    alg.setChild(true);
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.setProperty("NumMonitors", 2);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    TS_ASSERT(outWS->spectrumInfo().isMonitor(0));
    TS_ASSERT_DELTA(outWS->readY(0)[40], 0.3, 0.0001);
    TS_ASSERT_DELTA(outWS->readY(0)[50], 10.3, 0.0001);

    TS_ASSERT(outWS->spectrumInfo().isMonitor(1));
    TS_ASSERT_DELTA(outWS->readY(1)[40], 0.3, 0.0001);
    TS_ASSERT_DELTA(outWS->readY(1)[50], 10.3, 0.0001);

    TS_ASSERT(!outWS->spectrumInfo().isMonitor(2));
    TS_ASSERT_DELTA(outWS->readY(2)[40], 0.3, 0.0001);
    TS_ASSERT_DELTA(outWS->readY(2)[50], 10.3, 0.0001);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove("outWS");
  }

  void test_event_monitors() {
    // Name of the output workspace.
    CreateSampleWorkspace alg;
    alg.setChild(true);
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.setPropertyValue("WorkspaceType", "Event");
    alg.setProperty("NumMonitors", 2);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    EventWorkspace_sptr ews =
        boost::dynamic_pointer_cast<EventWorkspace>(outWS);

    TS_ASSERT_EQUALS(ews->getNumberEvents(), 191900);

    TS_ASSERT(ews->spectrumInfo().isMonitor(0));
    TS_ASSERT_DELTA(ews->readY(0)[50], 257, 0.0001);
    TS_ASSERT_DELTA(ews->readY(0)[60], 7, 0.0001);

    TS_ASSERT(ews->spectrumInfo().isMonitor(1));
    TS_ASSERT_DELTA(ews->readY(1)[50], 257, 0.0001);
    TS_ASSERT_DELTA(ews->readY(1)[60], 7, 0.0001);

    TS_ASSERT(!ews->spectrumInfo().isMonitor(2));
    TS_ASSERT_DELTA(ews->readY(2)[50], 257, 0.0001);
    TS_ASSERT_DELTA(ews->readY(2)[60], 7, 0.0001);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove("outWS");
  }

  void test_ScanningWorkspace_defaults() {
    // Name of the output workspace.
    std::string outWSName("scanning_workspace");

    const int numBanks = 2;
    const int bankPixelWidth = 10;
    const int numScanPoints = 10;

    MatrixWorkspace_sptr ws = createSampleWorkspace(
        outWSName, "", "", "", numBanks, bankPixelWidth, 1000, false, "TOF",
        0.0, 20000.0, 200.0, numScanPoints);

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), numBanks * bankPixelWidth *
                                                    bankPixelWidth *
                                                    numScanPoints);

    const auto &detectorInfo = ws->detectorInfo();
    TS_ASSERT(detectorInfo.isScanning());

    const auto centreDetector = numBanks * bankPixelWidth * bankPixelWidth / 2;
    const auto radiansToDegrees = 180.0 / M_PI;

    // The centre pixel should go from 0 -> 10 degrees, all at the same l2
    for (size_t j = 0; j < detectorInfo.scanCount(centreDetector); ++j) {
      const auto index = std::pair<size_t, size_t>(centreDetector, j);
      TS_ASSERT_DELTA(10.0, detectorInfo.l2(index), 1e-10);
      TS_ASSERT_DELTA(j, detectorInfo.twoTheta(index) * radiansToDegrees,
                      1e-10);
      TS_ASSERT_DELTA(j, detectorInfo.rotation(index).getEulerAngles("XYZ")[1],
                      1e-10);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
};

#endif /* MANTID_ALGORITHMS_CREATESAMPLEWORKSPACETEST_H_ */
