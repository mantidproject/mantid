// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/ISISJournal.h"

using namespace Mantid::DataHandling::ISISJournal;

class ISISJournalTest : public CxxTest::TestSuite {
public:
  void test_getRunData_with_empty_file_throws() {
    TS_ASSERT_THROWS(
        getRunDataFromFile(emptyFile, defaultTags(), defaultFilters()),
        std::runtime_error const &);
  }

  void test_getRunData_with_bad_xml_throws() {
    TS_ASSERT_THROWS(
        getRunDataFromFile(badFile, defaultTags(), defaultFilters()),
        std::runtime_error const &);
  }

  void test_getRunData_with_empty_xml_file_returns_empty_results() {
    auto results =
        getRunDataFromFile(emptyJournalFile, defaultTags(), defaultFilters());
    TS_ASSERT_EQUALS(results, std::vector<ISISJournalData>{});
  }

  void test_getRunData_still_returns_run_names_when_tags_list_is_empty() {
    auto results =
        getRunDataFromFile(journalFile, emptyTags(), defaultFilters());
    auto const expected = std::vector<ISISJournalData>{
        {{"name", "INTER12345"}}, {{"name", "INTER12346"}}};
  }

  void test_getRunData_returns_all_run_names_when_tags_and_filters_are_empty() {
    auto results = getRunDataFromFile(journalFile);
    auto const expected =
        std::vector<ISISJournalData>{{{"name", "INTER22345"}},
                                     {{"name", "INTER12345"}},
                                     {{"name", "INTER12346"}}};
  }

  void test_getRunData_returns_requested_tags_filtered_by_one_filter() {
    auto results =
        getRunDataFromFile(journalFile, defaultTags(), defaultFilters());
    auto const expected =
        std::vector<ISISJournalData>{{{"name", "INTER12345"},
                                      {"run_number", "12345"},
                                      {"title", "Experiment 1 run 1"}},
                                     {{"name", "INTER12346"},
                                      {"run_number", "12346"},
                                      {"title", "Experiment 1 run 2"}}};
    TS_ASSERT_EQUALS(results, expected);
  }

  void test_getRunData_returns_requested_tags_filtered_by_multiple_filters() {
    auto results =
        getRunDataFromFile(journalFile, defaultTags(), multipleFilters());
    auto const expected =
        std::vector<ISISJournalData>{{{"name", "INTER12346"},
                                      {"run_number", "12346"},
                                      {"title", "Experiment 1 run 2"}}};
    TS_ASSERT_EQUALS(results, expected);
  }

  void
  test_getRunData_returns_requested_tags_for_all_entries_when_no_filter_is_set() {
    auto results =
        getRunDataFromFile(journalFile, defaultTags(), emptyFilters());
    auto const expected =
        std::vector<ISISJournalData>{{{"name", "INTER22345"},
                                      {"run_number", "22345"},
                                      {"title", "Experiment 2 run 1"}},
                                     {{"name", "INTER12345"},
                                      {"run_number", "12345"},
                                      {"title", "Experiment 1 run 1"}},
                                     {{"name", "INTER12346"},
                                      {"run_number", "12346"},
                                      {"title", "Experiment 1 run 2"}}};
    TS_ASSERT_EQUALS(results, expected);
  }

  void test_getCycleList_with_empty_file_throws() {
    TS_ASSERT_THROWS(getCycleListFromFile(emptyFile),
                     std::runtime_error const &);
  }

  void test_getCycleList_with_bad_xml_throws() {
    TS_ASSERT_THROWS(getCycleListFromFile(badFile), std::runtime_error const &);
  }

  void test_getCycleList_with_empty_xml_file_returns_empty_results() {
    auto results = getCycleListFromFile(emptyIndexFile);
    TS_ASSERT_EQUALS(results, std::vector<std::string>{});
  }

  void test_getCycleList_throws_when_invalid_element_names() {
    TS_ASSERT_THROWS(getCycleListFromFile(invalidIndexFile),
                     std::invalid_argument const &);
  }

  void test_getCycleList_returns_all_valid_cycles() {
    auto results = getCycleListFromFile(indexFile);
    auto const expected =
        std::vector<std::string>{"17_1", "18_1", "19_1", "19_2"};
    TS_ASSERT_EQUALS(results, expected);
  }

private:
  ISISJournalTags defaultTags() {
    return ISISJournalTags{"run_number", "title"};
  }

  ISISJournalTags emptyTags() { return ISISJournalTags{}; }

  ISISJournalFilters defaultFilters() {
    return ISISJournalFilters{{"experiment_id", "100001"}};
  }

  ISISJournalFilters multipleFilters() {
    return ISISJournalFilters{{"experiment_id", "100001"}, {"count", "5"}};
  }

  ISISJournalFilters emptyFilters() { return ISISJournalFilters{}; }

  static constexpr const char *emptyFile = "";

  static constexpr const char *badFile = "<NXroot";

  static constexpr const char *emptyJournalFile = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
  <NXroot NeXus_version=\"4.3.0\" XML_version=\"mxml\"></NXroot>";

  static constexpr const char *emptyIndexFile = "<journal></journal>";

  static constexpr const char *invalidIndexFile =
      "<journal><badtag/></journal>";

  static constexpr const char *indexFile = "\
<journal>\
  <file name=\"journal.xml\" />\
  <file name=\"journal_17_1.xml\" />\
  <file name=\"journal_18_1.xml\" />\
  <file name=\"journal_19_1.xml\" />\
  <file name=\"journal_19_2.xml\" />\
</journal>";

  static constexpr const char *journalFile = "\
  <NXroot>\
    <NXentry name=\"INTER22345\">\
      <title>Experiment 2 run 1</title>\
      <experiment_id>200001</experiment_id>\
      <run_number> 22345</run_number>\
      <count> 5  </count>\
    </NXentry>\
    <NXentry name=\"INTER12345\">\
      <title>Experiment 1 run 1</title>\
      <experiment_id>100001</experiment_id>\
      <run_number> 12345</run_number>\
      <count> 3  </count>\
    </NXentry>\
    <NXentry name=\"INTER12346\">\
      <title>Experiment 1 run 2</title>\
      <experiment_id>100001</experiment_id>\
      <run_number> 12346</run_number>\
      <count> 5  </count>\
    </NXentry>\
  </NXroot>";
};
