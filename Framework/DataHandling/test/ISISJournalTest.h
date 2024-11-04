// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/ISISJournal.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InternetHelper.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using Mantid::DataHandling::ISISJournal;
using Mantid::Kernel::InternetHelper;
using Mantid::Kernel::Exception::InternetError;
using testing::_;
using testing::NiceMock;
using testing::Return;

namespace {
static constexpr const char *emptyFile = "";

static constexpr const char *badFile = "<NXroot";

static constexpr const char *emptyJournalFile = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
  <NXroot NeXus_version=\"4.3.0\" XML_version=\"mxml\"></NXroot>";

static constexpr const char *emptyIndexFile = "<journal></journal>";

static constexpr const char *invalidIndexFile = "<journal><badtag/></journal>";

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
} // namespace

class MockInternetHelper : public InternetHelper {
public:
  // The constructor specifies the string that we want sendRequest to return
  MockInternetHelper(std::string const &returnString) : m_returnString(returnString){};

  MOCK_METHOD2(sendRequestProxy, InternetHelper::HTTPStatus(std::string const &, std::ostream &));

  InternetHelper::HTTPStatus sendRequest(std::string const &url, std::ostream &serverReply) override {
    serverReply << m_returnString;
    return sendRequestProxy(url, serverReply);
  }

private:
  std::string m_returnString;
};

class ISISJournalTest : public CxxTest::TestSuite {
public:
  void tearDown() override {
    // Verifying and clearing of expectations happens when mock variables are destroyed.
    // Some of our mocks are created as member variables and will exist until all tests have run, so we need to
    // explicitly verify and clear them after each test.
    verifyAndClear();
  }

  void test_getRuns_requests_correct_url() {
    auto journal = makeJournal();
    auto url = std::string("http://data.isis.rl.ac.uk/journals/ndxinter/journal_19_4.xml");
    EXPECT_CALL(*m_internetHelper, sendRequestProxy(url, _)).Times(1);
    journal.getRuns();
  }

  void test_getCycleNames_requests_correct_url() {
    auto journal = makeJournal(indexFile);
    auto url = std::string("http://data.isis.rl.ac.uk/journals/ndxinter/journal_main.xml");
    EXPECT_CALL(*m_internetHelper, sendRequestProxy(url, _)).Times(1);
    journal.getCycleNames();
  }

  void test_getRuns_when_server_returns_journalFile() {
    auto journal = makeJournal(journalFile);
    auto results = journal.getRuns();
    auto const expected =
        std::vector<ISISJournal::RunData>{{{"name", "INTER22345"}}, {{"name", "INTER12345"}}, {{"name", "INTER12346"}}};
    TS_ASSERT_EQUALS(results, expected);
  }

  void test_getCycleNames_when_server_returns_indexFile() {
    auto journal = makeJournal(indexFile);
    auto results = journal.getCycleNames();
    auto const expected = std::vector<std::string>{"17_1", "18_1", "19_1", "19_2"};
    TS_ASSERT_EQUALS(results, expected);
  }

  void test_getRuns_throws_if_url_not_found() {
    auto journal = makeJournal();
    expectURLNotFound();
    TS_ASSERT_THROWS(journal.getRuns(), InternetError const &);
  }

  void test_getCycleNames_throws_if_url_not_found() {
    auto journal = makeJournal(indexFile);
    expectURLNotFound();
    TS_ASSERT_THROWS(journal.getCycleNames(), InternetError const &);
  }

  void test_getRuns_with_empty_file_throws() {
    auto journal = makeJournal(emptyFile);
    TS_ASSERT_THROWS(journal.getRuns(), std::runtime_error const &);
  }

  void test_getRuns_with_bad_xml_throws() {
    auto journal = makeJournal(badFile);
    TS_ASSERT_THROWS(journal.getRuns(), std::runtime_error const &);
  }

  void test_getRuns_with_empty_xml_file_returns_empty_results() {
    auto journal = makeJournal(emptyJournalFile);
    auto results = journal.getRuns(valuesToLookup(), filters());
    TS_ASSERT_EQUALS(results, std::vector<ISISJournal::RunData>{});
  }

  void test_getRuns_still_returns_run_names_when_requested_values_list_is_empty() {
    auto journal = makeJournal();
    auto results = journal.getRuns(emptyValueNames(), filters());
    auto const expected = std::vector<ISISJournal::RunData>{{{"name", "INTER12345"}}, {{"name", "INTER12346"}}};
  }

  void test_getRuns_returns_all_run_names_when_values_list_and_filters_are_empty() {
    auto journal = makeJournal();
    auto results = journal.getRuns();
    auto const expected =
        std::vector<ISISJournal::RunData>{{{"name", "INTER22345"}}, {{"name", "INTER12345"}}, {{"name", "INTER12346"}}};
  }

