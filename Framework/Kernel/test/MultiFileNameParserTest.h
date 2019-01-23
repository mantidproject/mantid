// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_MULTIFILENAMEPARSERTEST_H_
#define MANTID_KERNEL_MULTIFILENAMEPARSERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/MultiFileNameParser.h"
#include "MantidTestHelpers/FacilityHelper.h"

#include <Poco/Path.h>
#include <vector>

using namespace Mantid::Kernel;
using namespace Mantid::Kernel::MultiFileNameParsing;

class MultiFileNameParserTest : public CxxTest::TestSuite {
public:
  static MultiFileNameParserTest *createSuite() {
    return new MultiFileNameParserTest();
  }
  static void destroySuite(MultiFileNameParserTest *suite) { delete suite; }

  using ParsedRuns = std::vector<std::vector<unsigned int>>;

  /////////////////////////////////////////////////////////////////////////////
  // Testing of parseMultiRunString.
  /////////////////////////////////////////////////////////////////////////////

  void test_single() {
    ParsedRuns result = parseMultiRunString("1");

    TS_ASSERT_EQUALS(result[0][0], 1);
  }

  void test_listOfSingles() {
    ParsedRuns result = parseMultiRunString("1,2,3,4,5,6,7,8");

    TS_ASSERT_EQUALS(result[0][0], 1);
    TS_ASSERT_EQUALS(result[1][0], 2);
    TS_ASSERT_EQUALS(result[2][0], 3);
    TS_ASSERT_EQUALS(result[3][0], 4);
    TS_ASSERT_EQUALS(result[4][0], 5);
    TS_ASSERT_EQUALS(result[5][0], 6);
    TS_ASSERT_EQUALS(result[6][0], 7);
    TS_ASSERT_EQUALS(result[7][0], 8);
  }

  void test_range() {
    ParsedRuns result = parseMultiRunString("1:8");

    TS_ASSERT_EQUALS(result[0][0], 1);
    TS_ASSERT_EQUALS(result[1][0], 2);
    TS_ASSERT_EQUALS(result[2][0], 3);
    TS_ASSERT_EQUALS(result[3][0], 4);
    TS_ASSERT_EQUALS(result[4][0], 5);
    TS_ASSERT_EQUALS(result[5][0], 6);
    TS_ASSERT_EQUALS(result[6][0], 7);
    TS_ASSERT_EQUALS(result[7][0], 8);
  }

  void test_steppedRange() {
    ParsedRuns result = parseMultiRunString("1:8:3");

    TS_ASSERT_EQUALS(result[0][0], 1);
    TS_ASSERT_EQUALS(result[1][0], 4);
    TS_ASSERT_EQUALS(result[2][0], 7);
  }

  void test_descendingSteppedRange() {
    ParsedRuns result = parseMultiRunString("8:1:3");

    TS_ASSERT_EQUALS(result[0][0], 8);
    TS_ASSERT_EQUALS(result[1][0], 5);
    TS_ASSERT_EQUALS(result[2][0], 2);
  }

  void test_descendingSteppedRange_2() {
    ParsedRuns result = parseMultiRunString("8:1:2");

    TS_ASSERT_EQUALS(result[0][0], 8);
    TS_ASSERT_EQUALS(result[1][0], 6);
    TS_ASSERT_EQUALS(result[2][0], 4);
    TS_ASSERT_EQUALS(result[3][0], 2);
  }

  void test_addedList() {
    ParsedRuns result = parseMultiRunString("1+2+3+4+5+6+7+8");

    TS_ASSERT_EQUALS(result[0][0], 1);
    TS_ASSERT_EQUALS(result[0][1], 2);
    TS_ASSERT_EQUALS(result[0][2], 3);
    TS_ASSERT_EQUALS(result[0][3], 4);
    TS_ASSERT_EQUALS(result[0][4], 5);
    TS_ASSERT_EQUALS(result[0][5], 6);
    TS_ASSERT_EQUALS(result[0][6], 7);
    TS_ASSERT_EQUALS(result[0][7], 8);
  }

