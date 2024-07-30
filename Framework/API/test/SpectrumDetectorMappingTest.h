// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidFrameworkTestHelpers/FakeObjects.h"
#include <exception>

using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::SpectrumDetectorMapping;

class SpectrumDetectorMappingTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpectrumDetectorMappingTest *createSuite() { return new SpectrumDetectorMappingTest(); }
  static void destroySuite(SpectrumDetectorMappingTest *suite) { delete suite; }

  void test_workspace_constructor() {
    MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS(SpectrumDetectorMapping map(ws), const std::invalid_argument &);
  }

  void test_workspace_constructor_fills_map() {
    auto ws = std::make_shared<WorkspaceTester>();
    ws->initialize(3, 1, 1);
    // Override some of the default detector numbers to make it more interesting
    ws->getSpectrum(0).setDetectorIDs(std::set<detid_t>());
    int detids[] = {10, 20};
    ws->getSpectrum(2).setDetectorIDs(std::set<detid_t>(detids, detids + 2));
    SpectrumDetectorMapping map(ws);

    TS_ASSERT(map.getDetectorIDsForSpectrumNo(1).empty());
    auto idsFor2 = map.getDetectorIDsForSpectrumNo(2);
    TS_ASSERT_EQUALS(idsFor2.size(), 1);
    TS_ASSERT_EQUALS(idsFor2.count(1), 1);
    auto idsFor3 = map.getDetectorIDsForSpectrumNo(3);
    TS_ASSERT_EQUALS(idsFor3.size(), 2);
    TS_ASSERT_EQUALS(idsFor3.count(10), 1);
    TS_ASSERT_EQUALS(idsFor3.count(20), 1);
  }

  void test_vector_constructor_unequal_lengths() {
    TS_ASSERT_THROWS(SpectrumDetectorMapping(std::vector<specnum_t>(2), std::vector<detid_t>(1)),
                     const std::invalid_argument &);
  }

  void check_the_map(const SpectrumDetectorMapping &map) {
    TS_ASSERT_EQUALS(map.getMapping().size(), 3)
    auto idsFor1 = map.getDetectorIDsForSpectrumNo(1);
    TS_ASSERT_EQUALS(idsFor1.size(), 1);
    TS_ASSERT_EQUALS(idsFor1.count(10), 1);
    auto idsFor2 = map.getDetectorIDsForSpectrumNo(2);
    TS_ASSERT_EQUALS(idsFor2.size(), 2);
    TS_ASSERT_EQUALS(idsFor2.count(20), 1);
    TS_ASSERT_EQUALS(idsFor2.count(99), 1);
    auto idsFor3 = map.getDetectorIDsForSpectrumNo(3);
    TS_ASSERT_EQUALS(idsFor3.size(), 1);
    TS_ASSERT_EQUALS(idsFor3.count(30), 1);
  }

  void test_vector_constructor_uses_all_spectra_by_default() {
    // Empty is fine for the input
    std::vector<specnum_t> specs;
    std::vector<detid_t> detids;
    SpectrumDetectorMapping map(specs, detids);
    TS_ASSERT(map.getMapping().empty());

    // Now fill the vectors and test again
    for (int i = 1; i < 4; ++i) {
      specs.emplace_back(i);
      detids.emplace_back(i * 10);
    }
    // Add a second entry to one
    specs.emplace_back(2);
    detids.emplace_back(99);

    SpectrumDetectorMapping map2(specs, detids);
    check_the_map(map2);
  }

  void test_vector_constructor_ignores_detectors_in_ignore_list() {
    const int ndets = 5;
    std::vector<specnum_t> specs(ndets, 0);
    std::vector<detid_t> detids(ndets, 0);
    for (int i = 0; i < ndets; ++i) {
      specs[i] = i + 1;
      detids[i] = 10 * (i + 1);
    }

    // Ignore first & last
    const size_t nmons = 2;
    std::vector<detid_t> monids(nmons, 0);
    monids[0] = 10;
    monids[1] = 50;

    SpectrumDetectorMapping map(specs, detids, monids);

    TS_ASSERT_EQUALS(3, map.getMapping().size());
    auto idsFor1 = map.getDetectorIDsForSpectrumNo(2);
    TS_ASSERT_EQUALS(idsFor1.size(), 1);
    TS_ASSERT_EQUALS(idsFor1.count(20), 1);
    auto idsFor2 = map.getDetectorIDsForSpectrumNo(3);
    TS_ASSERT_EQUALS(idsFor2.size(), 1);
    TS_ASSERT_EQUALS(idsFor2.count(30), 1);
    auto idsFor3 = map.getDetectorIDsForSpectrumNo(4);
    TS_ASSERT_EQUALS(idsFor3.size(), 1);
    TS_ASSERT_EQUALS(idsFor3.count(40), 1);
  }

  void test_array_constructor_null_inputs() {
    specnum_t specs[2] = {0, 1};
    detid_t detids[2] = {0, 1};
    TS_ASSERT_THROWS(SpectrumDetectorMapping(nullptr, detids, 10), const std::invalid_argument &);
    TS_ASSERT_THROWS(SpectrumDetectorMapping(specs, nullptr, 10), const std::invalid_argument &);
  }

  void test_array_constructor() {
    specnum_t specs[] = {1, 2, 2, 3};
    detid_t detids[] = {10, 99, 20, 30};

    SpectrumDetectorMapping map(specs, detids, 4);
    check_the_map(map);
  }

  void test_getSpectrumNumbers() {
    specnum_t specs[] = {5, 4, 4, 3};
    detid_t detids[] = {10, 99, 20, 30};

    SpectrumDetectorMapping map(specs, detids, 4);
    auto uniqueSpecs = map.getSpectrumNumbers();

    TS_ASSERT_EQUALS(3, uniqueSpecs.size());

    TS_ASSERT_EQUALS(1, uniqueSpecs.count(3));
    TS_ASSERT_EQUALS(1, uniqueSpecs.count(4));
    TS_ASSERT_EQUALS(1, uniqueSpecs.count(5));
  }

  void test_getDetectorIDsForSpectrumNo() {
    MatrixWorkspace_const_sptr ws = std::make_shared<WorkspaceTester>();
    SpectrumDetectorMapping map(ws);
    // The happy path is tested in the methods above. Just test invalid entry
    // here.
    TS_ASSERT_THROWS(map.getDetectorIDsForSpectrumNo(1), const std::out_of_range &);
    TS_ASSERT_THROWS(map.getDetectorIDsForSpectrumNo(0), const std::out_of_range &);
    TS_ASSERT_THROWS(map.getDetectorIDsForSpectrumNo(-1), const std::out_of_range &);
  }
};
