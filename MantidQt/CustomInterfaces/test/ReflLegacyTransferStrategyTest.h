#ifndef MANTID_CUSTOMINTERFACES_REFLLEGACYTRANSFERSTRATEGYTEST_H
#define MANTID_CUSTOMINTERFACES_REFLLEGACYTRANSFERSTRATEGYTEST_H

#include <cxxtest/TestSuite.h>
#include <map>
#include <string>
#include <vector>
#include <gmock/gmock.h>

#include "MantidQtCustomInterfaces/ReflLegacyTransferStrategy.h"
#include "MantidQtCustomInterfaces/ReflTableSchema.h"
#include "ReflMainViewMockObjects.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

class ReflLegacyTransferStrategyTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflLegacyTransferStrategyTest *createSuite() { return new ReflLegacyTransferStrategyTest(); }
  static void destroySuite( ReflLegacyTransferStrategyTest *suite ) { delete suite; }

  ReflLegacyTransferStrategyTest()
  {
  }

  void testBasicTransfer()
  {
    std::map<std::string,std::string> input;
    input["1234"] = "fictitious run on gold";
    input["1235"] = "fictitious run on silver";
    input["1236"] = "fictitious run on bronze";

    std::vector<std::map<std::string,std::string> > expected;
    std::map<std::string,std::string> expectedRow;

    expectedRow[ReflTableSchema::RUNS] = "1234";
    expectedRow[ReflTableSchema::ANGLE] = "";
    expectedRow[ReflTableSchema::GROUP] = "0";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1235";
    expectedRow[ReflTableSchema::ANGLE] = "";
    expectedRow[ReflTableSchema::GROUP] = "1";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1236";
    expectedRow[ReflTableSchema::ANGLE] = "";
    expectedRow[ReflTableSchema::GROUP] = "2";
    expected.push_back(expectedRow);

    ReflLegacyTransferStrategy strategy;

    MockProgressBase progress;
    EXPECT_CALL(progress, doReport(_)).Times(AtLeast(1));

    auto output = strategy.transferRuns(input, progress);

    TS_ASSERT_EQUALS(output, expected);
    TS_ASSERT(Mock::VerifyAndClear(&progress));
  }

  void testGroupedTransfer()
  {
    std::map<std::string,std::string> input;
    input["1233"] = "fictitious run on platinum";
    input["1234"] = "fictitious run on gold";
    input["1235"] = "fictitious run on gold";
    input["1236"] = "fictitious run on silver";

    std::vector<std::map<std::string,std::string> > expected;
    std::map<std::string,std::string> expectedRow;

    expectedRow[ReflTableSchema::RUNS] = "1233";
    expectedRow[ReflTableSchema::ANGLE] = "";
    expectedRow[ReflTableSchema::GROUP] = "0";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1234+1235";
    expectedRow[ReflTableSchema::ANGLE] = "";
    expectedRow[ReflTableSchema::GROUP] = "1";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1236";
    expectedRow[ReflTableSchema::ANGLE] = "";
    expectedRow[ReflTableSchema::GROUP] = "2";
    expected.push_back(expectedRow);

    ReflLegacyTransferStrategy strategy;

    MockProgressBase progress;
    EXPECT_CALL(progress, doReport(_)).Times(AtLeast(1));

    auto output = strategy.transferRuns(input, progress);

    TS_ASSERT_EQUALS(output, expected);
    TS_ASSERT(Mock::VerifyAndClear(&progress));
  }

  void testThetaExtraction()
  {
    std::map<std::string,std::string> input;
    input["1234"] = "fictitious run on gold";
    input["1235"] = "fictitious run on silver in 3.14 theta";
    input["1236"] = "fictitious run on bronze th=2.17";
    input["1237"] = "fictitious run on platinum th:1.23 and pH=12";

    std::vector<std::map<std::string,std::string> > expected;
    std::map<std::string,std::string> expectedRow;

    expectedRow[ReflTableSchema::RUNS] = "1234";
    expectedRow[ReflTableSchema::ANGLE] = "";
    expectedRow[ReflTableSchema::GROUP] = "0";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1235";
    expectedRow[ReflTableSchema::ANGLE] = "3.14";
    expectedRow[ReflTableSchema::GROUP] = "1";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1236";
    expectedRow[ReflTableSchema::ANGLE] = "2.17";
    expectedRow[ReflTableSchema::GROUP] = "2";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1237";
    expectedRow[ReflTableSchema::ANGLE] = "1.23";
    expectedRow[ReflTableSchema::GROUP] = "3";
    expected.push_back(expectedRow);

    std::sort(expected.begin(), expected.end());

    ReflLegacyTransferStrategy strategy;

    MockProgressBase progress;
    EXPECT_CALL(progress, doReport(_)).Times(AtLeast(1));

    auto output = strategy.transferRuns(input, progress);

    TS_ASSERT_EQUALS(output, expected);
    TS_ASSERT(Mock::VerifyAndClear(&progress));
  }

  void testComplexExtraction()
  {
    std::map<std::string,std::string> input;
    input["1230"] = "fictitious run on gold";
    input["1231"] = "fictitious run on silver in 3.14 theta";
    input["1232"] = "fictitious run on silver in 3.14 theta";
    input["1233"] = "fictitious run on silver in 2.17 theta";
    input["1234"] = "fictitious run on bronze th=2.17";
    input["1235"] = "fictitious run on bronze th=1.23";
    input["1236"] = "fictitious run on platinum th:1.23 and pH=12";
    input["1237"] = "fictitious run on fool's gold";

    std::vector<std::map<std::string,std::string> > expected;
    std::map<std::string,std::string> expectedRow;

    expectedRow[ReflTableSchema::RUNS] = "1230";
    expectedRow[ReflTableSchema::ANGLE] = "";
    expectedRow[ReflTableSchema::GROUP] = "0";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1231+1232";
    expectedRow[ReflTableSchema::ANGLE] = "3.14";
    expectedRow[ReflTableSchema::GROUP] = "1";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1233";
    expectedRow[ReflTableSchema::ANGLE] = "2.17";
    expectedRow[ReflTableSchema::GROUP] = "1";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1234";
    expectedRow[ReflTableSchema::ANGLE] = "2.17";
    expectedRow[ReflTableSchema::GROUP] = "2";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1235";
    expectedRow[ReflTableSchema::ANGLE] = "1.23";
    expectedRow[ReflTableSchema::GROUP] = "2";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1236";
    expectedRow[ReflTableSchema::ANGLE] = "1.23";
    expectedRow[ReflTableSchema::GROUP] = "3";
    expected.push_back(expectedRow);

    expectedRow[ReflTableSchema::RUNS] = "1237";
    expectedRow[ReflTableSchema::ANGLE] = "";
    expectedRow[ReflTableSchema::GROUP] = "4";
    expected.push_back(expectedRow);

    std::sort(expected.begin(), expected.end());

    ReflLegacyTransferStrategy strategy;

    MockProgressBase progress;
    EXPECT_CALL(progress, doReport(_)).Times(AtLeast(1));

    auto output = strategy.transferRuns(input, progress);

    TS_ASSERT_EQUALS(output, expected);
    TS_ASSERT(Mock::VerifyAndClear(&progress));
  }
};

#endif
