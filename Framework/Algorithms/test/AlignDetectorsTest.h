// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/AlignDetectors.h"
#include "MantidDataHandling/LoadNexus.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/Unit.h"

using namespace Mantid::Algorithms;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

class AlignDetectorsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AlignDetectorsTest *createSuite() { return new AlignDetectorsTest(); }
  static void destroySuite(AlignDetectorsTest *suite) { delete suite; }

  /** Setup for loading Nexus data */
  void setUp_HRP38692() {

    LoadNexus loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "HRP38692a.nxs");
    inputWS = "nexusWS";
    loader.setPropertyValue("OutputWorkspace", inputWS);
    loader.execute();
  }

  void testTheBasics() {
    TS_ASSERT_EQUALS(align.name(), "AlignDetectors");
    TS_ASSERT_EQUALS(align.version(), 1);
  }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(align.initialize());
    TS_ASSERT(align.isInitialized());
  }

  /** Test alignDetectors for a Workspace2D loaded from some
   * raw data file.
   */
  void testExecWorkspace2D() {
    setUp_HRP38692();
    if (!align.isInitialized())
      align.initialize();

    TS_ASSERT_THROWS(align.execute(), const std::runtime_error &);

    align.setPropertyValue("InputWorkspace", inputWS);
    const std::string outputWS = "aligned";
    align.setPropertyValue("OutputWorkspace", outputWS);
    align.setPropertyValue("CalibrationFile", "hrpd_new_072_01.cal");

    TS_ASSERT_THROWS_NOTHING(align.execute());
    TS_ASSERT(align.isExecuted());

    std::shared_ptr<MatrixWorkspace> inWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputWS);
    std::shared_ptr<MatrixWorkspace> outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWS);

    TS_ASSERT_EQUALS(outWS->getAxis(0)->unit()->unitID(), "dSpacing");
    TS_ASSERT_EQUALS(outWS->size(), inWS->size());
    TS_ASSERT_EQUALS(outWS->blocksize(), inWS->blocksize());

    TS_ASSERT_DELTA(outWS->x(2)[50], 0.7223, 0.0001);
    TS_ASSERT_EQUALS(outWS->y(2)[50], inWS->y(1)[50]);
    TS_ASSERT_EQUALS(outWS->y(2)[50], inWS->y(1)[50]);

    for (size_t i = 0; i < outWS->getNumberHistograms(); i++) {
      TS_ASSERT_EQUALS(outWS->getSpectrum(i).getSpectrumNo(), inWS->getSpectrum(i).getSpectrumNo());
      TS_ASSERT_EQUALS(outWS->getSpectrum(i).getDetectorIDs().size(), inWS->getSpectrum(i).getDetectorIDs().size());
      TS_ASSERT_EQUALS(*outWS->getSpectrum(i).getDetectorIDs().begin(), *inWS->getSpectrum(i).getDetectorIDs().begin());
    }

    AnalysisDataService::Instance().remove(outputWS);
  }

  /** Setup for loading raw data */
  void setUp_Event() {
    inputWS = "eventWS";
    EventWorkspace_sptr ws = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(1, 10, false);
    ws->getAxis(0)->setUnit("TOF");
    AnalysisDataService::Instance().addOrReplace(inputWS, ws);
  }

  void testExecEventWorkspace_sameOutputWS() {
    this->setUp_Event();
    std::size_t wkspIndex = 1; // a good workspace index (with events)

    // Retrieve Workspace
    WS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(inputWS);
    TS_ASSERT(WS); // workspace is loaded
    size_t start_blocksize = WS->blocksize();
    size_t num_events = WS->getNumberEvents();
    double a_tof = WS->getSpectrum(wkspIndex).getEvents()[0].tof();

    // Start by init'ing the algorithm
    TS_ASSERT_THROWS_NOTHING(align.initialize());
    TS_ASSERT(align.isInitialized());

    // Set all the properties
    align.setPropertyValue("InputWorkspace", inputWS);
    const std::string outputWS = inputWS;
    align.setPropertyValue("OutputWorkspace", outputWS);
    align.setPropertyValue("CalibrationFile", "refl_fake.cal");

    TS_ASSERT_THROWS_NOTHING(align.execute());
    TS_ASSERT(align.isExecuted());

    // WS hasn;t changed

    // Things that haven't changed
    TS_ASSERT_EQUALS(start_blocksize, WS->blocksize());
    TS_ASSERT_EQUALS(num_events, WS->getNumberEvents());
    // But a TOF changed.
    TS_ASSERT_DIFFERS(a_tof, WS->getSpectrum(wkspIndex).getEvents()[0].tof());
  }

  void testExecEventWorkspace_differentOutputWS() {
    this->setUp_Event();
    std::size_t wkspIndex = 1; // a good workspace index (with events)

    // Retrieve Workspace
    WS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(inputWS);
    TS_ASSERT(WS); // workspace is loaded

    // Start by init'ing the algorithm
    TS_ASSERT_THROWS_NOTHING(align.initialize());
    TS_ASSERT(align.isInitialized());

    // Set all the properties
    align.setPropertyValue("InputWorkspace", inputWS);
    const std::string outputWS = "eventWS_changed";
    align.setPropertyValue("OutputWorkspace", outputWS);
    align.setPropertyValue("CalibrationFile", "refl_fake.cal");

    TS_ASSERT_THROWS_NOTHING(align.execute());
    TS_ASSERT(align.isExecuted());

    // Retrieve Workspace changed
    EventWorkspace_sptr outWS;
    outWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outputWS);
    TS_ASSERT(outWS); // workspace is loaded

    // Things that haven't changed
    TS_ASSERT_EQUALS(outWS->blocksize(), WS->blocksize());
    TS_ASSERT_EQUALS(outWS->getNumberEvents(), WS->getNumberEvents());
    // But a TOF changed.
    TS_ASSERT_DIFFERS(outWS->getSpectrum(wkspIndex).getEvents()[0].tof(),
                      WS->getSpectrum(wkspIndex).getEvents()[0].tof());
  }

private:
  AlignDetectors align;
  std::string inputWS;
  EventWorkspace_sptr WS;
};
