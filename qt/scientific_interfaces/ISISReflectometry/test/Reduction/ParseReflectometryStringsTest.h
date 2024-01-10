// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/Reduction/ParseReflectometryStrings.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

class ParseReflectometryStringsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ParseReflectometryStringsTest *createSuite() { return new ParseReflectometryStringsTest(); }
  static void destroySuite(ParseReflectometryStringsTest *suite) { delete suite; }

  void testParseRunNumber() {
    auto result = parseRunNumber("13460");
    TS_ASSERT(result.is_initialized());
    TS_ASSERT_EQUALS(result.get(), "13460");
  }

  void testParseRunNumberRemovesWhitespace() {
    auto result = parseRunNumber("  13460\t");
    TS_ASSERT(result.is_initialized());
    TS_ASSERT_EQUALS(result.get(), "13460");
  }

  void testParseRunNumberConsidersAllWhitespaceInvalid() {
    auto result = parseRunNumber("");
    TS_ASSERT(!result.is_initialized());
  }

  void testRunNumberHandlesFreeTextInput() {
    auto result = parseRunNumber("some workspace name");
    TS_ASSERT(result.is_initialized());
  }

  void testParseRunNumberOrWhitespaceExtractsRun() {
    auto result = parseRunNumberOrWhitespace("  13460\t");
    TS_ASSERT(result.is_initialized());
    TS_ASSERT_EQUALS(result.get(), "13460");
  }

  void testParseRunNumberOrWhitespaceReturnsEmptyString() {
    auto result = parseRunNumberOrWhitespace("  \t");
    TS_ASSERT(result.is_initialized());
    TS_ASSERT_EQUALS(result.get(), "");
  }

  void testParseTheta() {
    auto result = parseTheta("0.23");
    TS_ASSERT(result.is_initialized());
    TS_ASSERT_EQUALS(result.get(), 0.23);
  }

  void testParseThetaEmpty() {
    auto result = parseTheta("  \t");
    TS_ASSERT(!result.is_initialized());
  }

  void testParseThetaIgnoresWhitespace() {
    auto result = parseTheta("  \t0.23 ");
    TS_ASSERT(result.is_initialized());
    TS_ASSERT_EQUALS(result.get(), 0.23);
  }

  void testParseThetaConsidersNegativeDoubleInvalid() {
    auto result = parseTheta("-0.23");
    TS_ASSERT(!result.is_initialized());
  }

  void testParseThetaHandlesInvalidCharacters() {
    auto result = parseTheta("bad");
    TS_ASSERT(!result.is_initialized());
  }

  void testParseTitleMatcherEmpty() {
    auto result = parseTitleMatcher("      \t  ");
    TS_ASSERT(!result.is_initialized());
  }

  void testParseTitleMatcher() {
    auto result = parseTitleMatcher(".*");
    TS_ASSERT(result.is_initialized());
  }

  void testParseTitleMatcherHandlesInvalidRegex() {
    auto result = parseTitleMatcher("[");
    TS_ASSERT(!result.is_initialized());
  }

  void testParseOptions() {
    auto result = parseOptions("key1=value1, key2=value2");
    std::map<std::string, std::string> expected = {{"key1", "value1"}, {"key2", "value2"}};
    TS_ASSERT(result.is_initialized());
    TS_ASSERT_EQUALS(result.get(), expected);
  }

  void testParseOptionsReplacesBoolTextStrings() {
    auto result = parseOptions("key1=True, key2=false, key3=falser");
    std::map<std::string, std::string> const expected = {{{"key1", "1"}, {"key2", "0"}, {"key3", "falser"}}};
    TS_ASSERT(result.is_initialized());
    TS_ASSERT_EQUALS(result.get(), expected);
  }

  void testParseOptionsHandlesWhitespace() {
    auto result = parseOptions("\t key1=value1,   key2  =value2\t");
    std::map<std::string, std::string> expected = {{"key1", "value1"}, {"key2", "value2"}};
    TS_ASSERT(result.is_initialized());
    TS_ASSERT_EQUALS(result.get(), expected);
  }

  void testParseOptionsHandlesInvalidInput() {
    auto result = parseOptions("bad");
    TS_ASSERT(!result.is_initialized());
  }

  void testParseProcessingInstructions() {
    auto result = parseProcessingInstructions("1-3");
    TS_ASSERT(result.is_initialized());
    TS_ASSERT(result.get().is_initialized());
    TS_ASSERT_EQUALS(result.get().get(), "1-3");
  }

  void testParseProcessingInstructionsWhitespace() {
    auto result = parseProcessingInstructions("");
    TS_ASSERT(result.is_initialized());
    TS_ASSERT(!result.get().is_initialized());
  }

  void testParseProcessingInstructionsInvalid() {
    auto result = parseProcessingInstructions("bad");
    TS_ASSERT(!result.is_initialized());
  }

  void testParseScaleFactor() {
    auto result = parseScaleFactor("1.5");
    TS_ASSERT(result.is_initialized());
    TS_ASSERT(result.get().is_initialized());
    TS_ASSERT_EQUALS(result.get().get(), 1.5);
  }

  void testParseScaleFactorWhitespace() {
    auto result = parseScaleFactor("");
    TS_ASSERT(result.is_initialized());
    TS_ASSERT(!result.get().is_initialized());
  }

  void testParseScaleFactorInvalid() {
    auto result = parseScaleFactor("bad");
    TS_ASSERT(!result.is_initialized());
  }

  void testParseScaleFactorRejectsZero() {
    auto result = parseScaleFactor("0.0");
    TS_ASSERT(!result.is_initialized());
  }

  void testParseScaleFactorNegative() {
    auto result = parseScaleFactor("-1.0");
    TS_ASSERT(result.get().is_initialized());
    TS_ASSERT_EQUALS(result.get().get(), -1.0);
  }

  void testParseQRange() {
    auto result = parseQRange("0.05", "0.16", "0.02");
    TS_ASSERT_EQUALS(result.which(), asInt(Result::Value));
    TS_ASSERT_EQUALS(boost::get<RangeInQ>(result), RangeInQ(0.05, 0.02, 0.16));
  }

  void testParseQRangeNegativeQStep() {
    auto result = parseQRange("0.05", "0.16", "-1");
    TS_ASSERT_EQUALS(result.which(), asInt(Result::Value));
    TS_ASSERT_EQUALS(boost::get<RangeInQ>(result), RangeInQ(0.05, -1, 0.16));
  }

  void testParseQRangeInvalidQMin() {
    auto result = parseQRange("bad", "0.16", "0.02");
    std::vector<int> expected = {0};
    TS_ASSERT_EQUALS(result.which(), asInt(Result::Error));
    TS_ASSERT_EQUALS(boost::get<std::vector<int>>(result), expected);
  }

  void testParseQRangeInvalidQMax() {
    auto result = parseQRange("0.05", "bad", "0.02");
    std::vector<int> expected = {1};
    TS_ASSERT_EQUALS(result.which(), asInt(Result::Error));
    TS_ASSERT_EQUALS(boost::get<std::vector<int>>(result), expected);
  }

  void testParseQRangeInvalidQStep() {
    auto result = parseQRange("0.05", "0.16", "bad");
    std::vector<int> expected = {2};
    TS_ASSERT_EQUALS(result.which(), asInt(Result::Error));
    TS_ASSERT_EQUALS(boost::get<std::vector<int>>(result), expected);
  }

  void testParseQRangeInvalidQRange() {
    auto result = parseQRange("1.25", "0.01", "0.02");
    std::vector<int> expected = {0, 1};
    TS_ASSERT_EQUALS(result.which(), asInt(Result::Error));
    TS_ASSERT_EQUALS(boost::get<std::vector<int>>(result), expected);
  }

  void testParseRunNumbersSingle() {
    auto result = parseRunNumbers("13460");
    std::vector<std::string> expected = {"13460"};
    TS_ASSERT(result.is_initialized());
    TS_ASSERT_EQUALS(result.get(), expected);
  }

  void testParseRunNumbersWithCommaSeparator() {
    auto result = parseRunNumbers("13460, 13461");
    std::vector<std::string> expected = {"13460", "13461"};
    TS_ASSERT(result.is_initialized());
    TS_ASSERT_EQUALS(result.get(), expected);
  }

  void testParseRunNumbersWithPlusSeparator() {
    auto result = parseRunNumbers("13460+13461");
    std::vector<std::string> expected = {"13460", "13461"};
    TS_ASSERT(result.is_initialized());
    TS_ASSERT_EQUALS(result.get(), expected);
  }

  void testParseRunNumbersIgnoresWhitespace() {
    auto result = parseRunNumbers("  13460,\t13461");
    std::vector<std::string> expected = {"13460", "13461"};
    TS_ASSERT(result.is_initialized());
    TS_ASSERT_EQUALS(result.get(), expected);
  }

  void testParseRunNumbersEmptyExceptWhitespace() {
    auto result = parseRunNumbers("  \t");
    TS_ASSERT(!result.is_initialized());
  }

  void testParseRunNumbersHandlesFreeTextInput() {
    auto result = parseRunNumbers("13460, some workspace");
    TS_ASSERT(result.is_initialized());
  }

  void testParseTransmissionRuns() {
    auto result = parseTransmissionRuns("13463", "13464");
    TransmissionRunPair expected = {"13463", "13464"};
    TS_ASSERT_EQUALS(result.which(), asInt(Result::Value));
    TS_ASSERT_EQUALS(boost::get<TransmissionRunPair>(result), expected);
  }

  void testParseTransmissionRunsIgnoresWhitespace() {
    auto result = parseTransmissionRuns("  13463", " 13464\t ");
    TransmissionRunPair expected = {"13463", "13464"};
    TS_ASSERT_EQUALS(result.which(), asInt(Result::Value));
    TS_ASSERT_EQUALS(boost::get<TransmissionRunPair>(result), expected);
  }

  void testParseTransmissionRunsFirstOnly() {
    auto result = parseTransmissionRuns("13463", "");
    TransmissionRunPair expected = {{"13463"}, std::vector<std::string>()};
    TS_ASSERT_EQUALS(result.which(), asInt(Result::Value));
    TS_ASSERT_EQUALS(boost::get<TransmissionRunPair>(result), expected);
  }

  void testParseTransmissionRunsSecondOnly() {
    auto result = parseTransmissionRuns("", "13464");
    std::vector<int> expected = {0};
    TS_ASSERT_EQUALS(result.which(), asInt(Result::Error));
    TS_ASSERT_EQUALS(boost::get<std::vector<int>>(result), expected);
  }

  void testParseTransmissionRunsHandlesFreeTextInputForFirst() {
    auto result = parseTransmissionRuns("some workspace", "13464");
    TransmissionRunPair expected = {"some workspace", "13464"};
    TS_ASSERT_EQUALS(result.which(), asInt(Result::Value));
    TS_ASSERT_EQUALS(boost::get<TransmissionRunPair>(result), expected);
  }

  void testParseTransmissionRunsHandlesFreeTextInputForSecond() {
    auto result = parseTransmissionRuns("13463", "some workspace");
    TransmissionRunPair expected = {"13463", "some workspace"};
    TS_ASSERT_EQUALS(result.which(), asInt(Result::Value));
    TS_ASSERT_EQUALS(boost::get<TransmissionRunPair>(result), expected);
  }

  void testParseTitleAndThetaFromRunTitle() {
    auto runTitle = "ASF SM=0.75 th=0.8 ['SM2']=0.75";
    auto result = parseTitleAndThetaFromRunTitle(runTitle);
    std::vector<std::string> expected = {"ASF SM=0.75 ", "0.8"};
    TS_ASSERT(result.is_initialized());
    TS_ASSERT_EQUALS(result.get(), expected);
  }

  void testParseTitleAndThetaFromRunTitleReturnsNoneForEmptyString() {
    auto runTitle = "";
    auto result = parseTitleAndThetaFromRunTitle(runTitle);
    TS_ASSERT(!result.is_initialized());
  }

  void testParseTitleAndThetaFromRunTitleWithThetaOnly() {
    auto runTitle = "th=0.8";
    auto result = parseTitleAndThetaFromRunTitle(runTitle);
    std::vector<std::string> expected = {"", "0.8"};
    TS_ASSERT(result.is_initialized());
    TS_ASSERT_EQUALS(result.get(), expected);
  }

  void testParseTitleAndThetaFromRunTitleReturnsNoneForNoTheta() {
    auto runTitle = "ASF SM=0.75";
    auto result = parseTitleAndThetaFromRunTitle(runTitle);
    TS_ASSERT(!result.is_initialized());
  }

private:
  enum class Result : int { Value = 0, Error = 1 };

  // Helper function to cast the result enum value to an int and compare to a variant type
  inline int asInt(Result expected) { return static_cast<int>(expected); }
};
