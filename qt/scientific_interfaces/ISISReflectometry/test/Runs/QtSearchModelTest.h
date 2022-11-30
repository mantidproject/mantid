// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <vector>

#include "../../../ISISReflectometry/GUI/Runs/QtSearchModel.h"
#include "../../../ISISReflectometry/GUI/Runs/SearchResult.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using MantidQt::CustomInterfaces::ISISReflectometry::QtSearchModel;
using MantidQt::CustomInterfaces::ISISReflectometry::SearchResult;
using MantidQt::CustomInterfaces::ISISReflectometry::SearchResults;

class QtSearchModelTest : public CxxTest::TestSuite {
public:
  void test_csv_string_generated_correctly_from_search_results() {
    auto results = getTestSearchResults();

    auto model = QtSearchModel();
    auto csv = model.makeSearchResultsCSV(results);
    TS_ASSERT_EQUALS(getTestCSV(), csv)
  }

  void test_csv_string_not_generated_with_no_results() {
    auto results = SearchResults();

    auto expectedCsv = "";

    auto model = QtSearchModel();
    auto csv = model.makeSearchResultsCSV(results);
    TS_ASSERT_EQUALS(expectedCsv, csv)
  }

  void test_getSearchResultsCSV_uses_member_results() {
    auto results = getTestSearchResults();

    auto model = QtSearchModel();
    model.replaceResults(results);
    auto csv = model.getSearchResultsCSV();
    TS_ASSERT_EQUALS(getTestCSV(), csv)
  }

private:
  static SearchResults getTestSearchResults() {
    auto results = SearchResults();
    results.push_back(SearchResult("111111", "a title with a theta th=0.1", "a title with a theta", "0.1", "", "",
                                   "this is a good one"));
    results.push_back(SearchResult("222222", "a title without a theta"));
    results.push_back(SearchResult("333333", "This one is purposely excluded th=0.2", "This one is purposely excluded",
                                   "0.2", "", "it's bad", "something"));

    return results;
  }

  static std::string getTestCSV() {
    return "Run,Description,Exclude,Comment\n"
           "111111,a title with a theta th=0.1,,this is a good one\n"
           "222222,a title without a theta,,\n"
           "333333,This one is purposely excluded th=0.2,it's bad,something\n";
  }
};
