// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_INDEXING_GROUPTEST_H_
#define MANTID_INDEXING_GROUPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/Group.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidTypes/SpectrumDefinition.h"

using namespace Mantid;
using namespace Indexing;

class GroupTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GroupTest *createSuite() { return new GroupTest(); }
  static void destroySuite(GroupTest *suite) { delete suite; }

  void test_size_mismatch_fail() {
    IndexInfo source({1, 2, 3});
    std::vector<std::vector<size_t>> grouping{{0}, {1}, {2}};
    std::vector<SpectrumNumber> specNums{4, 5};
    TS_ASSERT_THROWS(group(source, std::move(specNums), grouping),
                     const std::runtime_error &);
    TS_ASSERT_EQUALS(specNums.size(), 2);
  }

  void test_no_grouping() {
    IndexInfo source({1, 2, 3});
    std::vector<SpectrumDefinition> specDefs(3);
    specDefs[0].add(10);
    specDefs[1].add(20);
    specDefs[2].add(30);
    source.setSpectrumDefinitions(specDefs);
    std::vector<std::vector<size_t>> grouping{{0}, {1}, {2}};
    auto result = group(source, {4, 5, 6}, grouping);
    TS_ASSERT_EQUALS(result.size(), 3);
    TS_ASSERT_EQUALS(result.spectrumNumber(0), 4);
    TS_ASSERT_EQUALS(result.spectrumNumber(1), 5);
    TS_ASSERT_EQUALS(result.spectrumNumber(2), 6);
    TS_ASSERT_EQUALS((*result.spectrumDefinitions())[0], specDefs[0]);
    TS_ASSERT_EQUALS((*result.spectrumDefinitions())[1], specDefs[1]);
    TS_ASSERT_EQUALS((*result.spectrumDefinitions())[2], specDefs[2]);
  }

  void test_swap_ids() {
    IndexInfo source({1, 2, 3});
    std::vector<SpectrumDefinition> specDefs(3);
    specDefs[0].add(10);
    specDefs[1].add(20);
    specDefs[2].add(30);
    source.setSpectrumDefinitions(specDefs);
    std::vector<std::vector<size_t>> grouping{{1}, {0}, {2}};
    auto result = group(source, {1, 2, 3}, grouping);
    TS_ASSERT_EQUALS(result.size(), 3);
    TS_ASSERT_EQUALS(result.spectrumNumber(0), 1);
    TS_ASSERT_EQUALS(result.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(result.spectrumNumber(2), 3);
    TS_ASSERT_EQUALS((*result.spectrumDefinitions())[0], specDefs[1]);
    TS_ASSERT_EQUALS((*result.spectrumDefinitions())[1], specDefs[0]);
    TS_ASSERT_EQUALS((*result.spectrumDefinitions())[2], specDefs[2]);
  }

  void test_extract() {
    IndexInfo source({1, 2, 3});
    std::vector<SpectrumDefinition> specDefs(3);
    specDefs[0].add(10);
    specDefs[1].add(20);
    specDefs[2].add(30);
    source.setSpectrumDefinitions(specDefs);
    std::vector<std::vector<size_t>> grouping{{1}};
    auto result = group(source, {1}, grouping);
    TS_ASSERT_EQUALS(result.size(), 1);
    TS_ASSERT_EQUALS(result.spectrumNumber(0), 1);
    TS_ASSERT_EQUALS((*result.spectrumDefinitions())[0], specDefs[1]);
  }

  void test_group() {
    IndexInfo source({1, 2, 3});
    std::vector<SpectrumDefinition> specDefs(3);
    specDefs[0].add(10);
    specDefs[1].add(20);
    specDefs[2].add(30);
    source.setSpectrumDefinitions(specDefs);
    std::vector<std::vector<size_t>> grouping{{0, 2}, {1}};
    auto result = group(source, {1, 2}, grouping);
    TS_ASSERT_EQUALS(result.size(), 2);
    TS_ASSERT_EQUALS(result.spectrumNumber(0), 1);
    TS_ASSERT_EQUALS(result.spectrumNumber(1), 2);
    SpectrumDefinition group;
    group.add(10);
    group.add(30);
    TS_ASSERT_EQUALS((*result.spectrumDefinitions())[0], group);
    TS_ASSERT_EQUALS((*result.spectrumDefinitions())[1], specDefs[1]);
  }
};

#endif /* MANTID_INDEXING_GROUPTEST_H_ */
