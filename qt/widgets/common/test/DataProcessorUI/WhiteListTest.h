#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORWHITELISTTEST_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORWHITELISTTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtWidgets/Common/DataProcessorUI/WhiteList.h"

using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::DataProcessor;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class WhiteListTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WhiteListTest *createSuite() { return new WhiteListTest(); }
  static void destroySuite(WhiteListTest *suite) { delete suite; }

  WhiteList makeTestWhiteList() {
    auto whitelist = WhiteList();
    whitelist.addElement("Column1", "Property1", "Description1");
    whitelist.addElement("Column2", "Property2", "Description2");
    whitelist.addElement("Column3", "Property3", "Description3");
    whitelist.addElement("Column4", "Property4", "Description4");
    whitelist.addElement("Column5", "Property5", "Description5");
    return whitelist;
  }

  void test_column_index() {
    auto whitelist = makeTestWhiteList();

    TS_ASSERT_EQUALS(whitelist.size(), 5);
    // Column indices
    TS_ASSERT_EQUALS(whitelist.indexFromName("Column1"), 0);
    TS_ASSERT_EQUALS(whitelist.indexFromName("Column3"), 2);
    TS_ASSERT_EQUALS(whitelist.indexFromName("Column5"), 4);
    // Algorithm properties
    TS_ASSERT_EQUALS(whitelist.algorithmProperty(1), "Property2");
    TS_ASSERT_EQUALS(whitelist.algorithmProperty(3), "Property4");
    // Descriptions
    TS_ASSERT_EQUALS(whitelist.description(2), "Description3");
    TS_ASSERT_EQUALS(whitelist.description(4), "Description5");
  }

  void test_column_name() {
    auto whitelist = makeTestWhiteList();

    TS_ASSERT_EQUALS(whitelist.size(), 5);
    // Column indices
    TS_ASSERT_EQUALS(whitelist.name(0), "Column1");
    TS_ASSERT_EQUALS(whitelist.name(3), "Column4");
    TS_ASSERT_EQUALS(whitelist.name(4), "Column5");
  }

  void test_column_iterator() {
    auto whitelist = makeTestWhiteList();

    TS_ASSERT_EQUALS(whitelist.size(), 5);
    // Column indices
    auto it = whitelist.begin();
    TS_ASSERT_EQUALS((*it).name(), "Column1");
    it += 3;
    TS_ASSERT_EQUALS((*it).name(), "Column4");
    ++it;
    TS_ASSERT_EQUALS((*it).name(), "Column5");
    ++it;
    TS_ASSERT_EQUALS(it, whitelist.end());
  }

  void test_column_property() {
    auto whitelist = makeTestWhiteList();
    TS_ASSERT_EQUALS(whitelist.size(), 5);
    // Algorithm properties
    TS_ASSERT_EQUALS(whitelist.algorithmProperty(1), "Property2");
    TS_ASSERT_EQUALS(whitelist.algorithmProperty(3), "Property4");
  }

  void test_column_description() {
    auto whitelist = makeTestWhiteList();
    TS_ASSERT_EQUALS(whitelist.size(), 5);
    // Descriptions
    TS_ASSERT_EQUALS(whitelist.description(0), "Description1");
    TS_ASSERT_EQUALS(whitelist.description(2), "Description3");
    TS_ASSERT_EQUALS(whitelist.description(4), "Description5");
  }

  void test_column_isShown() {
    WhiteList whitelist;
    whitelist.addElement("Column1", "Property1", "Description1");
    whitelist.addElement("Column3", "Property3", "Description3", true);

    TS_ASSERT_EQUALS(whitelist.size(), 2);
    // Descriptions
    TS_ASSERT_EQUALS(whitelist.isShown(0), false);
    TS_ASSERT_EQUALS(whitelist.isShown(1), true);
  }

  void test_column_isKey() {
    WhiteList whitelist;
    whitelist.addElement("Column1", "Property1", "Description1");
    whitelist.addElement("Column3", "Property3", "Description3", true);

    TS_ASSERT_EQUALS(whitelist.size(), 2);
    // Descriptions
    TS_ASSERT_EQUALS(whitelist.isKey(0), false);
    TS_ASSERT_EQUALS(whitelist.isKey(1), true);
  }

  void test_column_prefix() {
    WhiteList whitelist;
    whitelist.addElement("Column1", "Property1", "Description1");
    whitelist.addElement("Column3", "Property3", "Description3", true, "blah");

    TS_ASSERT_EQUALS(whitelist.size(), 2);
    // Descriptions
    TS_ASSERT_EQUALS(whitelist.prefix(0), "");
    TS_ASSERT_EQUALS(whitelist.prefix(1), "blah");
  }
};
#endif /* MANTID_MANTIDWIDGETS_DATAPROCESSORWHITELISTTEST_H */
