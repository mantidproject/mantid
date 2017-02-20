#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORPREPROCESSMAPTEST_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORPREPROCESSMAPTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPreprocessMap.h"

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class DataProcessorPreprocessMapTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataProcessorPreprocessMapTest *createSuite() {
    return new DataProcessorPreprocessMapTest();
  }
  static void destroySuite(DataProcessorPreprocessMapTest *suite) {
    delete suite;
  }
  DataProcessorPreprocessMapTest() { FrameworkManager::Instance(); };

  void test_add_element() {
    DataProcessorPreprocessMap preprocessMap;
    preprocessMap.addElement("Runs", "Plus");
    preprocessMap.addElement("Transmission Runs",
                             "CreateTransmissionWorkspaceAuto", "TRANS_",
                             "FirstTransmissionRun,SecondTransmissionRun");

    auto preprocessingInstructions = preprocessMap.asMap();

    DataProcessorPreprocessingAlgorithm algPlus =
        preprocessingInstructions["Runs"];
    TS_ASSERT_EQUALS(algPlus.name(), "Plus");
    TS_ASSERT_EQUALS(algPlus.prefix(), "");
    TS_ASSERT_EQUALS(algPlus.blacklist(), std::set<std::string>());

    DataProcessorPreprocessingAlgorithm algTrans =
        preprocessingInstructions["Transmission Runs"];
    TS_ASSERT_EQUALS(algTrans.name(), "CreateTransmissionWorkspaceAuto");
    TS_ASSERT_EQUALS(algTrans.prefix(), "TRANS_");
    std::set<std::string> blacklist = {"FirstTransmissionRun",
                                       "SecondTransmissionRun"};
    TS_ASSERT_EQUALS(algTrans.blacklist(), blacklist);
  }
};
#endif /* MANTID_MANTIDWIDGETS_DATAPROCESSORPREPROCESSMAPTEST_H */