  void test_addedRange() {
    ParsedRuns result = parseMultiRunString("1-8");

    TS_ASSERT_EQUALS(result[0][0], 1);
    TS_ASSERT_EQUALS(result[0][1], 2);
    TS_ASSERT_EQUALS(result[0][2], 3);
    TS_ASSERT_EQUALS(result[0][3], 4);
    TS_ASSERT_EQUALS(result[0][4], 5);
    TS_ASSERT_EQUALS(result[0][5], 6);
    TS_ASSERT_EQUALS(result[0][6], 7);
    TS_ASSERT_EQUALS(result[0][7], 8);
  }

  void test_addedSteppedRange() {
    ParsedRuns result = parseMultiRunString("1-8:3");

    TS_ASSERT_EQUALS(result[0][0], 1);
    TS_ASSERT_EQUALS(result[0][1], 4);
    TS_ASSERT_EQUALS(result[0][2], 7);
  }

  void test_descendingAddedRange() {
    ParsedRuns result = parseMultiRunString("8-1");

    TS_ASSERT_EQUALS(result[0][0], 8);
    TS_ASSERT_EQUALS(result[0][1], 7);
    TS_ASSERT_EQUALS(result[0][2], 6);
    TS_ASSERT_EQUALS(result[0][3], 5);
    TS_ASSERT_EQUALS(result[0][4], 4);
    TS_ASSERT_EQUALS(result[0][5], 3);
    TS_ASSERT_EQUALS(result[0][6], 2);
    TS_ASSERT_EQUALS(result[0][7], 1);
  }

  void test_descendingAddedSteppedRange() {
    ParsedRuns result = parseMultiRunString("8-1:3");

    TS_ASSERT_EQUALS(result[0][0], 8);
    TS_ASSERT_EQUALS(result[0][1], 5);
    TS_ASSERT_EQUALS(result[0][2], 2);
  }

  void test_descendingAddedSteppedRange_2() {
    ParsedRuns result = parseMultiRunString("8-1:2");

    TS_ASSERT_EQUALS(result[0][0], 8);
    TS_ASSERT_EQUALS(result[0][1], 6);
    TS_ASSERT_EQUALS(result[0][2], 4);
    TS_ASSERT_EQUALS(result[0][3], 2);
  }

  void test_complexList() {
    ParsedRuns result = parseMultiRunString("1-4,1:4,1+2+3+4,1,2,3,4");

    TS_ASSERT_EQUALS(result[0][0], 1);
    TS_ASSERT_EQUALS(result[0][1], 2);
    TS_ASSERT_EQUALS(result[0][2], 3);
    TS_ASSERT_EQUALS(result[0][3], 4);

    TS_ASSERT_EQUALS(result[1][0], 1);
    TS_ASSERT_EQUALS(result[2][0], 2);
    TS_ASSERT_EQUALS(result[3][0], 3);
    TS_ASSERT_EQUALS(result[4][0], 4);

    TS_ASSERT_EQUALS(result[5][0], 1);
    TS_ASSERT_EQUALS(result[5][1], 2);
    TS_ASSERT_EQUALS(result[5][2], 3);
    TS_ASSERT_EQUALS(result[5][3], 4);

    TS_ASSERT_EQUALS(result[6][0], 1);
    TS_ASSERT_EQUALS(result[7][0], 2);
    TS_ASSERT_EQUALS(result[8][0], 3);
    TS_ASSERT_EQUALS(result[9][0], 4);
  }

  void test_singlePlusAddRange() {
    ParsedRuns result = parseMultiRunString("1+3-6");

    TS_ASSERT_EQUALS(result[0][0], 1);
    TS_ASSERT_EQUALS(result[0][1], 3);
    TS_ASSERT_EQUALS(result[0][2], 4);
    TS_ASSERT_EQUALS(result[0][3], 5);
    TS_ASSERT_EQUALS(result[0][4], 6);
  }

