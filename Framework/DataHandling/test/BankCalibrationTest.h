// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/AlignAndFocusPowderSlim/BankCalibration.h"
#include <cxxtest/TestSuite.h>
#include <ranges>

using Mantid::detid_t;
using Mantid::DataHandling::AlignAndFocusPowderSlim::BankCalibration;

namespace {
constexpr double TIME_CONVERSION{10};
}

class BankCalibrationTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BankCalibrationTest *createSuite() { return new BankCalibrationTest(); }
  static void destroySuite(BankCalibrationTest *suite) { delete suite; }

  void test_wholeBankInOutput() {
    // simple calibration: tof' = tof * detID for testing
    std::map<detid_t, double> calibration_map;
    for (const auto &detid : std::views::iota(1, 5))
      calibration_map[detid] = detid;

    // simple scale at sample: scale = 2. * detid for testing
    std::map<detid_t, double> scale_map;
    for (const auto &detid : std::views::iota(1, 5))
      scale_map[detid] = 2. * detid;

    // mask detID 4
    std::set<detid_t> mask{4};

    // id range to use
    detid_t DETID_MIN(2);
    detid_t DETID_MAX(4);
    // all detectors go into the bank
    const std::set<detid_t> det_in_group{2, 3, 4};

    // only get a subset of pixels
    BankCalibration bankCalib(TIME_CONVERSION, det_in_group, calibration_map, scale_map, mask);

    // check class constants
    TS_ASSERT_EQUALS(bankCalib.idmin(), DETID_MIN);
    TS_ASSERT_EQUALS(bankCalib.idmax(), DETID_MAX);
    // only check values in range
    TS_ASSERT_EQUALS(bankCalib.value_calibration(2), calibration_map[2] * TIME_CONVERSION);
    TS_ASSERT_EQUALS(bankCalib.value_calibration(3), calibration_map[3] * TIME_CONVERSION);
    TS_ASSERT_EQUALS(bankCalib.value_calibration(4), Mantid::DataHandling::AlignAndFocusPowderSlim::IGNORE_PIXEL);
    TS_ASSERT_EQUALS(bankCalib.value_scale_at_sample(2), scale_map[2]);
    TS_ASSERT_EQUALS(bankCalib.value_scale_at_sample(3), scale_map[3]);
    TS_ASSERT_EQUALS(bankCalib.value_scale_at_sample(4), scale_map[4]);
  }

  void test_partialBankInOutput() {
    // simple calibration: tof' = tof * detID for testing
    std::map<detid_t, double> calibration_map;
    for (const auto &detid : std::views::iota(1, 5))
      calibration_map[detid] = detid;

    // simple scale at sample: scale = 2. * detid for testing
    std::map<detid_t, double> scale_map;
    for (const auto &detid : std::views::iota(1, 5))
      scale_map[detid] = 2. * detid;

    // mask detID 4
    std::set<detid_t> mask{4};

    // id range to use
    detid_t DETID_MIN(3);
    detid_t DETID_MAX(4);
    // all detectors go into the bank
    const std::set<detid_t> det_in_group{3, 4}; // skipping detid=2

    // only get a subset of pixels
    BankCalibration bankCalib(TIME_CONVERSION, det_in_group, calibration_map, scale_map, mask);

    // check class constants
    TS_ASSERT_EQUALS(bankCalib.idmin(), DETID_MIN);
    TS_ASSERT_EQUALS(bankCalib.idmax(), DETID_MAX);
    // only check values in range
    TS_ASSERT_EQUALS(bankCalib.value_calibration(3), calibration_map[3] * TIME_CONVERSION);
    TS_ASSERT_EQUALS(bankCalib.value_calibration(4), Mantid::DataHandling::AlignAndFocusPowderSlim::IGNORE_PIXEL);
    TS_ASSERT_EQUALS(bankCalib.value_scale_at_sample(3), scale_map[3]);
    TS_ASSERT_EQUALS(bankCalib.value_scale_at_sample(4), scale_map[4]); // the value shouldn't get used anyhow
  }

  void test_no_grouping_specified() {
    // simple calibration: tof' = tof * detID for testing
    std::map<detid_t, double> calibration_map;
    for (const auto &detid : std::views::iota(1, 5))
      calibration_map[detid] = detid;

    // simple scale at sample: scale = 2. * detid for testing
    std::map<detid_t, double> scale_map;
    for (const auto &detid : std::views::iota(1, 5))
      scale_map[detid] = 2. * detid;

    // mask detID 4
    std::set<detid_t> mask{4};

    // id range to use
    detid_t DETID_MIN(1);
    detid_t DETID_MAX(4);
    // all detectors go into the bank
    const std::set<detid_t> det_in_group{1, 2, 3, 4};

    // only get a subset of pixels
    BankCalibration bankCalib(TIME_CONVERSION, det_in_group, calibration_map, scale_map, mask);

    // check class constants
    TS_ASSERT_EQUALS(bankCalib.idmin(), DETID_MIN);
    TS_ASSERT_EQUALS(bankCalib.idmax(), DETID_MAX);
    // only check values in range
    TS_ASSERT_EQUALS(bankCalib.value_calibration(2), calibration_map[2] * TIME_CONVERSION);
    TS_ASSERT_EQUALS(bankCalib.value_calibration(3), calibration_map[3] * TIME_CONVERSION);
    TS_ASSERT_EQUALS(bankCalib.value_calibration(4), Mantid::DataHandling::AlignAndFocusPowderSlim::IGNORE_PIXEL);
    TS_ASSERT_EQUALS(bankCalib.value_calibration(42),
                     Mantid::DataHandling::AlignAndFocusPowderSlim::IGNORE_PIXEL); // out of range
    TS_ASSERT_EQUALS(bankCalib.value_scale_at_sample(2), scale_map[2]);
    TS_ASSERT_EQUALS(bankCalib.value_scale_at_sample(3), scale_map[3]);
    TS_ASSERT_EQUALS(bankCalib.value_scale_at_sample(4), scale_map[4]);
    TS_ASSERT_EQUALS(bankCalib.value_scale_at_sample(42),
                     Mantid::DataHandling::AlignAndFocusPowderSlim::IGNORE_PIXEL); // out of range
  }
};
