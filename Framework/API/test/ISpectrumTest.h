// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/ISpectrum.h"
#include "MantidFrameworkTestHelpers/FakeObjects.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::HistogramData::Histogram;

class ISpectrumTest : public CxxTest::TestSuite {
public:
  void test_empty_constructor() {
    SpectrumTester s(Histogram::XMode::Points, Histogram::YMode::Counts);
    TS_ASSERT_EQUALS(s.getDetectorIDs().size(), 0);
    TS_ASSERT_EQUALS(s.getSpectrumNo(), 0);
  }

  void test_constructor() {
    SpectrumTester s(1234, Histogram::XMode::Points, Histogram::YMode::Counts);
    TS_ASSERT_EQUALS(s.getDetectorIDs().size(), 0);
    TS_ASSERT_EQUALS(s.getSpectrumNo(), 1234);
  }

  void test_copyInfoFrom() {
    SpectrumTester a(1234, Histogram::XMode::Points, Histogram::YMode::Counts);
    a.addDetectorID(678);
    a.addDetectorID(789);
    SpectrumTester b(456, Histogram::XMode::Points, Histogram::YMode::Counts);

    TS_ASSERT_EQUALS(b.getDetectorIDs().size(), 0);
    b.copyInfoFrom(a);
    TS_ASSERT_EQUALS(b.getDetectorIDs().size(), 2);
    TS_ASSERT_EQUALS(b.getSpectrumNo(), 1234);
  }

  void test_setSpectrumNo() {
    SpectrumTester s(Histogram::XMode::Points, Histogram::YMode::Counts);
    TS_ASSERT_EQUALS(s.getSpectrumNo(), 0);
    s.setSpectrumNo(1234);
    TS_ASSERT_EQUALS(s.getSpectrumNo(), 1234);
  }

  void test_detectorID_handling() {
    SpectrumTester s(Histogram::XMode::Points, Histogram::YMode::Counts);
    TS_ASSERT(s.getDetectorIDs().empty());
    s.addDetectorID(123);
    TS_ASSERT_EQUALS(s.getDetectorIDs().size(), 1);
    TS_ASSERT_EQUALS(*s.getDetectorIDs().begin(), 123);
    s.addDetectorID(456);
    s.addDetectorID(789);
    TS_ASSERT_EQUALS(s.getDetectorIDs().size(), 3);
    TS_ASSERT(s.hasDetectorID(123));
    TS_ASSERT(s.hasDetectorID(456));
    TS_ASSERT(s.hasDetectorID(789));
    TS_ASSERT(!s.hasDetectorID(666)); // No devil! ;)
    TS_ASSERT(!s.hasDetectorID(999));

    int detids[] = {10, 20, 30, 40, 50, 60, 70, 80, 90};
    s.setDetectorIDs(std::set<detid_t>(detids, detids + 3));
    TS_ASSERT_EQUALS(s.getDetectorIDs().size(), 3);
    TS_ASSERT(s.hasDetectorID(20));
    s.addDetectorIDs(std::set<detid_t>(detids + 3, detids + 6));
    TS_ASSERT_EQUALS(s.getDetectorIDs().size(), 6);
    TS_ASSERT(s.hasDetectorID(20));
    TS_ASSERT(s.hasDetectorID(60));
    s.addDetectorIDs(std::set<detid_t>());
    TS_ASSERT_EQUALS(s.getDetectorIDs().size(), 6);
    s.addDetectorIDs(std::vector<detid_t>(detids + 4, detids + 9)); // N.B. check unique elements
    TS_ASSERT_EQUALS(s.getDetectorIDs().size(), 9);
    TS_ASSERT(s.hasDetectorID(10));
    TS_ASSERT(s.hasDetectorID(70));
    s.addDetectorIDs(std::vector<detid_t>());
    TS_ASSERT_EQUALS(s.getDetectorIDs().size(), 9);

    s.clearDetectorIDs();
    TS_ASSERT(s.getDetectorIDs().empty());

    s.addDetectorID(987);
    TS_ASSERT_EQUALS(s.getDetectorIDs().size(), 1);
    s.setDetectorIDs(std::set<detid_t>());
    TS_ASSERT(s.getDetectorIDs().empty());
  }

  void test_use_dx_flag_being_set_when_accessing_dx_with_non_const() {
    SpectrumTester s(Histogram::XMode::Points, Histogram::YMode::Counts);
    s.setPointStandardDeviations(0);
    TS_ASSERT(s.hasDx());

    // setDX vesion 2
    SpectrumTester s4(Histogram::XMode::Points, Histogram::YMode::Counts);
    auto Dx_vec_ptr_type = std::make_shared<Mantid::HistogramData::HistogramDx>(0);
    s4.setSharedDx(Dx_vec_ptr_type);
    TS_ASSERT(s4.hasDx());

    // setDX version 3
    SpectrumTester s5(Histogram::XMode::Points, Histogram::YMode::Counts);
    cow_ptr<Mantid::HistogramData::HistogramDx> Dx_vec_ptr;
    s5.setSharedDx(Dx_vec_ptr);
    TS_ASSERT(s5.hasDx());
  }

  void test_use_dx_flag_is_copied_during_copy_construction() {
    // Copy spectrum which had the flag set
    SpectrumTester s(Histogram::XMode::Points, Histogram::YMode::Counts);
    s.setPointStandardDeviations(0);
    TS_ASSERT(s.hasDx());

    SpectrumTester s2(s);
    TS_ASSERT(s2.hasDx());

    // Copy spectrum which did not have the flag set
    SpectrumTester s3(Histogram::XMode::Points, Histogram::YMode::Counts);
    SpectrumTester s4(s);
    TS_ASSERT(!s3.hasDx());
  }
};
