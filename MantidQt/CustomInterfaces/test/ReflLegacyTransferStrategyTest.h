#ifndef MANTID_CUSTOMINTERFACES_REFLLEGACYTRANSFERSTRATEGYTEST_H
#define MANTID_CUSTOMINTERFACES_REFLLEGACYTRANSFERSTRATEGYTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <map>
#include <string>
#include <vector>

#include "MantidQtCustomInterfaces/Reflectometry/ReflLegacyTransferStrategy.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflTableSchema.h"
#include "ReflMockObjects.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

class ReflLegacyTransferStrategyTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflLegacyTransferStrategyTest *createSuite() {
    return new ReflLegacyTransferStrategyTest();
  }
  static void destroySuite(ReflLegacyTransferStrategyTest *suite) {
    delete suite;
  }

  ReflLegacyTransferStrategyTest() {}

  void testBasicTransfer() {
    SearchResultMap input;
    input["1234"] = SearchResult("fictitious run on gold", "");
    input["1235"] = SearchResult("fictitious run on silver", "");
    input["1236"] = SearchResult("fictitious run on bronze", "");

    std::vector<std::map<std::string, std::string>> expected;
    std::map<std::string, std::string> expectedRow;

    expectedRow[ReflTableSchema::RUNS] = "1234";
    expectedRow[ReflTableSchema::ANGLE] = "";
    expectedRow[ReflTableSchema::GROUP] = "fictitious run on gold";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1235";
    expectedRow[ReflTableSchema::ANGLE] = "";
    expectedRow[ReflTableSchema::GROUP] = "fictitious run on silver";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1236";
    expectedRow[ReflTableSchema::ANGLE] = "";
    expectedRow[ReflTableSchema::GROUP] = "fictitious run on bronze";
    expected.push_back(expectedRow);

    std::sort(expected.begin(), expected.end());

    ReflLegacyTransferStrategy strategy;

    MockProgressBase progress;
    EXPECT_CALL(progress, doReport(_)).Times(AtLeast(1));

    auto transferRuns = strategy.transferRuns(input, progress);
    auto output = transferRuns.getTransferRuns();
    TS_ASSERT_EQUALS(output, expected);
    TS_ASSERT(Mock::VerifyAndClear(&progress));
  }

  void testGroupedTransfer() {
    SearchResultMap input;
    input["1233"] = SearchResult("fictitious run on platinum", "");
    input["1234"] = SearchResult("fictitious run on gold", "");
    input["1235"] = SearchResult("fictitious run on gold", "");
    input["1236"] = SearchResult("fictitious run on silver", "");

    std::vector<std::map<std::string, std::string>> expected;
    std::map<std::string, std::string> expectedRow;

    expectedRow[ReflTableSchema::RUNS] = "1233";
    expectedRow[ReflTableSchema::ANGLE] = "";
    expectedRow[ReflTableSchema::GROUP] = "fictitious run on platinum";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1234+1235";
    expectedRow[ReflTableSchema::ANGLE] = "";
    expectedRow[ReflTableSchema::GROUP] = "fictitious run on gold";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1236";
    expectedRow[ReflTableSchema::ANGLE] = "";
    expectedRow[ReflTableSchema::GROUP] = "fictitious run on silver";
    expected.push_back(expectedRow);

    std::sort(expected.begin(), expected.end());

    ReflLegacyTransferStrategy strategy;

    MockProgressBase progress;
    EXPECT_CALL(progress, doReport(_)).Times(AtLeast(1));

    auto transferRuns = strategy.transferRuns(input, progress);
    auto output = transferRuns.getTransferRuns();

    TS_ASSERT_EQUALS(output, expected);
    TS_ASSERT(Mock::VerifyAndClear(&progress));
  }

  void testThetaExtraction() {
    SearchResultMap input;
    input["1234"] = SearchResult("fictitious run on gold", "");
    input["1235"] = SearchResult("fictitious run on silver in 3.14 theta", "");
    input["1236"] = SearchResult("fictitious run on bronze th=2.17", "");
    input["1237"] =
        SearchResult("fictitious run on platinum th:1.23 and pH=12", "");

    std::vector<std::map<std::string, std::string>> expected;
    std::map<std::string, std::string> expectedRow;

    expectedRow[ReflTableSchema::RUNS] = "1234";
    expectedRow[ReflTableSchema::ANGLE] = "";
    expectedRow[ReflTableSchema::GROUP] = "fictitious run on gold";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1235";
    expectedRow[ReflTableSchema::ANGLE] = "3.14";
    expectedRow[ReflTableSchema::GROUP] = "fictitious run on silver in 3.14";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1236";
    expectedRow[ReflTableSchema::ANGLE] = "2.17";
    expectedRow[ReflTableSchema::GROUP] = "fictitious run on bronze";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1237";
    expectedRow[ReflTableSchema::ANGLE] = "1.23";
    expectedRow[ReflTableSchema::GROUP] = "fictitious run on platinum";
    expected.push_back(expectedRow);

    std::sort(expected.begin(), expected.end());

    ReflLegacyTransferStrategy strategy;

    MockProgressBase progress;
    EXPECT_CALL(progress, doReport(_)).Times(AtLeast(1));

    auto transferRuns = strategy.transferRuns(input, progress);
    auto output = transferRuns.getTransferRuns();

    TS_ASSERT_EQUALS(output, expected);
    TS_ASSERT(Mock::VerifyAndClear(&progress));
  }

  void testComplexExtraction() {
    SearchResultMap input;
    input["1230"] = SearchResult("fictitious run on gold", "");
    input["1231"] = SearchResult("fictitious run on silver in 3.14 theta", "");
    input["1232"] = SearchResult("fictitious run on silver in 3.14 theta", "");
    input["1233"] = SearchResult("fictitious run on silver in 2.17 theta", "");
    input["1234"] = SearchResult("fictitious run on bronze th=2.17", "");
    input["1235"] = SearchResult("fictitious run on bronze th=1.23", "");
    input["1236"] =
        SearchResult("fictitious run on platinum th:1.23 and pH=12", "");
    input["1237"] = SearchResult("fictitious run on fool's gold", "");

    std::vector<std::map<std::string, std::string>> expected;
    std::map<std::string, std::string> expectedRow;

    expectedRow[ReflTableSchema::RUNS] = "1230";
    expectedRow[ReflTableSchema::ANGLE] = "";
    expectedRow[ReflTableSchema::GROUP] = "fictitious run on gold";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1231+1232";
    expectedRow[ReflTableSchema::ANGLE] = "3.14";
    expectedRow[ReflTableSchema::GROUP] = "fictitious run on silver in 3.14";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1233";
    expectedRow[ReflTableSchema::ANGLE] = "2.17";
    expectedRow[ReflTableSchema::GROUP] = "fictitious run on silver in 3.14";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1234";
    expectedRow[ReflTableSchema::ANGLE] = "2.17";
    expectedRow[ReflTableSchema::GROUP] = "fictitious run on bronze";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1235";
    expectedRow[ReflTableSchema::ANGLE] = "1.23";
    expectedRow[ReflTableSchema::GROUP] = "fictitious run on bronze";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1236";
    expectedRow[ReflTableSchema::ANGLE] = "1.23";
    expectedRow[ReflTableSchema::GROUP] = "fictitious run on platinum";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1237";
    expectedRow[ReflTableSchema::ANGLE] = "";
    expectedRow[ReflTableSchema::GROUP] = "fictitious run on fool's gold";
    expected.push_back(expectedRow);

    std::sort(expected.begin(), expected.end());

    ReflLegacyTransferStrategy strategy;

    MockProgressBase progress;
    EXPECT_CALL(progress, doReport(_)).Times(AtLeast(1));

    auto transferRuns = strategy.transferRuns(input, progress);
    auto output = transferRuns.getTransferRuns();

    TS_ASSERT_EQUALS(output, expected);
    TS_ASSERT(Mock::VerifyAndClear(&progress));
  }

  void test_clone() {
    ReflLegacyTransferStrategy strategy;
    auto clone = strategy.clone();
    TS_ASSERT(dynamic_cast<ReflLegacyTransferStrategy *>(clone.get()));
  }

  void test_filtering() {
    ReflLegacyTransferStrategy strategy;

    // Raw file where written with the description information needed for this
    // sort of transfer
    TSM_ASSERT("Yes this transfer mechanism should know about raw formats",
               strategy.knownFileType("madeup.raw"));

    // Nexus file may not have this description information.
    TSM_ASSERT(
        "No this transfer mechanism should know about anything but raw formats",
        !strategy.knownFileType("madeup.nxs"));
  }
};

#endif
