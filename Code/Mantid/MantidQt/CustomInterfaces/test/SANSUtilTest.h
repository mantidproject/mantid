#ifndef MANTID_CUSTOMINTERFACES_SANSUTILITYTEST_H
#define MANTID_CUSTOMINTERFACES_SANSUTILITYTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <MantidQtCustomInterfaces/SANSUtil.h>
#include <QString>

class SANSUtilTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SANSUtilTest *createSuite() { return new SANSUtilTest(); }
  static void destroySuite(SANSUtilTest *suite) { delete suite; }

  void test_python_string_list_created_for_correct_input() {
    // Arrange
    QString var1("test1"), var2("test2"), var3("test43");
    QString delimiter = ",";
    QString input = var1 + delimiter + var2 + delimiter + var3;
    MantidQt::CustomInterfaces::SANSUtil util;
    // Act
    QString stringList = util.createPythonStringList(input, delimiter);

    // Assert
    QString qMark = "'";
    QString expectedString = "[" + qMark + var1 + qMark
                              + delimiter 
                              + qMark + var2 + qMark
                              + delimiter
                              + qMark + var3 + qMark
                          +"]";
    TSM_ASSERT_EQUALS("String list have the form: ['test1','test2','test3'].", expectedString, stringList);
  }

  void test_empty_python_string_list_is_returned_for_empty_input() {
    // Arrange
    QString delimiter = ",";
    QString input;
    MantidQt::CustomInterfaces::SANSUtil util;
    // Act
    QString stringList = util.createPythonStringList(input, delimiter);

    // Assert
    QString qMark = "'";
    QString expectedString = "[]";
    TSM_ASSERT_EQUALS("String list have the form: [].", expectedString, stringList);
  }

  void test_python_string_list_contains_single_entry_for_wrong_delimiter() {
    // Arrange
    QString var1("test1"), var2("test2"), var3("test43");
    QString delimiter = ":";
    QString delimiter_py = ",";
    QString input = var1 + delimiter + var2 + delimiter + var3;
    MantidQt::CustomInterfaces::SANSUtil util;
    // Act
    QString stringList = util.createPythonStringList(input, delimiter_py);

    // Assert
    QString qMark = "'";
    QString expectedString = "["+qMark+input+qMark +"]";
    TSM_ASSERT_EQUALS("String list have the form: ['test1:test2:test3'].", expectedString.toStdString(), stringList.toStdString());
  }

  void test_python_list_ignores_empty_entries_between_delimiters() {
    // Arrange
    QString var1("test1"), var2("test2"), var3("test43");
    QString delimiter = ":";
    QString input = var1 +"  " + delimiter + delimiter + var2 + delimiter + " " + var3;
    MantidQt::CustomInterfaces::SANSUtil util;
    // Act
    QString stringList = util.createPythonStringList(input, delimiter);

    // Assert
    QString qMark = "'";
    QString expectedString = "[" + qMark + var1 + qMark
                              + delimiter 
                              + qMark + var2 + qMark
                              + delimiter
                              + qMark + var3 + qMark
                          +"]";
    TSM_ASSERT_EQUALS("String list have the form: ['test1','test2','test3'].", expectedString, stringList);
  }
};

#endif