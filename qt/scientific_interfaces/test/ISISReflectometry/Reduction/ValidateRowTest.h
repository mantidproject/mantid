// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_VALIDATEROWTEST_H_
#define MANTID_CUSTOMINTERFACES_VALIDATEROWTEST_H_
#include "../../../ISISReflectometry/Common/Parse.h"
#include "../../../ISISReflectometry/Reduction/ValidateRow.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;

class ValidateRowTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ValidateRowTest *createSuite() { return new ValidateRowTest(); }
  static void destroySuite(ValidateRowTest *suite) { delete suite; }
  static auto constexpr TOLERANCE = 0.000001;

  void test() {}

  void testParsesTriviallyValidDoubles() {
    TS_ASSERT_DELTA(1.0, parseDouble("1.0").get(), TOLERANCE);
    TS_ASSERT_DELTA(6.4, parseDouble("6.4").get(), TOLERANCE);
    TS_ASSERT_DELTA(0.0, parseDouble("0").get(), TOLERANCE);
    TS_ASSERT_DELTA(-7000.3, parseDouble("-7000.3").get(), TOLERANCE);
  }

  void testParsesValidDoublesWithLeadingAndTrailingWhitespace() {
    TS_ASSERT_DELTA(1.0, parseDouble("  1.0  ").get(), TOLERANCE);
    TS_ASSERT_DELTA(6.4, parseDouble("\n   6.4").get(), TOLERANCE);
    TS_ASSERT_DELTA(0.0, parseDouble("0").get(), TOLERANCE);
    TS_ASSERT_DELTA(-7000.3, parseDouble("\t-7000.3\t").get(), TOLERANCE);
  }

  void testFailsForTriviallyInvalidDoubles() {
    TS_ASSERT_EQUALS(boost::none, parseDouble(""));
    TS_ASSERT_EQUALS(boost::none, parseDouble("ABCD"));
    TS_ASSERT_EQUALS(boost::none, parseDouble("A0.12"));
    TS_ASSERT_EQUALS(boost::none, parseDouble("O.12"));
  }

  void testFailsForOutOfRangeDoubles() {
    auto bigPositiveDoubleAsString = std::string("9", 380);
    TS_ASSERT_EQUALS(boost::none, parseDouble(bigPositiveDoubleAsString));
    auto smallNegativeDoubleAsString = "-" + bigPositiveDoubleAsString;
    TS_ASSERT_EQUALS(boost::none, parseDouble(smallNegativeDoubleAsString));
  }

  void testParsesTriviallyValidInts() {
    TS_ASSERT_EQUALS(1, parseInt("1").get());
    TS_ASSERT_EQUALS(64, parseInt("64").get());
    TS_ASSERT_EQUALS(0, parseInt("0").get());
    TS_ASSERT_EQUALS(-7000, parseInt("-7000").get());
  }

  void testParsesValidIntsWithLeadingAndTrailingWhitespace() {
    TS_ASSERT_EQUALS(10, parseInt("  10  ").get());
    TS_ASSERT_EQUALS(64, parseInt("\n   64").get());
    TS_ASSERT_EQUALS(0, parseInt("  0\r\n").get());
    TS_ASSERT_EQUALS(-7003, parseInt("\t-7003\t").get());
  }

  void testParsesValidIntsWithLeadingZeroes() {
    TS_ASSERT_EQUALS(30, parseInt("000030").get());
    TS_ASSERT_EQUALS(64, parseInt(" 00064").get());
    TS_ASSERT_EQUALS(100, parseInt("00100").get());
  }

  void testFailsForTriviallyInvalidInts() {
    TS_ASSERT_EQUALS(boost::none, parseInt(""));
    TS_ASSERT_EQUALS(boost::none, parseInt("ABCD"));
    TS_ASSERT_EQUALS(boost::none, parseInt("A0"));
    TS_ASSERT_EQUALS(boost::none, parseInt("O.12"));
  }

  void testFailsForOutOfRangeInts() {
    auto bigPositiveIntAsString = std::string("9", 380);
    TS_ASSERT_EQUALS(boost::none, parseInt(bigPositiveIntAsString));
    auto smallNegativeIntAsString = "-" + bigPositiveIntAsString;
    TS_ASSERT_EQUALS(boost::none, parseInt(smallNegativeIntAsString));
  }

  void testParsesTriviallyValidNonNegativeInts() {
    TS_ASSERT_EQUALS(1, parseNonNegativeInt("1").get());
    TS_ASSERT_EQUALS(64, parseNonNegativeInt("64").get());
    TS_ASSERT_EQUALS(0, parseNonNegativeInt("0").get());
    TS_ASSERT_EQUALS(6999, parseNonNegativeInt("6999").get());
  }

  void testParsesValidNonNegativeIntsWithLeadingAndTrailingWhitespace() {
    TS_ASSERT_EQUALS(13, parseNonNegativeInt("  13  ").get());
    TS_ASSERT_EQUALS(58, parseNonNegativeInt("\n   58").get());
    TS_ASSERT_EQUALS(0, parseNonNegativeInt("  0\r\n").get());
    TS_ASSERT_EQUALS(7003, parseNonNegativeInt("\t7003\t").get());
  }

  void testParsesValidNonNegativeIntsWithLeadingZeroes() {
    TS_ASSERT_EQUALS(30, parseNonNegativeInt("000030").get());
    TS_ASSERT_EQUALS(64, parseNonNegativeInt(" 00064").get());
    TS_ASSERT_EQUALS(100, parseNonNegativeInt("00100").get());
  }

  void testFailsForTriviallyInvalidNonNegativeInts() {
    TS_ASSERT_EQUALS(boost::none, parseNonNegativeInt(""));
    TS_ASSERT_EQUALS(boost::none, parseNonNegativeInt("ABCD"));
    TS_ASSERT_EQUALS(boost::none, parseNonNegativeInt("A0"));
    TS_ASSERT_EQUALS(boost::none, parseNonNegativeInt("O.12"));
  }

  void testFailsForOutOfRangeNonNegativeInts() {
    auto bigPositiveIntAsString = std::string("9", 380);
    TS_ASSERT_EQUALS(boost::none, parseNonNegativeInt(bigPositiveIntAsString));
    auto smallNegativeIntAsString = "-" + bigPositiveIntAsString;
    TS_ASSERT_EQUALS(boost::none,
                     parseNonNegativeInt(smallNegativeIntAsString));
  }

  void testFailsForNegativeInts() {
    TS_ASSERT_EQUALS(boost::none, parseNonNegativeInt("-1"));
    TS_ASSERT_EQUALS(boost::none, parseNonNegativeInt("-3400"));
  }

  void testParsesSingleRunNumber() {
    TS_ASSERT_EQUALS(std::vector<std::string>({"100"}),
                     parseRunNumbers("100").get());
    TS_ASSERT_EQUALS(std::vector<std::string>({"102"}),
                     parseRunNumbers("000102").get());
  }

  void testParsesMultipleRunNumbersSeparatedByPlus() {
    TS_ASSERT_EQUALS(std::vector<std::string>({"100", "1002"}),
                     parseRunNumbers("100+1002").get());
    TS_ASSERT_EQUALS(std::vector<std::string>({"102", "111102", "10"}),
                     parseRunNumbers("000102+111102+010").get());
  }

  void testParsesMultipleRunNumbersSeparatedByComma() {
    TS_ASSERT_EQUALS(std::vector<std::string>({"100", "1002"}),
                     parseRunNumbers("100,1002").get());
    TS_ASSERT_EQUALS(std::vector<std::string>({"102", "111102", "10"}),
                     parseRunNumbers("000102+111102+010").get());
  }

  void testFailsForNoRunNumbers() {
    TS_ASSERT_EQUALS(boost::none, parseRunNumbers(""));
    TS_ASSERT_EQUALS(boost::none, parseRunNumbers("   "));
    TS_ASSERT_EQUALS(boost::none, parseRunNumbers("\n\n"));
    TS_ASSERT_EQUALS(boost::none, parseRunNumbers("+"));
  }

  void testFailsForBadRunNumbersMixedWithGood() {
    TS_ASSERT_EQUALS(boost::none, parseRunNumbers("00001+00012A+111249"));
    TS_ASSERT_EQUALS(boost::none, parseRunNumbers("000A01+00012+111249"));
    TS_ASSERT_EQUALS(boost::none, parseRunNumbers("00001+00012+11124D9"));
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
    auto const expected =
        TransmissionRunPair({"1000"}, std::vector<std::string>());
    auto const result =
        boost::get<TransmissionRunPair>(parseTransmissionRuns("1000", ""));
    TS_ASSERT_EQUALS(expected, result);
  }

  void testParsesTwoTransmissionRuns() {
    auto const expected = TransmissionRunPair("1000", "2010");
    auto const result =
        boost::get<TransmissionRunPair>(parseTransmissionRuns("1000", "2010"));
    TS_ASSERT_EQUALS(expected, result);
  }

  void testParsesNoTransmissionRuns() {
    auto const expected = TransmissionRunPair();
    auto const result =
        boost::get<TransmissionRunPair>(parseTransmissionRuns("", ""));
    TS_ASSERT_EQUALS(expected, result);
  }

  void testParsesMultipleTransmissionRunNumbersSeparatedByPlus() {
    auto const expected =
        TransmissionRunPair(std::vector<std::string>{"100", "1002"},
                            std::vector<std::string>{"2200", "2255"});
    auto const result = boost::get<TransmissionRunPair>(
        parseTransmissionRuns("100+1002", "2200 + 2255"));
    TS_ASSERT_EQUALS(expected, result);
  }

  void testParsesMultipleTransmissionRunNumbersSeparatedByComma() {
    auto const expected =
        TransmissionRunPair(std::vector<std::string>{"100", "1002"},
                            std::vector<std::string>{"2200", "2255"});
    auto const result = boost::get<TransmissionRunPair>(
        parseTransmissionRuns("100,1002", "2200, 2255"));
    TS_ASSERT_EQUALS(expected, result);
  }

  void testFailsForOnlySecondTransmissionRun() {
    TS_ASSERT_EQUALS(
        std::vector<int>({0}),
        boost::get<std::vector<int>>(parseTransmissionRuns("", "1000")));
  }

  void testFailsForInvalidFirstTransmissionRun() {
    TS_ASSERT_EQUALS(
        std::vector<int>({0}),
        boost::get<std::vector<int>>(parseTransmissionRuns("HDSK~", "1000")));
  }

  void testFailsForInvalidSecondTransmissionRun() {
    TS_ASSERT_EQUALS(
        std::vector<int>({1}),
        boost::get<std::vector<int>>(parseTransmissionRuns("1000", "10ABSC")));
  }

  void testFailsForInvalidFirstAndSecondTransmissionRun() {
    TS_ASSERT_EQUALS(std::vector<int>({0, 1}),
                     boost::get<std::vector<int>>(
                         parseTransmissionRuns("1bad000", "10ABSC")));
  }
};
#endif // MANTID_CUSTOMINTERFACES_VALIDATEROWTEST_H_
