#ifndef EXTRACTSINGLESPECTRUMTEST_H_
#define EXTRACTSINGLESPECTRUMTEST_H_

#include "CropWorkspaceTest.h" // Use the test label functionality as it should do the same thing
#include "MantidAlgorithms/ExtractSingleSpectrum.h"
#include "MantidTestHelpers/InstrumentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::detid_t;

class ExtractSingleSpectrumTest : public CxxTest::TestSuite {
public:
  void testName() {
    IAlgorithm *nameTester = createExtractSingleSpectrum();
    TS_ASSERT_EQUALS(nameTester->name(), "ExtractSingleSpectrum");
  }

  void testVersion() {
    IAlgorithm *versionTester = createExtractSingleSpectrum();
    TS_ASSERT_EQUALS(versionTester->version(), 1);
  }

  void testInit() {
    IAlgorithm *initTester = createExtractSingleSpectrum();
    TS_ASSERT_THROWS_NOTHING(initTester->initialize());
    TS_ASSERT(initTester->isInitialized());
    TS_ASSERT_EQUALS(initTester->getProperties().size(), 3);
  }

  void testExec() {
    using namespace Mantid::API;
    const int nbins(5);
    MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(5, nbins);
    const int wsIndex = 2;
    for (int i = 0; i < nbins + 1; ++i) {
      inputWS->dataX(wsIndex)[i] = i;
      if (i < nbins) {
        inputWS->dataY(wsIndex)[i] = 20 - i;
        inputWS->dataE(wsIndex)[i] = 7;
      }
    }
    MatrixWorkspace_sptr outputWS = runAlgorithm(inputWS, wsIndex);

    TS_ASSERT(outputWS);
    TS_ASSERT_EQUALS(outputWS->blocksize(), 5);
    TS_ASSERT_EQUALS(outputWS->readX(0).size(), nbins + 1);
    TS_ASSERT_EQUALS(outputWS->getAxis(1)->spectraNo(0), wsIndex + 1);
    for (int j = 0; j < nbins + 1; ++j) {
      TS_ASSERT_EQUALS(outputWS->readX(0)[j], j);
      if (j < nbins) {
        TS_ASSERT_EQUALS(outputWS->readY(0)[j], 20 - j);
        TS_ASSERT_EQUALS(outputWS->readE(0)[j], 7);
      }
    }
    do_Spectrum_Tests(outputWS, 3, 3);
  }

  void test_Input_With_TextAxis() {
    Algorithm *extractorWithText = new ExtractSingleSpectrum;
    extractorWithText->initialize();
    extractorWithText->setPropertyValue("WorkspaceIndex", "1");
    CropWorkspaceTest::doTestWithTextAxis(extractorWithText); // Takes ownership
  }

  void test_Input_With_Event_Workspace() {
    // Create and input event workspace
    const int eventsPerPixel(25);
    const int numPixels(10);
    EventWorkspace_sptr eventWS = WorkspaceCreationHelper::createEventWorkspace(
        numPixels, 50, eventsPerPixel, 0.0, 1.0, 1 /*EventPattern=1*/);
    InstrumentCreationHelper::addFullInstrumentToWorkspace(*eventWS, false,
                                                           false, "");
    TS_ASSERT(eventWS);
    const int wsIndex(4);
    MatrixWorkspace_sptr output = runAlgorithm(eventWS, wsIndex);

    EventWorkspace_sptr outputWS =
        boost::dynamic_pointer_cast<EventWorkspace>(output);
    TSM_ASSERT("Output should be an event workspace", outputWS);
    const size_t numEvents = outputWS->getNumberEvents();
    TS_ASSERT_EQUALS(numEvents, eventsPerPixel);
    do_Spectrum_Tests(outputWS, 4, 4);
    TS_ASSERT_EQUALS(eventWS->blocksize(), 50);
    TS_ASSERT_DELTA(outputWS->getSpectrum(0).getTofMin(), 4.5, 1e-08);
    TS_ASSERT_DELTA(outputWS->getSpectrum(0).getTofMax(), 28.5, 1e-08);
  }

private:
  ExtractSingleSpectrum *createExtractSingleSpectrum() {
    return new ExtractSingleSpectrum();
  }

  MatrixWorkspace_sptr runAlgorithm(MatrixWorkspace_sptr inputWS,
                                    const int index) {
    Algorithm *extractor = createExtractSingleSpectrum();
    extractor->initialize();
    extractor->setChild(true); // Don't add the output to the ADS, then we don't
                               // have to clear it
    TS_ASSERT_THROWS_NOTHING(extractor->setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        extractor->setPropertyValue("OutputWorkspace", "child_algorithm"));
    TS_ASSERT_THROWS_NOTHING(extractor->setProperty("WorkspaceIndex", index));
    TS_ASSERT_THROWS_NOTHING(extractor->execute());
    TS_ASSERT(extractor->isExecuted());
    if (!extractor->isExecuted()) {
      TS_FAIL("Error running algorithm");
    }
    return extractor->getProperty("OutputWorkspace");
  }

  void do_Spectrum_Tests(MatrixWorkspace_sptr outputWS, const specnum_t specID,
                         const detid_t detID) {
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1);
    TS_ASSERT_THROWS_NOTHING(outputWS->getSpectrum(0));
    const auto &spectrum = outputWS->getSpectrum(0);
    TS_ASSERT_EQUALS(spectrum.getSpectrumNo(), specID);
    auto detids = spectrum.getDetectorIDs();
    TS_ASSERT_EQUALS(detids.size(), 1);
    const detid_t id = *(detids.begin());
    TS_ASSERT_EQUALS(id, detID);
  }
};

#endif /*EXTRACTSINGLESPECTRUMTEST_H_*/
