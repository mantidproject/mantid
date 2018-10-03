// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef COMPRESSEVENTSTEST_H_
#define COMPRESSEVENTSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidDataHandling/CompressEvents.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::MantidVecPtr;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

class CompressEventsTest : public CxxTest::TestSuite {
public:
  void test_TheBasics() {
    CompressEvents alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_EQUALS(alg.name(), "CompressEvents");
  }

  void test_InvalidInputs() {
    CompressEvents alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS(alg.setPropertyValue("Tolerance", "-1.0"),
                     std::invalid_argument);
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Tolerance", "0.0"));
  }

  void doTest(std::string inputName, std::string outputName, double tolerance,
              int numPixels = 50, double wallClockTolerance = 0.) {
    EventWorkspace_sptr input, output;
    EventType eventType = WEIGHTED_NOTIME;
    if (wallClockTolerance > 0.)
      eventType = WEIGHTED;

    /** Create event workspace with:
     * 50 pixels (or another number)
     * 100 histogrammed bins from 0.0 in steps of 1.0
     * 200 events; two in each bin, at time 0.5, 1.5, etc.
     * PulseTime = 1 second, 2 seconds, etc.
     */
    input = WorkspaceCreationHelper::createEventWorkspace(numPixels, 100, 100,
                                                          0.0, 1.0, 2);
    AnalysisDataService::Instance().addOrReplace(inputName, input);
    // Quick initial check
    TS_ASSERT_EQUALS(input->getNumberEvents(), 200 * numPixels);
    const double inputIntegral =
        input->getSpectrum(0).integrate(0., 1.0 * 100, true);

    CompressEvents alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", inputName);
    alg.setPropertyValue("OutputWorkspace", outputName);
    alg.setProperty("Tolerance", tolerance);
    if (wallClockTolerance > 0.) {
      alg.setProperty("WallClockTolerance", wallClockTolerance);
      alg.setProperty(
          "StartTime",
          "2010-01-01T00:00:00"); // copied from createEventWorkspace
    }
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(
        input = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            inputName));
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            outputName));

    // Avoid accessing empty pointer
    TS_ASSERT(output);
    if (!output)
      return;

    if (inputName == outputName) {
      TS_ASSERT_EQUALS(input, output);
    } else {
      TS_ASSERT_DIFFERS(input, output);
    }

    // Half the previous # of events
    TS_ASSERT_EQUALS(output->getNumberEvents(), 100 * numPixels);

    // Event list is now of type WeightedEventNoTime
    TS_ASSERT_EQUALS(output->getEventType(), eventType);

    TS_ASSERT_DELTA(output->getSpectrum(0).integrate(0., 1.0 * 100, true),
                    inputIntegral, 1.e-6);

    // Check an event to see if it makes sense
    if (output->getSpectrum(0).getNumberEvents() > 0) {
      WeightedEvent ev = output->getSpectrum(0).getEvent(0);
      TS_ASSERT_DELTA(ev.weight(), 2.0, 1e-6);
      TS_ASSERT_DELTA(ev.errorSquared(), 2.0, 1e-6);
      TS_ASSERT_DELTA(ev.tof(), 0.5, 1e-6);
    }

    // We still have an X axis with the right # of bins
    TS_ASSERT_EQUALS(output->readX(0).size(), 101);
    // Y and E values are the same
    TS_ASSERT_DELTA(output->readY(0)[1], 2.0, 1e-5);
    TS_ASSERT_DELTA(output->readE(0)[1], M_SQRT2, 1e-5);
    TS_ASSERT_EQUALS(output->YUnit(), input->YUnit());
    TS_ASSERT_EQUALS(output->getAxis(0)->unit(), input->getAxis(0)->unit());
    TS_ASSERT_EQUALS(output->getAxis(1)->unit(), input->getAxis(1)->unit());
  }

  // WEIGHTED_NOTIME tests
  void test_DifferentOutput() {
    doTest("CompressEvents_input", "CompressEvents_output", 0.5);
  }
  void test_InPlace() {
    doTest("CompressEvents_input", "CompressEvents_input", 0.5);
  }
  void test_DifferentOutput_ZeroTolerance() {
    doTest("CompressEvents_input", "CompressEvents_output", 0.0);
  }
  void test_InPlace_ZeroTolerance() {
    doTest("CompressEvents_input", "CompressEvents_input", 0.0);
  }

  void test_DifferentOutput_Parallel() {
    doTest("CompressEvents_input", "CompressEvents_output", 0.5, 1);
  }
  void test_InPlace_Parallel() {
    doTest("CompressEvents_input", "CompressEvents_input", 0.5, 1);
  }

  //  doTest(std::string inputName, std::string outputName, double tolerance,
  //                int numPixels = 50, double wallClockTolerance = 0.)
  // WEIGHTED tests
  void test_DifferentOutput_WithPulseTime() {
    doTest("CompressEvents_input", "CompressEvents_output", 0.5, 50, .001);
  }
  void test_InPlace_WithPulseTime() {
    doTest("CompressEvents_input", "CompressEvents_input", 0.5, 50, .001);
  }
  void test_DifferentOutput_ZeroTolerance_WithPulseTime() {
    doTest("CompressEvents_input", "CompressEvents_output", 0.0, 50, .001);
  }
  void test_InPlace_ZeroTolerance_WithPulseTime() {
    doTest("CompressEvents_input", "CompressEvents_input", 0.0, 50, .001);
  }
};

#endif