  void test_addRangePlusSingle() {
    ParsedRuns result = parseMultiRunString("1-3+5");

    TS_ASSERT_EQUALS(result[0][0], 1);
    TS_ASSERT_EQUALS(result[0][1], 2);
    TS_ASSERT_EQUALS(result[0][2], 3);
    TS_ASSERT_EQUALS(result[0][3], 5);
  }

  void test_nothingReturnedWhenPassedEmptyString() {
    TS_ASSERT_EQUALS(parseMultiRunString("").size(), 0);
  }

  void test_sumTwoAddRanges() {
    ParsedRuns result = parseMultiRunString("1-2+4-6");

    TS_ASSERT_EQUALS(result[0][0], 1);
    TS_ASSERT_EQUALS(result[0][1], 2);
    TS_ASSERT_EQUALS(result[0][2], 4);
    TS_ASSERT_EQUALS(result[0][3], 5);
    TS_ASSERT_EQUALS(result[0][4], 6);
  }

  void test_sumMultipleAddRanges() {
    ParsedRuns result = parseMultiRunString("1-2+4-6+8-10");

    TS_ASSERT_EQUALS(result.size(), 1)
    TS_ASSERT_EQUALS(result.front().size(), 8)
    TS_ASSERT_EQUALS(result[0][0], 1);
    TS_ASSERT_EQUALS(result[0][1], 2);
    TS_ASSERT_EQUALS(result[0][2], 4);
    TS_ASSERT_EQUALS(result[0][3], 5);
    TS_ASSERT_EQUALS(result[0][4], 6);
    TS_ASSERT_EQUALS(result[0][5], 8);
    TS_ASSERT_EQUALS(result[0][6], 9);
    TS_ASSERT_EQUALS(result[0][7], 10);
  }

  void test_multipleAddRangesAndSinge() {
    ParsedRuns result = parseMultiRunString("1-2+4-6+8-10+15");

    TS_ASSERT_EQUALS(result.size(), 1)
    TS_ASSERT_EQUALS(result.front().size(), 9)
    TS_ASSERT_EQUALS(result[0][0], 1);
    TS_ASSERT_EQUALS(result[0][1], 2);
    TS_ASSERT_EQUALS(result[0][2], 4);
    TS_ASSERT_EQUALS(result[0][3], 5);
    TS_ASSERT_EQUALS(result[0][4], 6);
    TS_ASSERT_EQUALS(result[0][5], 8);
    TS_ASSERT_EQUALS(result[0][6], 9);
    TS_ASSERT_EQUALS(result[0][7], 10);
    TS_ASSERT_EQUALS(result[0][8], 15);
  }

  void test_multipleAddRangesAndSingle() {
    ParsedRuns result = parseMultiRunString("1-2+4-6+8-10+15");

    TS_ASSERT_EQUALS(result.size(), 1)
    TS_ASSERT_EQUALS(result.front().size(), 9)
    TS_ASSERT_EQUALS(result[0][0], 1);
    TS_ASSERT_EQUALS(result[0][1], 2);
    TS_ASSERT_EQUALS(result[0][2], 4);
    TS_ASSERT_EQUALS(result[0][3], 5);
    TS_ASSERT_EQUALS(result[0][4], 6);
    TS_ASSERT_EQUALS(result[0][5], 8);
    TS_ASSERT_EQUALS(result[0][6], 9);
    TS_ASSERT_EQUALS(result[0][7], 10);
    TS_ASSERT_EQUALS(result[0][8], 15);
  }

  void test_singleAndMultipleAddRanges() {
    ParsedRuns result = parseMultiRunString("1+3-4+6-9+11-12");

    TS_ASSERT_EQUALS(result.size(), 1)
    TS_ASSERT_EQUALS(result.front().size(), 9)
    TS_ASSERT_EQUALS(result[0][0], 1);
    TS_ASSERT_EQUALS(result[0][1], 3);
    TS_ASSERT_EQUALS(result[0][2], 4);
    TS_ASSERT_EQUALS(result[0][3], 6);
    TS_ASSERT_EQUALS(result[0][4], 7);
    TS_ASSERT_EQUALS(result[0][5], 8);
    TS_ASSERT_EQUALS(result[0][6], 9);
    TS_ASSERT_EQUALS(result[0][7], 11);
    TS_ASSERT_EQUALS(result[0][8], 12);
  }

