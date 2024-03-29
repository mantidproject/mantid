// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/GlobalSpectrumIndex.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/make_cow.h"
#include "MantidTypes/SpectrumDefinition.h"

using namespace Mantid;
using namespace Indexing;

class IndexInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IndexInfoTest *createSuite() { return new IndexInfoTest(); }
  static void destroySuite(IndexInfoTest *suite) { delete suite; }

  void test_size_constructor() { TS_ASSERT_THROWS_NOTHING(IndexInfo(3)); }

  void test_size_constructor_sets_correct_indices() {
    IndexInfo info(3);
    TS_ASSERT_EQUALS(info.spectrumNumber(0), 1);
    TS_ASSERT_EQUALS(info.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(info.spectrumNumber(2), 3);
    TS_ASSERT(!info.spectrumDefinitions());
  }

  void test_vector_constructor() { TS_ASSERT_THROWS_NOTHING(IndexInfo({3, 2, 1})); }

  void test_vector_constructor_sets_correct_indices() {
    IndexInfo info({3, 2, 1});
    TS_ASSERT_EQUALS(info.spectrumNumber(0), 3);
    TS_ASSERT_EQUALS(info.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(info.spectrumNumber(2), 1);
    TS_ASSERT_EQUALS(info.spectrumNumbers()[0], 3);
    TS_ASSERT_EQUALS(info.spectrumNumbers()[1], 2);
    TS_ASSERT_EQUALS(info.spectrumNumbers()[2], 1);
  }

  void test_construct_from_parent_reorder() {
    IndexInfo parent({3, 2, 1});
    std::vector<SpectrumDefinition> specDefs(3);
    specDefs[0].add(6);
    specDefs[0].add(7);
    specDefs[2].add(8);
    parent.setSpectrumDefinitions(specDefs);

    IndexInfo i(std::vector<SpectrumNumber>{2, 1, 3}, parent);

    TS_ASSERT_EQUALS(i.size(), 3);
    TS_ASSERT_EQUALS(i.globalSize(), 3);
    TS_ASSERT_EQUALS(i.spectrumNumber(0), 2);
    TS_ASSERT_EQUALS(i.spectrumNumber(1), 1);
    TS_ASSERT_EQUALS(i.spectrumNumber(2), 3);
    TS_ASSERT(i.spectrumDefinitions());
    TS_ASSERT_EQUALS((*i.spectrumDefinitions())[0], specDefs[1]);
    TS_ASSERT_EQUALS((*i.spectrumDefinitions())[1], specDefs[2]);
    TS_ASSERT_EQUALS((*i.spectrumDefinitions())[2], specDefs[0]);
  }

  void test_construct_from_parent_filter() {
    IndexInfo parent({3, 2, 1});
    std::vector<SpectrumDefinition> specDefs(3);
    specDefs[0].add(6);
    specDefs[0].add(7);
    specDefs[2].add(8);
    parent.setSpectrumDefinitions(specDefs);

    IndexInfo i(std::vector<SpectrumNumber>{1, 2}, parent);

    TS_ASSERT_EQUALS(i.size(), 2);
    TS_ASSERT_EQUALS(i.globalSize(), 2);
    TS_ASSERT_EQUALS(i.spectrumNumber(0), 1);
    TS_ASSERT_EQUALS(i.spectrumNumber(1), 2);
    TS_ASSERT(i.spectrumDefinitions());
    TS_ASSERT_EQUALS((*i.spectrumDefinitions())[0], specDefs[2]);
    TS_ASSERT_EQUALS((*i.spectrumDefinitions())[1], specDefs[1]);
  }

  void test_size() { TS_ASSERT_EQUALS(IndexInfo(3).size(), 3); }

  void test_copy() {
    IndexInfo info({3, 2, 1});
    const auto defs = Kernel::make_cow<std::vector<SpectrumDefinition>>(3);
    info.setSpectrumDefinitions(defs);
    auto copy(info);
    TS_ASSERT_EQUALS(info.size(), 3);
    TS_ASSERT_EQUALS(copy.size(), 3);
    TS_ASSERT_EQUALS(copy.spectrumNumber(0), 3);
    TS_ASSERT_EQUALS(copy.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(copy.spectrumNumber(2), 1);
    TS_ASSERT_EQUALS(info.spectrumDefinitions(), copy.spectrumDefinitions());
  }

  void test_move() {
    IndexInfo info({3, 2, 1});
    const auto defs = Kernel::make_cow<std::vector<SpectrumDefinition>>(3);
    info.setSpectrumDefinitions(defs);
    auto moved(std::move(info));
    TS_ASSERT_EQUALS(info.size(), 0);
    TS_ASSERT_EQUALS(moved.size(), 3);
    TS_ASSERT_EQUALS(moved.spectrumNumber(0), 3);
    TS_ASSERT_EQUALS(moved.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(moved.spectrumNumber(2), 1);
    TS_ASSERT_EQUALS(info.spectrumDefinitions(), nullptr);
  }

  void test_setSpectrumNumbers_size_mismatch() {
    IndexInfo t(3);
    TS_ASSERT_THROWS(t.setSpectrumNumbers({1, 2}), const std::runtime_error &);
  }

  void test_setSpectrumDefinitions_size_mismatch() {
    IndexInfo t(3);
    TS_ASSERT_THROWS_EQUALS(t.setSpectrumDefinitions(std::vector<SpectrumDefinition>(2)), const std::runtime_error &e,
                            std::string(e.what()), "IndexInfo: Size mismatch when setting new spectrum definitions");
  }

  void test_setSpectrumNumbers() {
    IndexInfo t(3);
    TS_ASSERT_THROWS_NOTHING(t.setSpectrumNumbers({3, 4, 5}));
    TS_ASSERT_EQUALS(t.spectrumNumber(0), 3);
    TS_ASSERT_EQUALS(t.spectrumNumber(1), 4);
    TS_ASSERT_EQUALS(t.spectrumNumber(2), 5);
  }

  void test_setSpectrumDefinitions() {
    IndexInfo t(3);
    std::vector<SpectrumDefinition> specDefs(3);
    specDefs[0].add(6);
    specDefs[1].add(7);
    specDefs[2].add(8);
    TS_ASSERT_THROWS_NOTHING(t.setSpectrumDefinitions(specDefs));
    TS_ASSERT(t.spectrumDefinitions());
    TS_ASSERT_EQUALS((*t.spectrumDefinitions())[0], specDefs[0]);
    TS_ASSERT_EQUALS((*t.spectrumDefinitions())[1], specDefs[1]);
    TS_ASSERT_EQUALS((*t.spectrumDefinitions())[2], specDefs[2]);
  }

  void test_setSpectrumDefinitions_setting_nullptr_fails() {
    // This might be supported in the future but is not needed now and might
    // break some things, so we forbid this for now.
    IndexInfo info(3);
    Kernel::cow_ptr<std::vector<SpectrumDefinition>> defs{nullptr};
    TS_ASSERT_THROWS(info.setSpectrumDefinitions(defs), const std::runtime_error &);
  }

  void test_setSpectrumDefinitions_size_mismatch_cow_ptr() {
    IndexInfo info(3);
    const auto defs = Kernel::make_cow<std::vector<SpectrumDefinition>>(2);
    TS_ASSERT_THROWS(info.setSpectrumDefinitions(defs), const std::runtime_error &);
  }

  void test_setSpectrumDefinitions_cow_ptr() {
    IndexInfo info(3);
    const auto defs = Kernel::make_cow<std::vector<SpectrumDefinition>>(3);
    TS_ASSERT_THROWS_NOTHING(info.setSpectrumDefinitions(defs));
    TS_ASSERT_EQUALS(info.spectrumDefinitions().get(), defs.get());
  }

  void test_globalSpectrumIndicesFromDetectorIndices_fails_without_spec_defs() {
    IndexInfo info(3);
    std::vector<size_t> detectorIndices{6, 8};
    TS_ASSERT_THROWS_EQUALS(info.globalSpectrumIndicesFromDetectorIndices(detectorIndices), const std::runtime_error &e,
                            std::string(e.what()),
                            "IndexInfo::globalSpectrumIndicesFromDetectorIndices -- no spectrum "
                            "definitions available");
  }

  void test_globalSpectrumIndicesFromDetectorIndices_fails_multiple() {
    IndexInfo info(3);
    std::vector<size_t> detectorIndices{6, 8};
    std::vector<SpectrumDefinition> specDefs(3);
    specDefs[0].add(6);
    specDefs[1].add(7);
    specDefs[1].add(77);
    specDefs[2].add(8);
    info.setSpectrumDefinitions(specDefs);
    TS_ASSERT_THROWS_EQUALS(info.globalSpectrumIndicesFromDetectorIndices(detectorIndices), const std::runtime_error &e,
                            std::string(e.what()),
                            "SpectrumDefinition contains multiple entries. No unique mapping from "
                            "detector to spectrum possible");
  }

  void test_globalSpectrumIndicesFromDetectorIndices_fails_missing() {
    IndexInfo info(3);
    std::vector<size_t> detectorIndices{6, 8};
    std::vector<SpectrumDefinition> specDefs(3);
    // Nothing maps to 8
    specDefs[0].add(6);
    specDefs[1].add(7);
    info.setSpectrumDefinitions(specDefs);
    TS_ASSERT_THROWS_EQUALS(info.globalSpectrumIndicesFromDetectorIndices(detectorIndices), const std::runtime_error &e,
                            std::string(e.what()),
                            "Some of the requested detectors do not have a corresponding spectrum");
  }

  void test_globalSpectrumIndicesFromDetectorIndices_scanning_different_time_indices() {
    IndexInfo info(3);
    std::vector<size_t> detectorIndices{6, 8};
    std::vector<SpectrumDefinition> specDefs(3);
    // Two indices map to the same detector at different time indices; typical
    // for time-indexed workspaces.
    specDefs[0].add(6, 1);
    specDefs[1].add(6, 2);
    specDefs[2].add(8, 1);
    info.setSpectrumDefinitions(specDefs);
    TS_ASSERT_THROWS_NOTHING(info.globalSpectrumIndicesFromDetectorIndices(detectorIndices));
    const auto &indices = info.globalSpectrumIndicesFromDetectorIndices(detectorIndices);
    TS_ASSERT_EQUALS(indices.size(), 3);
    TS_ASSERT_EQUALS(indices[0], 0);
    TS_ASSERT_EQUALS(indices[1], 1);
    TS_ASSERT_EQUALS(indices[2], 2);
  }

  void test_globalSpectrumIndicesFromDetectorIndices_scanning_same_time_indices() {
    IndexInfo info(2);
    std::vector<size_t> detectorIndices{6};
    std::vector<SpectrumDefinition> specDefs(2);
    // Two indices map to the same detector at the same time index; throw
    specDefs[0].add(6, 1);
    specDefs[1].add(6, 1);
    info.setSpectrumDefinitions(specDefs);
    TS_ASSERT_THROWS_EQUALS(info.globalSpectrumIndicesFromDetectorIndices(detectorIndices), const std::runtime_error &e,
                            std::string(e.what()),
                            "Some of the spectra map to the same detector at the same time index");
  }

  void test_globalSpectrumIndicesFromDetectorIndices_fails_conflict_miss() {
    IndexInfo info(3);
    std::vector<size_t> detectorIndices{6, 8};
    std::vector<SpectrumDefinition> specDefs(3);
    // Two indices map to same detector, but additionally one is missing.
    specDefs[0].add(6, 1);
    specDefs[1].add(6, 2);
    info.setSpectrumDefinitions(specDefs);
    TS_ASSERT_THROWS_EQUALS(info.globalSpectrumIndicesFromDetectorIndices(detectorIndices), const std::runtime_error &e,
                            std::string(e.what()),
                            "Some of the requested detectors do not have a "
                            "corresponding spectrum");
  }

  void test_globalSpectrumIndicesFromDetectorIndices() {
    IndexInfo info(3);
    std::vector<size_t> detectorIndices{6, 8};
    std::vector<SpectrumDefinition> specDefs(3);
    specDefs[0].add(6);
    specDefs[1].add(7);
    specDefs[2].add(8);
    info.setSpectrumDefinitions(specDefs);
    const auto &indices = info.globalSpectrumIndicesFromDetectorIndices(detectorIndices);
    TS_ASSERT_EQUALS(indices.size(), detectorIndices.size());
    TS_ASSERT_EQUALS(indices[0], 0);
    TS_ASSERT_EQUALS(indices[1], 2);
  }
};
