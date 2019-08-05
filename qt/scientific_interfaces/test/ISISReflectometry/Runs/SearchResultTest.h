// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "../../../ISISReflectometry/GUI/Runs/SearchResult.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using MantidQt::CustomInterfaces::SearchResult;

class SearchResultTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SearchResultTest *createSuite() { return new SearchResultTest(); }
  static void destroySuite(SearchResultTest *suite) { delete suite; }

  void testConstructorSetsRunNumber() {
    auto result = SearchResult("test run", "", "");
    TS_ASSERT_EQUALS(result.runNumber(), "test run");
  }

  void testConstructorSetsLocation() {
    auto result = SearchResult("", "", "test location");
    TS_ASSERT_EQUALS(result.location(), "test location");
  }

  void testSetError() {
    auto result = SearchResult("", "", "");
    result.setError("test error");
    TS_ASSERT_EQUALS(result.error(), "test error");
  }

  void testGroupNameAndThetaAreEmptyIfDescriptionEmpty() {
    auto result = SearchResult("", "", "");
    TS_ASSERT_EQUALS(result.groupName(), "");
    TS_ASSERT_EQUALS(result.theta(), "");
  }

  void testGroupNameSetFromDescriptionIfThetaNotGiven() {
    auto result = SearchResult("", "test description", "");
    TS_ASSERT_EQUALS(result.groupName(), "test description");
  }

  void testThetaIsEmptyIfNotIncludedInDescription() {
    auto result = SearchResult("", "test description", "");
    TS_ASSERT_EQUALS(result.theta(), "");
  }

  void testGroupNameAndThetaParsedFromDescription() {
    auto result = SearchResult("", "test descriptionth=1.5", "");
    TS_ASSERT_EQUALS(result.groupName(), "test description");
    TS_ASSERT_EQUALS(result.theta(), "1.5");
  }

  void testTextAfterThetaIsIgnoredInParsing() {
    auto result =
        SearchResult("", "test descriptionth=1.5 <this is ignored>", "");
    TS_ASSERT_EQUALS(result.groupName(), "test description");
    TS_ASSERT_EQUALS(result.theta(), "1.5");
  }

  void testGroupNameContainsOnlyWhitespaceWithThetaSpecified() {
    auto result = SearchResult("", "  th=1.5", "");
    TS_ASSERT_EQUALS(result.groupName(), "  ");
    TS_ASSERT_EQUALS(result.theta(), "1.5");
  }

  void testGroupNameAndThetaEmptyIfDescriptionContainsOnlyWhitespace() {
    auto result = SearchResult("", "  ", "");
    TS_ASSERT_EQUALS(result.groupName(), "  ");
    TS_ASSERT_EQUALS(result.theta(), "");
  }

  void testThetaIsSetAndGroupNameIsEmptyIfDescriptionOnlyContainsTheta() {
    auto result = SearchResult("", "th=1.5", "");
    TS_ASSERT_EQUALS(result.groupName(), "");
    TS_ASSERT_EQUALS(result.theta(), "1.5");
  }

  void testWhitespaceOutsideGroupNameIsNotClippedIfThetaIsFound() {
    auto result = SearchResult("", "   test description  th=1.5", "");
    TS_ASSERT_EQUALS(result.groupName(), "   test description  ");
  }

  void testWhitespaceOutsideGroupNameIsNotClippedIfThetaIsNotFound() {
    auto result = SearchResult("", "   test description  ", "");
    TS_ASSERT_EQUALS(result.groupName(), "   test description  ");
  }

  void testWhitespaceInsideGroupNameIsNotClippedIfThetaIsFound() {
    auto result = SearchResult("", "test   descriptionth=1.5", "");
    TS_ASSERT_EQUALS(result.groupName(), "test   description");
  }

  void testWhitespaceInsideGroupNameIsNotClippedIfThetaIsNotFound() {
    auto result = SearchResult("", "test   description", "");
    TS_ASSERT_EQUALS(result.groupName(), "test   description");
  }

  void testSpecialCharactersInDescription() {
    auto result = SearchResult("", "test*+.descriptionth=1.5", "");
    TS_ASSERT_EQUALS(result.groupName(), "test*+.description");
    TS_ASSERT_EQUALS(result.theta(), "1.5");
  }

  void testSearchResultsWithSameRunDescriptionAndLocationAreEqual() {
    auto result1 = SearchResult("run1", "desc1", "locn1");
    auto result2 = SearchResult("run1", "desc1", "locn1");
    TS_ASSERT(result1 == result2);
  }

  void testSearchResultsDifferringOnlyByErrorsAreEqual() {
    auto result1 = SearchResult("run1", "desc1", "locn1");
    auto result2 = SearchResult("run1", "desc1", "locn1");
    result1.setError("error1");
    result2.setError("error2");
    TS_ASSERT(result1 == result2);
  }

  void testSearchResultsWithSameGroupNameButDifferentDescriptionsAreNotEqual() {
    auto result1 = SearchResult("", "group-title th=1.5<ignored text>", "");
    auto result2 = SearchResult("", "group-title th=1.5", "");
    TS_ASSERT(result1 != result2);
  }

  void testSearchResultsWithDifferentRunAreNotEqual() {
    auto result1 = SearchResult("run1", "desc1", "locn1");
    auto result2 = SearchResult("run2", "desc1", "locn1");
    TS_ASSERT(result1 != result2);
  }

  void testSearchResultsWithDifferentDescriptionAreNotEqual() {
    auto result1 = SearchResult("run1", "desc1", "locn1");
    auto result2 = SearchResult("run1", "desc2", "locn1");
    TS_ASSERT(result1 != result2);
  }

  void testSearchResultsWithDifferentLocationAreNotEqual() {
    auto result1 = SearchResult("run1", "desc1", "locn1");
    auto result2 = SearchResult("run1", "desc1", "locn2");
    TS_ASSERT(result1 != result2);
  }
};
