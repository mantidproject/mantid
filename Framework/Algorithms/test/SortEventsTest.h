// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_SORTEVENTSTEST_H_
#define MANTID_ALGORITHMS_SORTEVENTSTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAlgorithms/SortEvents.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataHandling;
using Mantid::Types::Event::TofEvent;

class SortEventsTest : public CxxTest::TestSuite {
public:
  double BIN_DELTA;
  int NUMPIXELS, NUMBINS;

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SortEventsTest *createSuite() { return new SortEventsTest(); }
  static void destroySuite(SortEventsTest *suite) { delete suite; }

  SortEventsTest() {
    BIN_DELTA = 2.0;
    NUMPIXELS = 20;
    NUMBINS = 50;
  }

  void testSortByTof() {
    std::string wsName("test_inEvent3");
    EventWorkspace_sptr test_in =
        WorkspaceCreationHelper::createRandomEventWorkspace(NUMBINS, NUMPIXELS);
    AnalysisDataService::Instance().add(wsName, test_in);

    Workspace2D_sptr test_in_ws2d =
        WorkspaceCreationHelper::create2DWorkspaceBinned(NUMBINS, NUMPIXELS);
    AnalysisDataService::Instance().add("workspace2d", test_in_ws2d);

    SortEvents sort;
    sort.initialize();
    // Not an event workspace
    TS_ASSERT_THROWS(sort.setPropertyValue("InputWorkspace", "workspace2d"),
                     const std::invalid_argument &);
    // This one will be ok
    sort.setPropertyValue("InputWorkspace", wsName);
    sort.setPropertyValue("SortBy", "X Value");

    TS_ASSERT(sort.execute());
    TS_ASSERT(sort.isExecuted());

    EventWorkspace_const_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<const EventWorkspace>(
            wsName);

    std::vector<TofEvent> ve = outWS->getSpectrum(0).getEvents();
    TS_ASSERT_EQUALS(ve.size(), NUMBINS);
    for (size_t i = 0; i < ve.size() - 1; i++)
      TS_ASSERT_LESS_THAN_EQUALS(ve[i].tof(), ve[i + 1].tof());

    AnalysisDataService::Instance().remove(wsName);
    AnalysisDataService::Instance().remove("workspace2d");
  }

  void testSortByPulseTime() {
    std::string wsName("test_inEvent4");
    EventWorkspace_sptr test_in =
        WorkspaceCreationHelper::createRandomEventWorkspace(NUMBINS, NUMPIXELS);
    AnalysisDataService::Instance().add(wsName, test_in);

    SortEvents sort;
    sort.initialize();
    sort.setPropertyValue("InputWorkspace", wsName);
    sort.setPropertyValue("SortBy", "Pulse Time");
    TS_ASSERT(sort.execute());
    TS_ASSERT(sort.isExecuted());

    EventWorkspace_const_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<const EventWorkspace>(
            wsName);
    std::vector<TofEvent> ve = outWS->getSpectrum(0).getEvents();
    TS_ASSERT_EQUALS(ve.size(), NUMBINS);
    for (size_t i = 0; i < ve.size() - 1; i++)
      TS_ASSERT_LESS_THAN_EQUALS(ve[i].pulseTime(), ve[i + 1].pulseTime());

    AnalysisDataService::Instance().remove(wsName);
  }

  void testSortByPulseTimeTOF() {
    std::string wsName("test_inEvent4");
    EventWorkspace_sptr test_in =
        WorkspaceCreationHelper::createRandomEventWorkspace(NUMBINS, NUMPIXELS);
    AnalysisDataService::Instance().add(wsName, test_in);

    SortEvents sort;
    sort.initialize();
    sort.setPropertyValue("InputWorkspace", wsName);
    sort.setPropertyValue("SortBy", "Pulse Time + TOF");
    TS_ASSERT(sort.execute());
    TS_ASSERT(sort.isExecuted());

    EventWorkspace_const_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<const EventWorkspace>(
            wsName);
    std::vector<TofEvent> ve = outWS->getSpectrum(0).getEvents();
    TS_ASSERT_EQUALS(ve.size(), NUMBINS);
    for (size_t i = 0; i < ve.size() - 1; i++) {
      bool less = true;
      if (ve[i].pulseTime() > ve[i + 1].pulseTime()) {
        less = false;
      } else if ((ve[i].pulseTime() == ve[i + 1].pulseTime()) &&
                 (ve[i].tof() >= ve[i + 1].tof())) {
        less = false;
      }
      if (!less) {
        std::cout << "Event " << i << "  is later than Event " << i + 1 << '\n';
        std::cout << "Event " << i << ": " << ve[i].pulseTime() << " + "
                  << ve[i].tof() << '\n';
      }
      TS_ASSERT(less);
    }

    AnalysisDataService::Instance().remove(wsName);
  }
};

#endif /* MANTID_ALGORITHMS_SORTEVENTSTEST_H_ */
