// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "../../../ISISReflectometry/Reduction/ValidateLookupRow.h"
#include <cxxtest/TestSuite.h>
#include <unordered_set>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

class ValidateLookupRowTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ValidateLookupRowTest *createSuite() { return new ValidateLookupRowTest(); }
  static void destroySuite(ValidateLookupRowTest *suite) { delete suite; }

  void testParseTheta() {
    LookupRowValidator validator;
    auto result = validator({"1.3"});
    TS_ASSERT(result.isValid());
    TS_ASSERT(result.assertValid().thetaOrWildcard().is_initialized());
    TS_ASSERT_EQUALS(result.assertValid().thetaOrWildcard().get(), 1.3);
  }

  void testParseThetaWildcard() {
    LookupRowValidator validator;
    auto result = validator({""});
    TS_ASSERT(result.isValid());
    TS_ASSERT(!result.assertValid().thetaOrWildcard().is_initialized());
  }

  void testParseThetaError() {
    LookupRowValidator validator;
    auto result = validator({"bad"});
    std::unordered_set<int> errorCells = {LookupRow::Column::THETA};
    TS_ASSERT(result.isError());
    TS_ASSERT_EQUALS(result.assertError(), errorCells);
  }

  void testParseTitleMatcherEmpty() {
    LookupRowValidator validator;
    auto result = validator({"0.5", ""});
    TS_ASSERT(result.isValid());                                // Outer initialized (not invalid)
    TS_ASSERT(!result.assertValid().titleMatcher().has_value()) // Inner not initialized (empty)
  }

  void testParseTitleMatcherWhitespace() {
    LookupRowValidator validator;
    auto result = validator({"0.5", "    \t"});
    TS_ASSERT(result.isValid());
    // All whitespace is the equivalent of an empty string
    TS_ASSERT(!result.assertValid().titleMatcher().has_value())
  }

  void testParseTitleMatcherSimpleValid() {
    LookupRowValidator validator;
    std::string const expected = "test";
    auto result = validator({"0.5", expected});
    TS_ASSERT(result.isValid());

    auto const &titleMatcher = result.assertValid().titleMatcher();
    TS_ASSERT(titleMatcher.has_value())
    TS_ASSERT_EQUALS(expected, titleMatcher.value().expression())
  }

  void testParseTitleMatcherRegexCharsValid() {
    LookupRowValidator validator;
    std::string const expected = "test.*";
    auto result = validator({"0.5", expected});
    TS_ASSERT(result.isValid());

    auto const &titleMatcher = result.assertValid().titleMatcher();
    TS_ASSERT(titleMatcher.has_value())
    TS_ASSERT_EQUALS(expected, titleMatcher.value().expression())
  }

  void testParseTitleMatcherInvalid() {
    LookupRowValidator validator;
    auto result = validator({"0.5", "["});
    auto expectedErrorSet = std::unordered_set<int>{LookupRow::Column::TITLE};
    TS_ASSERT(result.isError()); // Outer not initialized (invalid)
    TS_ASSERT_EQUALS(expectedErrorSet, result.assertError())
  }

  void testParseTitleMatcherWithNoThetaIsInvalid() {
    LookupRowValidator validator;
    auto result = validator({"", "test.*"});
    auto expectedErrorSet = std::unordered_set<int>{LookupRow::Column::THETA, LookupRow::Column::TITLE};
    TS_ASSERT(result.isError()); // Outer not initialized (invalid)
    TS_ASSERT_EQUALS(expectedErrorSet, result.assertError())
  }

  void testParseTransmissionRuns() {
    LookupRowValidator validator;
    auto result = validator({"", "", "13463", "13464"});
    auto expected = TransmissionRunPair("13463", "13464");
    TS_ASSERT(result.isValid());
    TS_ASSERT_EQUALS(result.assertValid().transmissionWorkspaceNames(), expected);
  }

  void testParseTransmissionRunsWithWorkspaceNames() {
    LookupRowValidator validator;
    auto result = validator({"", "", "some workspace", "another_workspace"});
    auto expected = TransmissionRunPair("some workspace", "another_workspace");
    TS_ASSERT(result.isValid());
    TS_ASSERT_EQUALS(result.assertValid().transmissionWorkspaceNames(), expected);
  }

  void testParseTransmissionProcessingInstructions() {
    LookupRowValidator validator;
    auto result = validator({"", "", "", "", "4-7"});
    TS_ASSERT(result.isValid());
    TS_ASSERT(result.assertValid().transmissionProcessingInstructions().is_initialized());
    TS_ASSERT_EQUALS(result.assertValid().transmissionProcessingInstructions().get(), "4-7");
  }

  void testParseTransmissionProcessingInstructionsError() {
    LookupRowValidator validator;
    auto result = validator({"", "", "", "", "bad"});
    std::unordered_set<int> errorCells = {LookupRow::Column::TRANS_SPECTRA};
    TS_ASSERT(result.isError());
    TS_ASSERT_EQUALS(result.assertError(), errorCells);
  }

  void testParseQRange() {
    LookupRowValidator validator;
    auto result = validator({"", "", "", "", "", "0.05", "1.3", "0.02"});
    TS_ASSERT(result.isValid());
    TS_ASSERT_EQUALS(result.assertValid().qRange(), RangeInQ(0.05, 0.02, 1.3));
  }

  void testParseQRangeNegativeStep() {
    LookupRowValidator validator;
    auto result = validator({"", "", "", "", "", "0.05", "1.3", "-1"});
    TS_ASSERT(result.isValid());
    TS_ASSERT_EQUALS(result.assertValid().qRange(), RangeInQ(0.05, -1, 1.3));
  }

  void testParseQRangeError() {
    LookupRowValidator validator;
    auto result = validator({"", "", "", "", "", "bad", "bad", "bad"});
    std::unordered_set<int> errorCells = {LookupRow::Column::QMIN, LookupRow::Column::QMAX, LookupRow::Column::QSTEP};
    TS_ASSERT(result.isError());
    TS_ASSERT_EQUALS(result.assertError(), errorCells);
  }

  void testParseScaleFactor() {
    LookupRowValidator validator;
    auto result = validator({"", "", "", "", "", "", "", "", "1.4"});
    TS_ASSERT(result.isValid());
    TS_ASSERT_EQUALS(result.assertValid().scaleFactor(), 1.4);
  }

  void testParseScaleFactorError() {
    LookupRowValidator validator;
    auto result = validator({"", "", "", "", "", "", "", "", "bad"});
    std::unordered_set<int> errorCells = {LookupRow::Column::SCALE};
    TS_ASSERT(result.isError());
    TS_ASSERT_EQUALS(result.assertError(), errorCells);
  }

  void testParseProcessingInstructions() {
    LookupRowValidator validator;
    auto result = validator({"", "", "", "", "", "", "", "", "", "1-3"});
    TS_ASSERT(result.isValid());
    TS_ASSERT(result.assertValid().processingInstructions().is_initialized());
    TS_ASSERT_EQUALS(result.assertValid().processingInstructions().get(), "1-3");
  }

  void testParseProcessingInstructionsError() {
    LookupRowValidator validator;
    auto result = validator({"", "", "", "", "", "", "", "", "", "bad"});
    std::unordered_set<int> errorCells = {LookupRow::Column::RUN_SPECTRA};
    TS_ASSERT(result.isError());
    TS_ASSERT_EQUALS(result.assertError(), errorCells);
  }

  void testParseBackgroundProcessingInstructions() {
    LookupRowValidator validator;
    auto result = validator({"", "", "", "", "", "", "", "", "", "", "4-7"});
    TS_ASSERT(result.isValid());
    TS_ASSERT(result.assertValid().backgroundProcessingInstructions().is_initialized());
    TS_ASSERT_EQUALS(result.assertValid().backgroundProcessingInstructions().get(), "4-7");
  }

  void testParseBackgroundProcessingInstructionsError() {
    LookupRowValidator validator;
    auto result = validator({"", "", "", "", "", "", "", "", "", "", "bad"});
    std::unordered_set<int> errorCells = {LookupRow::Column::BACKGROUND_SPECTRA};
    TS_ASSERT(result.isError());
    TS_ASSERT_EQUALS(result.assertError(), errorCells);
  }

  void testParseROIDetectorIDs() {
    LookupRowValidator validator;
    auto result = validator({"", "", "", "", "", "", "", "", "", "", "", "4-7"});
    TS_ASSERT(result.isValid());
    TS_ASSERT(result.assertValid().roiDetectorIDs().is_initialized());
    TS_ASSERT_EQUALS(result.assertValid().roiDetectorIDs().get(), "4-7");
  }

  void testParseROIDetectorIDsError() {
    LookupRowValidator validator;
    auto result = validator({"", "", "", "", "", "", "", "", "", "", "", "bad"});
    std::unordered_set<int> errorCells = {LookupRow::Column::ROI_DETECTOR_IDS};
    TS_ASSERT(result.isError());
    TS_ASSERT_EQUALS(result.assertError(), errorCells);
  }
};
