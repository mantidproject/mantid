#ifndef MANTID_CUSTOMINTERFACES_REFLLEGACYTRANSFERSTRATEGYTEST_H
#define MANTID_CUSTOMINTERFACES_REFLLEGACYTRANSFERSTRATEGYTEST_H

#include <cxxtest/TestSuite.h>
#include <map>
#include <string>
#include <vector>

#include "MantidQtCustomInterfaces/ReflLegacyTransferStrategy.h"

using namespace MantidQt::CustomInterfaces;

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

    expectedRow["runs"] = "1234";
    expectedRow["theta"] = "";
    expectedRow["group"] = "0";
    expected.push_back(expectedRow);

    expectedRow["runs"] = "1235";
    expectedRow["theta"] = "";
    expectedRow["group"] = "1";
    expected.push_back(expectedRow);

    expectedRow["runs"] = "1236";
    expectedRow["theta"] = "";
    expectedRow["group"] = "2";
    expected.push_back(expectedRow);

    ReflLegacyTransferStrategy strategy;
    auto output = strategy.transferRuns(input);

    TS_ASSERT_EQUALS(output, expected);
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

    expectedRow["runs"] = "1233";
    expectedRow["theta"] = "";
    expectedRow["group"] = "0";
    expected.push_back(expectedRow);

    expectedRow["runs"] = "1234+1235";
    expectedRow["theta"] = "";
    expectedRow["group"] = "1";
    expected.push_back(expectedRow);

    expectedRow["runs"] = "1236";
    expectedRow["theta"] = "";
    expectedRow["group"] = "2";
    expected.push_back(expectedRow);

    ReflLegacyTransferStrategy strategy;
    auto output = strategy.transferRuns(input);

    TS_ASSERT_EQUALS(output, expected);
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

    expectedRow["runs"] = "1234";
    expectedRow["theta"] = "";
    expectedRow["group"] = "0";
    expected.push_back(expectedRow);

    expectedRow["runs"] = "1235";
    expectedRow["theta"] = "3.14";
    expectedRow["group"] = "1";
    expected.push_back(expectedRow);

    expectedRow["runs"] = "1236";
    expectedRow["theta"] = "2.17";
    expectedRow["group"] = "2";
    expected.push_back(expectedRow);

    expectedRow["runs"] = "1237";
    expectedRow["theta"] = "1.23";
    expectedRow["group"] = "3";
    expected.push_back(expectedRow);

    ReflLegacyTransferStrategy strategy;
    auto output = strategy.transferRuns(input);

    TS_ASSERT_EQUALS(output, expected);
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

    expectedRow["runs"] = "1230";
    expectedRow["theta"] = "";
    expectedRow["group"] = "0";
    expected.push_back(expectedRow);

    expectedRow["runs"] = "1231+1232";
    expectedRow["theta"] = "3.14";
    expectedRow["group"] = "1";
    expected.push_back(expectedRow);

    expectedRow["runs"] = "1233";
    expectedRow["theta"] = "2.17";
    expectedRow["group"] = "1";
    expected.push_back(expectedRow);

    expectedRow["runs"] = "1234";
    expectedRow["theta"] = "2.17";
    expectedRow["group"] = "2";
    expected.push_back(expectedRow);

    expectedRow["runs"] = "1235";
    expectedRow["theta"] = "1.23";
    expectedRow["group"] = "2";
    expected.push_back(expectedRow);

    expectedRow["runs"] = "1236";
    expectedRow["theta"] = "1.23";
    expectedRow["group"] = "3";
    expected.push_back(expectedRow);

    expectedRow["runs"] = "1237";
    expectedRow["theta"] = "";
    expectedRow["group"] = "4";
    expected.push_back(expectedRow);

    ReflLegacyTransferStrategy strategy;
    auto output = strategy.transferRuns(input);

    TS_ASSERT_EQUALS(output, expected);
  }
};

#endif
