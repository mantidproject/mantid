#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORWHITELISTTEST_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORWHITELISTTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorWhiteList.h"

using namespace MantidQt::MantidWidgets;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class DataProcessorWhiteListTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataProcessorWhiteListTest *createSuite() {
    return new DataProcessorWhiteListTest();
  }
  static void destroySuite(DataProcessorWhiteListTest *suite) { delete suite; }

  void test_whitelist() {
    DataProcessorWhiteList whitelist;
    whitelist.addElement("Column1", "Property1", "Description1");
    whitelist.addElement("Column2", "Property2", "Description2");
    whitelist.addElement("Column3", "Property3", "Description3");
    whitelist.addElement("Column4", "Property4", "Description4");
    whitelist.addElement("Column5", "Property5", "Description5");
    // Not much to test, just make sure that columns are inserted in the right
    // order
    TS_ASSERT_EQUALS(whitelist.size(), 5);
    // Column indices
    TS_ASSERT_EQUALS(whitelist.colIndexFromColName("Column1"), 0);
    TS_ASSERT_EQUALS(whitelist.colIndexFromColName("Column3"), 2);
    TS_ASSERT_EQUALS(whitelist.colIndexFromColName("Column5"), 4);
    // Algorithm properties
    TS_ASSERT_EQUALS(whitelist.algPropFromColIndex(1), "Property2");
    TS_ASSERT_EQUALS(whitelist.algPropFromColIndex(3), "Property4");
    // Descriptions
    TS_ASSERT_EQUALS(whitelist.description(2), "Description3");
    TS_ASSERT_EQUALS(whitelist.description(4), "Description5");
  }
};
#endif /* MANTID_MANTIDWIDGETS_DATAPROCESSORWHITELISTTEST_H */
