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

  void test_column_index() {
    DataProcessorWhiteList whitelist;
    whitelist.addElement("Column1", "Property1", "Description1");
    whitelist.addElement("Column2", "Property2", "Description2");
    whitelist.addElement("Column3", "Property3", "Description3");
    whitelist.addElement("Column4", "Property4", "Description4");
    whitelist.addElement("Column5", "Property5", "Description5");

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

  void test_column_name() {
    DataProcessorWhiteList whitelist;
    whitelist.addElement("Column1", "Property1", "Description1");
    whitelist.addElement("Column2", "Property2", "Description2");
    whitelist.addElement("Column3", "Property3", "Description3");
    whitelist.addElement("Column4", "Property4", "Description4");
    whitelist.addElement("Column5", "Property5", "Description5");

    TS_ASSERT_EQUALS(whitelist.size(), 5);
    // Column indices
    TS_ASSERT_EQUALS(whitelist.colNameFromColIndex(0), "Column1");
    TS_ASSERT_EQUALS(whitelist.colNameFromColIndex(3), "Column4");
    TS_ASSERT_EQUALS(whitelist.colNameFromColIndex(4), "Column5");
  }

  void test_column_property() {
    DataProcessorWhiteList whitelist;
    whitelist.addElement("Column1", "Property1", "Description1");
    whitelist.addElement("Column2", "Property2", "Description2");
    whitelist.addElement("Column3", "Property3", "Description3");
    whitelist.addElement("Column4", "Property4", "Description4");
    whitelist.addElement("Column5", "Property5", "Description5");

    TS_ASSERT_EQUALS(whitelist.size(), 5);
    // Algorithm properties
    TS_ASSERT_EQUALS(whitelist.algPropFromColIndex(1), "Property2");
    TS_ASSERT_EQUALS(whitelist.algPropFromColIndex(3), "Property4");
  }

  void test_column_description() {
    DataProcessorWhiteList whitelist;
    whitelist.addElement("Column1", "Property1", "Description1");
    whitelist.addElement("Column2", "Property2", "Description2");
    whitelist.addElement("Column3", "Property3", "Description3");
    whitelist.addElement("Column4", "Property4", "Description4");
    whitelist.addElement("Column5", "Property5", "Description5");

    TS_ASSERT_EQUALS(whitelist.size(), 5);
    // Descriptions
    TS_ASSERT_EQUALS(whitelist.description(0), "Description1");
    TS_ASSERT_EQUALS(whitelist.description(2), "Description3");
    TS_ASSERT_EQUALS(whitelist.description(4), "Description5");
  }

  void test_column_showValue() {
    DataProcessorWhiteList whitelist;
    whitelist.addElement("Column1", "Property1", "Description1");
    whitelist.addElement("Column3", "Property3", "Description3", true);

    TS_ASSERT_EQUALS(whitelist.size(), 2);
    // Descriptions
    TS_ASSERT_EQUALS(whitelist.showValue(0), false);
    TS_ASSERT_EQUALS(whitelist.showValue(1), true);
  }

  void test_column_prefix() {
    DataProcessorWhiteList whitelist;
    whitelist.addElement("Column1", "Property1", "Description1");
    whitelist.addElement("Column3", "Property3", "Description3", true, "blah");

    TS_ASSERT_EQUALS(whitelist.size(), 2);
    // Descriptions
    TS_ASSERT_EQUALS(whitelist.prefix(0), "");
    TS_ASSERT_EQUALS(whitelist.prefix(1), "blah");
  }
};
#endif /* MANTID_MANTIDWIDGETS_DATAPROCESSORWHITELISTTEST_H */
