// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/Common/Parse.h"
#include "../../../ISISReflectometry/Reduction/ValidateRow.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

class ValidateRowTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ValidateRowTest *createSuite() { return new ValidateRowTest(); }
  static void destroySuite(ValidateRowTest *suite) { delete suite; }
  static auto constexpr TOLERANCE = 0.000001;

  void test() {}

  void testParsesTriviallyValidDoubles() {
    TS_ASSERT_DELTA(1.0, parseDouble("1.0").value(), TOLERANCE);
    TS_ASSERT_DELTA(6.4, parseDouble("6.4").value(), TOLERANCE);
    TS_ASSERT_DELTA(0.0, parseDouble("0").value(), TOLERANCE);
    TS_ASSERT_DELTA(-7000.3, parseDouble("-7000.3").value(), TOLERANCE);
  }

  void testParsesValidDoublesWithLeadingAndTrailingWhitespace() {
    TS_ASSERT_DELTA(1.0, parseDouble("  1.0  ").value(), TOLERANCE);
    TS_ASSERT_DELTA(6.4, parseDouble("\n   6.4").value(), TOLERANCE);
    TS_ASSERT_DELTA(0.0, parseDouble("0").value(), TOLERANCE);
    TS_ASSERT_DELTA(-7000.3, parseDouble("\t-7000.3\t").value(), TOLERANCE);
  }

  void testFailsForTriviallyInvalidDoubles() {
    TS_ASSERT_EQUALS(std::nullopt, parseDouble(""));
    TS_ASSERT_EQUALS(std::nullopt, parseDouble("ABCD"));
    TS_ASSERT_EQUALS(std::nullopt, parseDouble("A0.12"));
    TS_ASSERT_EQUALS(std::nullopt, parseDouble("O.12"));
  }

  void testFailsForOutOfRangeDoubles() {
    auto bigPositiveDoubleAsString = std::string(380, '9');
    TS_ASSERT_EQUALS(std::nullopt, parseDouble(bigPositiveDoubleAsString));
    auto smallNegativeDoubleAsString = "-" + bigPositiveDoubleAsString;
    TS_ASSERT_EQUALS(std::nullopt, parseDouble(smallNegativeDoubleAsString));
  }

  void testParsesTriviallyValidInts() {
    TS_ASSERT_EQUALS(1, parseInt("1").value());
    TS_ASSERT_EQUALS(64, parseInt("64").value());
    TS_ASSERT_EQUALS(0, parseInt("0").value());
    TS_ASSERT_EQUALS(-7000, parseInt("-7000").value());
  }

  void testParsesValidIntsWithLeadingAndTrailingWhitespace() {
    TS_ASSERT_EQUALS(10, parseInt("  10  ").value());
    TS_ASSERT_EQUALS(64, parseInt("\n   64").value());
    TS_ASSERT_EQUALS(0, parseInt("  0\r\n").value());
    TS_ASSERT_EQUALS(-7003, parseInt("\t-7003\t").value());
  }

  void testParsesValidIntsWithLeadingZeroes() {
    TS_ASSERT_EQUALS(30, parseInt("000030").value());
    TS_ASSERT_EQUALS(64, parseInt(" 00064").value());
    TS_ASSERT_EQUALS(100, parseInt("00100").value());
  }

  void testFailsForTriviallyInvalidInts() {
    TS_ASSERT_EQUALS(std::nullopt, parseInt(""));
    TS_ASSERT_EQUALS(std::nullopt, parseInt("ABCD"));
    TS_ASSERT_EQUALS(std::nullopt, parseInt("A0"));
    TS_ASSERT_EQUALS(std::nullopt, parseInt("O.12"));
  }

  void testFailsForOutOfRangeInts() {
    auto bigPositiveIntAsString = std::string(380, '9');
    TS_ASSERT_EQUALS(std::nullopt, parseInt(bigPositiveIntAsString));
    auto smallNegativeIntAsString = "-" + bigPositiveIntAsString;
    TS_ASSERT_EQUALS(std::nullopt, parseInt(smallNegativeIntAsString));
  }

  void testParsesTriviallyValidNonNegativeInts() {
    TS_ASSERT_EQUALS(1, parseNonNegativeInt("1").value());
    TS_ASSERT_EQUALS(64, parseNonNegativeInt("64").value());
    TS_ASSERT_EQUALS(0, parseNonNegativeInt("0").value());
    TS_ASSERT_EQUALS(6999, parseNonNegativeInt("6999").value());
  }

  void testParsesValidNonNegativeIntsWithLeadingAndTrailingWhitespace() {
    TS_ASSERT_EQUALS(13, parseNonNegativeInt("  13  ").value());
    TS_ASSERT_EQUALS(58, parseNonNegativeInt("\n   58").value());
    TS_ASSERT_EQUALS(0, parseNonNegativeInt("  0\r\n").value());
    TS_ASSERT_EQUALS(7003, parseNonNegativeInt("\t7003\t").value());
  }

  void testParsesValidNonNegativeIntsWithLeadingZeroes() {
    TS_ASSERT_EQUALS(30, parseNonNegativeInt("000030").value());
    TS_ASSERT_EQUALS(64, parseNonNegativeInt(" 00064").value());
    TS_ASSERT_EQUALS(100, parseNonNegativeInt("00100").value());
  }

  void testFailsForTriviallyInvalidNonNegativeInts() {
    TS_ASSERT_EQUALS(std::nullopt, parseNonNegativeInt(""));
    TS_ASSERT_EQUALS(std::nullopt, parseNonNegativeInt("ABCD"));
    TS_ASSERT_EQUALS(std::nullopt, parseNonNegativeInt("A0"));
    TS_ASSERT_EQUALS(std::nullopt, parseNonNegativeInt("O.12"));
  }

  void testFailsForOutOfRangeNonNegativeInts() {
    auto bigPositiveIntAsString = std::string(380, '9');
    TS_ASSERT_EQUALS(std::nullopt, parseNonNegativeInt(bigPositiveIntAsString));
    auto smallNegativeIntAsString = "-" + bigPositiveIntAsString;
    TS_ASSERT_EQUALS(std::nullopt, parseNonNegativeInt(smallNegativeIntAsString));
  }

  void testFailsForNegativeInts() {
    TS_ASSERT_EQUALS(std::nullopt, parseNonNegativeInt("-1"));
    TS_ASSERT_EQUALS(std::nullopt, parseNonNegativeInt("-3400"));
  }

  void testParsesSingleRunNumber() {
    TS_ASSERT_EQUALS(std::vector<std::string>({"100"}), parseRunNumbers("100").get());
    TS_ASSERT_EQUALS(std::vector<std::string>({"000102"}), parseRunNumbers("000102").get());
  }

  void testParsesMultipleRunNumbersSeparatedByPlus() {
    TS_ASSERT_EQUALS(std::vector<std::string>({"100", "1002"}), parseRunNumbers("100+1002").get());
    TS_ASSERT_EQUALS(std::vector<std::string>({"000102", "111102", "010"}), parseRunNumbers("000102+111102+010").get());
  }

  void testParsesMultipleRunNumbersSeparatedByComma() {
    TS_ASSERT_EQUALS(std::vector<std::string>({"100", "1002"}), parseRunNumbers("100,1002").get());
    TS_ASSERT_EQUALS(std::vector<std::string>({"000102", "111102", "010"}), parseRunNumbers("000102+111102+010").get());
  }

  void testFailsForNoRunNumbers() {
    TS_ASSERT_EQUALS(boost::none, parseRunNumbers(""));
    TS_ASSERT_EQUALS(boost::none, parseRunNumbers("   "));
    TS_ASSERT_EQUALS(boost::none, parseRunNumbers("\n\n"));
    TS_ASSERT_EQUALS(boost::none, parseRunNumbers("+"));
  }

  void testParsesRunNumbersMixedWithWorkspaceNames() {
    TS_ASSERT_EQUALS(std::vector<std::string>({"00001", "00012A", "111249"}), parseRunNumbers("00001+00012A+111249"));
    TS_ASSERT_EQUALS(std::vector<std::string>({"000A01", "00012", "111249"}), parseRunNumbers("000A01+00012+111249"));
    TS_ASSERT_EQUALS(std::vector<std::string>({"00001", "00012", "11124D9"}), parseRunNumbers("00001+00012+11124D9"));
  }

  void testParseThetaParsesValidThetaValues() {
    TS_ASSERT_DELTA(0.1, parseTheta("0.1"), TOLERANCE);
    TS_ASSERT_DELTA(0.2, parseTheta("0.2"), TOLERANCE);
    TS_ASSERT_DELTA(0.02, parseTheta("0.02"), TOLERANCE);
    TS_ASSERT_DELTA(1, parseTheta("1"), TOLERANCE);
  }

  void testParseThetaFailsForNegativeAndZeroValues() {
    TS_ASSERT_EQUALS(boost::none, parseTheta("-0.01"));
    TS_ASSERT_EQUALS(boost::none, parseTheta("-0.12"));
    TS_ASSERT_EQUALS(boost::none, parseTheta("-1"));
    TS_ASSERT_EQUALS(boost::none, parseTheta("0.0"));
  }

  void testParseScaleFactor() {
    TS_ASSERT_EQUALS(boost::none, parseScaleFactor("ABSC"));
    TS_ASSERT_EQUALS(boost::none, parseScaleFactor("").get());
    TS_ASSERT_EQUALS(0.1, parseScaleFactor("0.1").get().get());
  }

  void testParsesFirstTransmissionRun() {
    auto const expected = TransmissionRunPair({"1000"}, std::vector<std::string>());
    auto const result = boost::get<TransmissionRunPair>(parseTransmissionRuns("1000", ""));
    TS_ASSERT_EQUALS(expected, result);
  }

  void testParsesTwoTransmissionRuns() {
    auto const expected = TransmissionRunPair("1000", "2010");
    auto const result = boost::get<TransmissionRunPair>(parseTransmissionRuns("1000", "2010"));
    TS_ASSERT_EQUALS(expected, result);
  }

  void testParsesNoTransmissionRuns() {
    auto const expected = TransmissionRunPair();
    auto const result = boost::get<TransmissionRunPair>(parseTransmissionRuns("", ""));
    TS_ASSERT_EQUALS(expected, result);
  }

  void testParsesMultipleTransmissionRunNumbersSeparatedByPlus() {
    auto const expected =
        TransmissionRunPair(std::vector<std::string>{"100", "1002"}, std::vector<std::string>{"2200", "2255"});
    auto const result = boost::get<TransmissionRunPair>(parseTransmissionRuns("100+1002", "2200 + 2255"));
    TS_ASSERT_EQUALS(expected, result);
  }

  void testParsesMultipleTransmissionRunNumbersSeparatedByComma() {
    auto const expected =
        TransmissionRunPair(std::vector<std::string>{"100", "1002"}, std::vector<std::string>{"2200", "2255"});
    auto const result = boost::get<TransmissionRunPair>(parseTransmissionRuns("100,1002", "2200, 2255"));
    TS_ASSERT_EQUALS(expected, result);
  }

  void testFailsForOnlySecondTransmissionRun() {
    TS_ASSERT_EQUALS(std::vector<int>({0}), boost::get<std::vector<int>>(parseTransmissionRuns("", "1000")));
  }

  void testParsesWorkspaceNamesForTransmissionRuns() {
    auto const expected = TransmissionRunPair(std::vector<std::string>{"trans1a", "trans1b"},
                                              std::vector<std::string>{"trans2 a", "trans2 b"});
    auto const result = boost::get<TransmissionRunPair>(parseTransmissionRuns("trans1a,trans1b", "trans2 a, trans2 b"));
    TS_ASSERT_EQUALS(expected, result);
  }
};