  void test_getRuns_returns_requested_values_filtered_by_one_filter() {
    auto journal = makeJournal();
    auto results = journal.getRuns(valuesToLookup(), filters());
    auto const expected = std::vector<ISISJournal::RunData>{
        {{"name", "INTER12345"}, {"run_number", "12345"}, {"title", "Experiment 1 run 1"}},
        {{"name", "INTER12346"}, {"run_number", "12346"}, {"title", "Experiment 1 run 2"}}};
    TS_ASSERT_EQUALS(results, expected);
  }

  void test_getRuns_returns_requested_values_filtered_by_multiple_filters() {
    auto journal = makeJournal();
    auto results = journal.getRuns(valuesToLookup(), multipleFilters());
    auto const expected = std::vector<ISISJournal::RunData>{
        {{"name", "INTER12346"}, {"run_number", "12346"}, {"title", "Experiment 1 run 2"}}};
    TS_ASSERT_EQUALS(results, expected);
  }

  void test_getRuns_returns_requested_values_for_all_entries_when_no_filter_is_set() {
    auto journal = makeJournal();
    auto results = journal.getRuns(valuesToLookup(), emptyFilters());
    auto const expected = std::vector<ISISJournal::RunData>{
        {{"name", "INTER22345"}, {"run_number", "22345"}, {"title", "Experiment 2 run 1"}},
        {{"name", "INTER12345"}, {"run_number", "12345"}, {"title", "Experiment 1 run 1"}},
        {{"name", "INTER12346"}, {"run_number", "12346"}, {"title", "Experiment 1 run 2"}}};
    TS_ASSERT_EQUALS(results, expected);
  }

  void test_getCycleList_with_empty_file_throws() {
    auto journal = makeJournal(emptyFile);
    TS_ASSERT_THROWS(journal.getCycleNames(), std::runtime_error const &);
  }

  void test_getCycleList_with_bad_xml_throws() {
    auto journal = makeJournal(badFile);
    TS_ASSERT_THROWS(journal.getCycleNames(), std::runtime_error const &);
  }

  void test_getCycleList_with_empty_xml_file_returns_empty_results() {
    auto journal = makeJournal(emptyIndexFile);
    auto results = journal.getCycleNames();
    TS_ASSERT_EQUALS(results, std::vector<std::string>{});
  }

  void test_getCycleList_throws_when_invalid_element_names() {
    auto journal = makeJournal(invalidIndexFile);
    TS_ASSERT_THROWS(journal.getCycleNames(), std::invalid_argument const &);
  }

  void test_getCycleList_returns_all_valid_cycles() {
    auto journal = makeJournal(indexFile);
    auto results = journal.getCycleNames();
    auto const expected = std::vector<std::string>{"17_1", "18_1", "19_1", "19_2"};
    TS_ASSERT_EQUALS(results, expected);
  }

private:
  NiceMock<MockInternetHelper> *m_internetHelper;

  ISISJournal makeJournal(std::string const &xmlContents = std::string(journalFile)) {
    // Inject a mock internet helper. The journal takes ownership of the
    // unique_ptr but we store a raw pointer to use in our test.
    auto internetHelper = std::make_unique<NiceMock<MockInternetHelper>>(xmlContents);
    m_internetHelper = internetHelper.get();
    auto journal = ISISJournal("INTER", "19_4", std::move(internetHelper));
    // Ensure the internet helper returns an ok status by default.
    auto status = InternetHelper::HTTPStatus::OK;
    ON_CALL(*m_internetHelper, sendRequestProxy(_, _)).WillByDefault(Return(status));
    return journal;
  }

  std::vector<std::string> valuesToLookup() { return std::vector<std::string>{"run_number", "title"}; }

  std::vector<std::string> emptyValueNames() { return std::vector<std::string>{}; }

  ISISJournal::RunData filters() { return ISISJournal::RunData{{"experiment_id", "100001"}}; }

  ISISJournal::RunData multipleFilters() { return ISISJournal::RunData{{"experiment_id", "100001"}, {"count", "5"}}; }

  ISISJournal::RunData emptyFilters() { return ISISJournal::RunData{}; }

  void expectURLNotFound() {
    auto status = InternetHelper::HTTPStatus::NOT_FOUND;
    EXPECT_CALL(*m_internetHelper, sendRequestProxy(_, _)).Times(1).WillOnce(Return(status));
  }

  void verifyAndClear() { TS_ASSERT(testing::Mock::VerifyAndClearExpectations(m_internetHelper)); }
};