  void test_singleWithinMultipleAddRanges() {
    ParsedRuns result = parseMultiRunString("1-2+4+6-9+11-12");

    TS_ASSERT_EQUALS(result.size(), 1)
    TS_ASSERT_EQUALS(result.front().size(), 9)
    TS_ASSERT_EQUALS(result[0][0], 1);
    TS_ASSERT_EQUALS(result[0][1], 2);
    TS_ASSERT_EQUALS(result[0][2], 4);
    TS_ASSERT_EQUALS(result[0][3], 6);
    TS_ASSERT_EQUALS(result[0][4], 7);
    TS_ASSERT_EQUALS(result[0][5], 8);
    TS_ASSERT_EQUALS(result[0][6], 9);
    TS_ASSERT_EQUALS(result[0][7], 11);
    TS_ASSERT_EQUALS(result[0][8], 12);
  }

  void test_steppedRangeWithinMultipleAddRanges() {
    ParsedRuns result = parseMultiRunString("1-2+4-8:2+10-11");

    TS_ASSERT_EQUALS(result.size(), 1)
    TS_ASSERT_EQUALS(result.front().size(), 7)
    TS_ASSERT_EQUALS(result[0][0], 1);
    TS_ASSERT_EQUALS(result[0][1], 2);
    TS_ASSERT_EQUALS(result[0][2], 4);
    TS_ASSERT_EQUALS(result[0][3], 6);
    TS_ASSERT_EQUALS(result[0][4], 8);
    TS_ASSERT_EQUALS(result[0][5], 10);
    TS_ASSERT_EQUALS(result[0][6], 11);
  }

  void test_allAddRangeVariantsTogether() {
    ParsedRuns result = parseMultiRunString("1+2-3+4-6:1+7+8-9+10");
    TS_ASSERT_EQUALS(result.size(), 1)
    TS_ASSERT_EQUALS(result.front().size(), 10)
    TS_ASSERT_EQUALS(result[0][0], 1);
    TS_ASSERT_EQUALS(result[0][1], 2);
    TS_ASSERT_EQUALS(result[0][2], 3);
    TS_ASSERT_EQUALS(result[0][3], 4);
    TS_ASSERT_EQUALS(result[0][4], 5);
    TS_ASSERT_EQUALS(result[0][5], 6);
    TS_ASSERT_EQUALS(result[0][6], 7);
    TS_ASSERT_EQUALS(result[0][7], 8);
    TS_ASSERT_EQUALS(result[0][8], 9);
    TS_ASSERT_EQUALS(result[0][9], 10);
  }

  void test_errorThrownWhenPassedUnexpectedChar() {
    std::string message =
        "Non-numeric or otherwise unaccetable character(s) detected.";
    TS_ASSERT_THROWS_EQUALS(parseMultiRunString("#"),
                            const std::runtime_error &re,
                            std::string(re.what()), message);
    TS_ASSERT_THROWS_EQUALS(parseMultiRunString("a"),
                            const std::runtime_error &re,
                            std::string(re.what()), message);
    TS_ASSERT_THROWS_EQUALS(parseMultiRunString("Z"),
                            const std::runtime_error &re,
                            std::string(re.what()), message);
    TS_ASSERT_THROWS_EQUALS(parseMultiRunString("("),
                            const std::runtime_error &re,
                            std::string(re.what()), message);
    TS_ASSERT_THROWS_EQUALS(parseMultiRunString(">"),
                            const std::runtime_error &re,
                            std::string(re.what()), message);

    TS_ASSERT_THROWS_EQUALS(parseMultiRunString("1012-n1059:5"),
                            const std::runtime_error &re,
                            std::string(re.what()), message);
  }

