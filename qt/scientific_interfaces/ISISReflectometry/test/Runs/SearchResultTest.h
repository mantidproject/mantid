// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Runs/SearchResult.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using MantidQt::CustomInterfaces::ISISReflectometry::SearchResult;

class SearchResultTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SearchResultTest *createSuite() { return new SearchResultTest(); }
  static void destroySuite(SearchResultTest *suite) { delete suite; }

  void testConstructorSetsRunNumber() {
    auto result = SearchResult("test run", "");
    TS_ASSERT_EQUALS(result.runNumber(), "test run");
  }

  void testErrorIsSetIfRunEmpty() {
    auto result = SearchResult("", "test descriptionth=1.5");
    TS_ASSERT_EQUALS(result.error(), "Run number is not specified");
  }

  void testGroupNameAndThetaAreEmptyIfDescriptionEmpty() {
    auto result = SearchResult("", "");
    TS_ASSERT_EQUALS(result.groupName(), "");
    TS_ASSERT_EQUALS(result.theta(), "");
  }

  void testGroupNameSetFromDescriptionIfThetaNotGiven() {
    auto result = SearchResult("", "test description");
    TS_ASSERT_EQUALS(result.groupName(), "test description");
  }

  void testThetaIsEmptyIfNotIncludedInDescription() {
    auto result = SearchResult("", "test description");
    TS_ASSERT_EQUALS(result.theta(), "");
  }

  void testErrorIsSetIfThetaNotIncludedInDescription() {
    auto result = SearchResult("test run", "test description");
    TS_ASSERT_EQUALS(result.error(), "Theta was not specified in the run title.")
  }

  void testMultipleErrorsAreSetIfRunAndThetaMissing() {
    auto result = SearchResult("", "test description");
    TS_ASSERT_EQUALS(result.error(), "Run number is not specified\nTheta was "
                                     "not specified in the run title.")
  }

  void testGroupNameAndThetaParsedFromDescription() {
    auto result = SearchResult("", "test descriptionth=1.5");
    TS_ASSERT_EQUALS(result.groupName(), "test description");
    TS_ASSERT_EQUALS(result.theta(), "1.5");
  }

  void testGroupAndThetaAreSetIfThetaIsNegative() {
    auto result = SearchResult("", "test descriptionth=-1.5");
    TS_ASSERT_EQUALS(result.groupName(), "test description");
    TS_ASSERT_EQUALS(result.theta(), "-1.5");
  }

  void testErrorIsSetIfThetaIsNegative() {
    auto result = SearchResult("test run", "test descriptionth=-1.5");
    TS_ASSERT_EQUALS(result.error(), "Invalid theta value in run title: -1.5");
  }

  void testWhitespaceBeforeThetaIsIgnored() {
    auto result = SearchResult("", "test descriptionth= 1.5");
    TS_ASSERT_EQUALS(result.groupName(), "test description");
    TS_ASSERT_EQUALS(result.theta(), "1.5");
  }

  void testTextAfterThetaIsIgnored() {
    auto result = SearchResult("", "test descriptionth=1.5 <this is ignored>");
    TS_ASSERT_EQUALS(result.groupName(), "test description");
    TS_ASSERT_EQUALS(result.theta(), "1.5");
  }

  void testGroupNameContainsOnlyWhitespaceWithThetaSpecified() {
    auto result = SearchResult("", "  th=1.5");
    TS_ASSERT_EQUALS(result.groupName(), "  ");
    TS_ASSERT_EQUALS(result.theta(), "1.5");
  }

  void testGroupNameAndThetaEmptyIfDescriptionContainsOnlyWhitespace() {
    auto result = SearchResult("", "  ");
    TS_ASSERT_EQUALS(result.groupName(), "  ");
    TS_ASSERT_EQUALS(result.theta(), "");
  }

  void testThetaIsSetAndGroupNameIsEmptyIfDescriptionOnlyContainsTheta() {
    auto result = SearchResult("", "th=1.5");
    TS_ASSERT_EQUALS(result.groupName(), "");
    TS_ASSERT_EQUALS(result.theta(), "1.5");
  }

  void testWhitespaceOutsideGroupNameIsNotClippedIfThetaIsFound() {
    auto result = SearchResult("", "   test description  th=1.5");
    TS_ASSERT_EQUALS(result.groupName(), "   test description  ");
  }

  void testWhitespaceOutsideGroupNameIsNotClippedIfThetaIsNotFound() {
    auto result = SearchResult("", "   test description  ");
    TS_ASSERT_EQUALS(result.groupName(), "   test description  ");
  }

  void testWhitespaceInsideGroupNameIsNotClippedIfThetaIsFound() {
    auto result = SearchResult("", "test   descriptionth=1.5");
    TS_ASSERT_EQUALS(result.groupName(), "test   description");
  }

  void testWhitespaceInsideGroupNameIsNotClippedIfThetaIsNotFound() {
    auto result = SearchResult("", "test   description");
    TS_ASSERT_EQUALS(result.groupName(), "test   description");
  }

  void testSpecialCharactersInDescription() {
    auto result = SearchResult("", "test*+.descriptionth=1.5");
    TS_ASSERT_EQUALS(result.groupName(), "test*+.description");
    TS_ASSERT_EQUALS(result.theta(), "1.5");
  }

  void testSearchResultsWithSameRunDescriptionAreEqual() {
    auto result1 = SearchResult("run1", "desc1");
    auto result2 = SearchResult("run1", "desc1");
    TS_ASSERT(result1 == result2);
  }

  void testSearchResultsWithSameGroupNameButDifferentDescriptionsAreNotEqual() {
    auto result1 = SearchResult("", "group-title th=1.5<ignored text>");
    auto result2 = SearchResult("", "group-title th=1.5");
    TS_ASSERT(result1 != result2);
  }

  void testSearchResultsWithDifferentRunAreNotEqual() {
    auto result1 = SearchResult("run1", "desc1");
    auto result2 = SearchResult("run2", "desc1");
    TS_ASSERT(result1 != result2);
  }

  void testSearchResultsWithDifferentDescriptionAreNotEqual() {
    auto result1 = SearchResult("run1", "desc1");
    auto result2 = SearchResult("run1", "desc2");
    TS_ASSERT(result1 != result2);
  }
};
