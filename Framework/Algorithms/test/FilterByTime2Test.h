// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef FILTERBYTIMETEST_H_
#define FILTERBYTIMETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FilterByTime2.h"
#include "MantidDataHandling/LoadEventPreNexus2.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Algorithms;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

class FilterByTime2Test : public CxxTest::TestSuite {
public:
  /** Setup for loading raw data */
  void setUp_Event() {
    inputWS = "eventWS";
    LoadEventPreNexus2 loader;
    loader.initialize();
    std::string eventfile("CNCS_7860_neutron_event.dat");
    std::string pulsefile("CNCS_7860_pulseid.dat");
    loader.setPropertyValue("EventFilename", eventfile);
    loader.setPropertyValue("PulseidFilename", pulsefile);
    loader.setPropertyValue("MappingFilename", "CNCS_TS_2008_08_18.dat");
    loader.setPropertyValue("OutputWorkspace", inputWS);
    loader.execute();
    TS_ASSERT(loader.isExecuted());
  }

  /** In this test, only a very simple event workspace is used
   */
  void NtestTooManyParams() {
    EventWorkspace_sptr ws =
        WorkspaceCreationHelper::createEventWorkspace(1, 1);
    AnalysisDataService::Instance().addOrReplace("eventWS", ws);

    FilterByTime2 algStopTime;
    algStopTime.initialize();
    algStopTime.setPropertyValue("InputWorkspace", "eventWS");
    algStopTime.setPropertyValue("OutputWorkspace", "out");
    algStopTime.setPropertyValue("StopTime", "120");
    algStopTime.setPropertyValue("AbsoluteStartTime", "2010");
    algStopTime.execute();
    TS_ASSERT(!algStopTime.isExecuted());

    FilterByTime2 algStartStopTime;
    algStartStopTime.initialize();
    algStartStopTime.setPropertyValue("InputWorkspace", "eventWS");
    algStartStopTime.setPropertyValue("OutputWorkspace", "out");
    algStartStopTime.setPropertyValue("StartTime", "60");
    algStartStopTime.setPropertyValue("StopTime", "120");
    algStartStopTime.setPropertyValue("AbsoluteStartTime", "2010");
    algStartStopTime.execute();
    TS_ASSERT(!algStartStopTime.isExecuted());

    FilterByTime2 algStopAbsStartAbsStopTime;
    algStopAbsStartAbsStopTime.initialize();
    algStopAbsStartAbsStopTime.setPropertyValue("InputWorkspace", "eventWS");
    algStopAbsStartAbsStopTime.setPropertyValue("OutputWorkspace", "out");
    algStopAbsStartAbsStopTime.setPropertyValue("StopTime", "120");
    algStopAbsStartAbsStopTime.setPropertyValue("AbsoluteStartTime", "2010");
    algStopAbsStartAbsStopTime.setPropertyValue("AbsoluteStopTime", "2010-03");
    algStopAbsStartAbsStopTime.execute();
    TS_ASSERT(!algStopAbsStartAbsStopTime.isExecuted());

    return;
  }

  /** Test Filter by relative time and absolute time
   */
  void testExecEventWorkspace_relativeTime_and_absolute_time() {
    std::string outputWS;
    this->setUp_Event();

    // Retrieve Workspace
    WS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(inputWS);
    TS_ASSERT(WS); // workspace is loaded

    // Do the filtering now.
    FilterByTime2 algRelative;
    algRelative.initialize();
    TS_ASSERT_THROWS_NOTHING(
        algRelative.setPropertyValue("InputWorkspace", inputWS));
    outputWS = "eventWS_relative";
    TS_ASSERT_THROWS_NOTHING(
        algRelative.setPropertyValue("OutputWorkspace", outputWS));
    // Get 1 minute worth
    TS_ASSERT_THROWS_NOTHING(algRelative.setPropertyValue("StartTime", "60"));
    TS_ASSERT_THROWS_NOTHING(algRelative.setPropertyValue("StopTime", "120"));

    algRelative.execute();
    TS_ASSERT(algRelative.isExecuted());

    // Retrieve Workspace changed
    EventWorkspace_sptr outWS;
    outWS =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outputWS);
    TS_ASSERT(outWS); // workspace is loaded

    // Things that haven't changed
    TS_ASSERT_EQUALS(outWS->blocksize(), WS->blocksize());
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), WS->getNumberHistograms());
    // Things that changed
    TS_ASSERT_LESS_THAN(outWS->getNumberEvents(), WS->getNumberEvents());
    // Proton charge is lower
    TS_ASSERT_LESS_THAN(outWS->run().getProtonCharge(),
                        WS->run().getProtonCharge());

    //-------------- Absolute time filtering --------------------
    FilterByTime2 algAbs;
    algAbs.initialize();
    TS_ASSERT_THROWS_NOTHING(
        algAbs.setPropertyValue("InputWorkspace", inputWS));
    outputWS = "eventWS_absolute";
    TS_ASSERT_THROWS_NOTHING(
        algAbs.setPropertyValue("OutputWorkspace", outputWS));
    // Get 1 minutes worth, starting at minute 1
    TS_ASSERT_THROWS_NOTHING(
        algAbs.setPropertyValue("AbsoluteStartTime", "2010-03-25T16:09:37.46"));
    TS_ASSERT_THROWS_NOTHING(
        algAbs.setPropertyValue("AbsoluteStopTime", "2010-03-25T16:10:37.46"));
    algAbs.execute();
    TS_ASSERT(algAbs.isExecuted());

    EventWorkspace_sptr outWS2;
    outWS2 =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outputWS);
    TS_ASSERT(outWS2); // workspace is loaded

    // Things that haven't changed
    TS_ASSERT_EQUALS(outWS2->blocksize(), WS->blocksize());
    TS_ASSERT_EQUALS(outWS2->getNumberHistograms(), WS->getNumberHistograms());
    // Things that changed
    TS_ASSERT_LESS_THAN(outWS2->getNumberEvents(), WS->getNumberEvents());

    // TODO Fix this later
    // TS_ASSERT_LESS_THAN( outWS2->run().getProtonCharge(),
    // WS->run().getProtonCharge() );

    //------------------ Comparing both -----------------------
    // Similar total number of events
    TS_ASSERT_DELTA(outWS->getNumberEvents(), outWS2->getNumberEvents(), 10);

    int count = 0;
    for (size_t i = 0; i < outWS->getNumberHistograms(); i++) {
      double diff = fabs(double(outWS->getSpectrum(i).getNumberEvents() -
                                outWS2->getSpectrum(i).getNumberEvents()));
      // No more than 2 events difference because of rounding to 0.01 second
      TS_ASSERT_LESS_THAN(diff, 3);
      if (diff > 3)
        count++;
      if (count > 50)
        break;
    }

    // Almost same proton charge
    TS_ASSERT_DELTA(outWS->run().getProtonCharge(),
                    outWS2->run().getProtonCharge(), 0.01);
  }

private:
  std::string inputWS;
  EventWorkspace_sptr WS;
};

#endif /* FILTERBYTIMETEST_H_ */