  void test_errorThrownOnEmptyToken() {
    TS_ASSERT_THROWS_EQUALS(
        parseMultiRunString("1,,3"), const std::runtime_error &re,
        std::string(re.what()), "A comma-separated token is empty.");
  }

  void test_errorThrownWhenStringDoesNotStartAndEndWithNumeral() {
    TS_ASSERT_THROWS_EQUALS(parseMultiRunString("+2+3"),
                            const std::runtime_error &re,
                            std::string(re.what()),
                            "The token \"+2+3\" is of an incorrect form.  Does "
                            "it begin or end with a plus, minus or colon?");
    TS_ASSERT_THROWS_EQUALS(parseMultiRunString("2-3:"),
                            const std::runtime_error &re,
                            std::string(re.what()),
                            "The token \"2-3:\" is of an incorrect form.  Does "
                            "it begin or end with a plus, minus or colon?");
  }

  void test_errorThrownIfStepSizeEqualsZero() {
    TS_ASSERT_THROWS_EQUALS(
        parseMultiRunString("1:3:0"), const std::runtime_error &re,
        std::string(re.what()),
        "Unable to generate a range with a step size of zero.");
  }

  void test_errorThrownIfAddedStepSizeEqualsZero() {
    TS_ASSERT_THROWS_EQUALS(
        parseMultiRunString("1-3:0"), const std::runtime_error &re,
        std::string(re.what()),
        "Unable to generate a range with a step size of zero.");
  }

  void test_errorThrownIfOfIncorrectForm() {
    TS_ASSERT_THROWS_EQUALS(
        parseMultiRunString("1-3-1"), const std::runtime_error &re,
        std::string(re.what()), "The token \"1-3-1\" is of an incorrect form.");
  }

  /////////////////////////////////////////////////////////////////////////////
  // Testing of Parse class.
  /////////////////////////////////////////////////////////////////////////////

  void test_complexFileNameString() {
    Parser parser;

    parser.parse("TSC10-15, 30:40:2, 50+51+52.raw");

    TS_ASSERT_EQUALS(parser.dirString(), "");
    TS_ASSERT_EQUALS(parser.instString(), "TSC");
    TS_ASSERT_EQUALS(parser.underscoreString(), "");
    TS_ASSERT_EQUALS(parser.runString(), "10-15, 30:40:2, 50+51+52");
    TS_ASSERT_EQUALS(parser.extString(), ".raw");

    std::vector<std::vector<std::string>> filenames = parser.fileNames();

    TS_ASSERT_EQUALS(filenames[0][0], "TSC00010.raw");
    TS_ASSERT_EQUALS(filenames[0][1], "TSC00011.raw");
    TS_ASSERT_EQUALS(filenames[0][2], "TSC00012.raw");
    TS_ASSERT_EQUALS(filenames[0][3], "TSC00013.raw");
    TS_ASSERT_EQUALS(filenames[0][4], "TSC00014.raw");
    TS_ASSERT_EQUALS(filenames[0][5], "TSC00015.raw");

    TS_ASSERT_EQUALS(filenames[1][0], "TSC00030.raw");
    TS_ASSERT_EQUALS(filenames[2][0], "TSC00032.raw");
    TS_ASSERT_EQUALS(filenames[3][0], "TSC00034.raw");
    TS_ASSERT_EQUALS(filenames[4][0], "TSC00036.raw");
    TS_ASSERT_EQUALS(filenames[5][0], "TSC00038.raw");
    TS_ASSERT_EQUALS(filenames[6][0], "TSC00040.raw");

    TS_ASSERT_EQUALS(filenames[7][0], "TSC00050.raw");
    TS_ASSERT_EQUALS(filenames[7][1], "TSC00051.raw");
    TS_ASSERT_EQUALS(filenames[7][2], "TSC00052.raw");
  }

