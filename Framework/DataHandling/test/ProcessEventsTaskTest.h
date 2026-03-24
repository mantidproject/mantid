// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/AlignAndFocusPowderSlim/BankCalibration.h"
#include "MantidDataHandling/AlignAndFocusPowderSlim/ProcessEventsTask.h"

using Mantid::detid_t;
using Mantid::DataHandling::AlignAndFocusPowderSlim::BankCalibration;
using Mantid::DataHandling::AlignAndFocusPowderSlim::ProcessEventsTask;

class ProcessEventsTaskTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProcessEventsTaskTest *createSuite() { return new ProcessEventsTaskTest(); }
  static void destroySuite(ProcessEventsTaskTest *suite) { delete suite; }

  void test_ProcessEventsTask() {
    std::vector<detid_t> detIDs = {1, 2, 3, 4, 1, 2, 3, 4};
    std::vector<float> tofs = {1000., 1000., 1000., 1000., 100., 5000., 500., 500.};
    std::vector<double> binEdges = {1000., 2000., 5000.};

    std::map<detid_t, double> calibration_map;
    for (const auto &id : std::views::iota(1, 5))
      calibration_map[id] = id; // simple calibration: tof' = tof * detID for testing

    std::set<detid_t> mask{4};                  // mask detID 4
    std::set<detid_t> det_in_group{1, 2, 3, 4}; // all detectors

    BankCalibration bankCal(1., det_in_group, calibration_map, std::map<detid_t, double>(), mask);

    ProcessEventsTask task(&detIDs, &tofs, &bankCal, &binEdges);
    task(tbb::blocked_range<size_t>(0, tofs.size()));

    // calibrated tofs are 1000(1),2000(2),3000(3),4000(4),100(1),10000(2),1500(3),2000(4)
    // after binning with edges 1000,2000,5000 and remove masked detID 4, we have:

    TS_ASSERT_EQUALS(task.y_temp.size(), 2);
    TS_ASSERT_EQUALS(task.y_temp[0], 2); // 1000(1), 1500(3)
    TS_ASSERT_EQUALS(task.y_temp[1], 2); // 2000(2), 3000(3)
  }
};