  void test_errorThrownIfPassedEmptyString() {
    Parser parser;

    TS_ASSERT_THROWS_EQUALS(parser.parse(""), const std::runtime_error &re,
                            std::string(re.what()), "No file name to parse.");
  }

  void test_defaultInstrumentUsedIfPassedNoInstrumentName() {
    Parser parser;

    Mantid::Kernel::ConfigService::Instance().setString("default.instrument",
                                                        "TSC");

    parser.parse("c:/2:4.raw");

    TS_ASSERT_EQUALS(parser.dirString(), "c:/");
    TS_ASSERT_EQUALS(parser.instString(), "TSC");
    TS_ASSERT_EQUALS(parser.underscoreString(), "");
    TS_ASSERT_EQUALS(parser.runString(), "2:4");
    TS_ASSERT_EQUALS(parser.extString(), ".raw");

    std::vector<std::vector<std::string>> filenames = parser.fileNames();

    TS_ASSERT_EQUALS(filenames[0][0], "c:/TSC00002.raw");
    TS_ASSERT_EQUALS(filenames[1][0], "c:/TSC00003.raw");
    TS_ASSERT_EQUALS(filenames[2][0], "c:/TSC00004.raw");
  }

  void test_complexFileNameString_SNS() {
    Parser parser;

    parser.parse("CNCS10-15, 30:40:2, 50+51+52.nxs");

    TS_ASSERT_EQUALS(parser.dirString(), "");
    TS_ASSERT_EQUALS(parser.instString(), "CNCS");
    TS_ASSERT_EQUALS(parser.underscoreString(), "");
    TS_ASSERT_EQUALS(parser.runString(), "10-15, 30:40:2, 50+51+52");
    TS_ASSERT_EQUALS(parser.extString(), ".nxs");

    std::vector<std::vector<std::string>> filenames = parser.fileNames();

    TS_ASSERT_EQUALS(filenames[0][0], "CNCS_10.nxs");
    TS_ASSERT_EQUALS(filenames[0][1], "CNCS_11.nxs");
    TS_ASSERT_EQUALS(filenames[0][2], "CNCS_12.nxs");
    TS_ASSERT_EQUALS(filenames[0][3], "CNCS_13.nxs");
    TS_ASSERT_EQUALS(filenames[0][4], "CNCS_14.nxs");
    TS_ASSERT_EQUALS(filenames[0][5], "CNCS_15.nxs");

    TS_ASSERT_EQUALS(filenames[1][0], "CNCS_30.nxs");
    TS_ASSERT_EQUALS(filenames[2][0], "CNCS_32.nxs");
    TS_ASSERT_EQUALS(filenames[3][0], "CNCS_34.nxs");
    TS_ASSERT_EQUALS(filenames[4][0], "CNCS_36.nxs");
    TS_ASSERT_EQUALS(filenames[5][0], "CNCS_38.nxs");
    TS_ASSERT_EQUALS(filenames[6][0], "CNCS_40.nxs");

    TS_ASSERT_EQUALS(filenames[7][0], "CNCS_50.nxs");
    TS_ASSERT_EQUALS(filenames[7][1], "CNCS_51.nxs");
    TS_ASSERT_EQUALS(filenames[7][2], "CNCS_52.nxs");
  }

  void test_instrument_with_multiple_padding() {
    FacilityHelper::ScopedFacilities loadTESTFacility(
        "unit_testing/UnitTestFacilities.xml", "TEST");

    Parser parser;
    parser.parse("TESTHISTOLISTENER123,299-301");

    std::vector<std::vector<std::string>> filenames = parser.fileNames();

    TS_ASSERT_EQUALS(filenames[0][0], "TESTHISTOLISTENER00000123");
    TS_ASSERT_EQUALS(filenames[1][0], "TESTHISTOLISTENER00000299");
    TS_ASSERT_EQUALS(filenames[1][1], "TST00000000300");
    TS_ASSERT_EQUALS(filenames[1][2], "TST00000000301");
  }
};

#endif /* MANTID_KERNEL_MULTIFILENAMEPARSERTEST_H_ */
