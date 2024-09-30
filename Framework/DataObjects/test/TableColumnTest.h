// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/TableColumn.h"
#include "MantidDataObjects/TableWorkspace.h"
#include <cxxtest/TestSuite.h>

#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"

#include <memory>

using namespace Mantid::DataObjects;

class TableColumnTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TableColumnTest *createSuite() { return new TableColumnTest(); }
  static void destroySuite(TableColumnTest *suite) { delete suite; }

  void test_sortIndex() {
    const size_t n = 10;
    TableWorkspace ws(n);
    ws.addColumn("int", "col");
    TableColumn<int> &column = static_cast<TableColumn<int> &>(*ws.getColumn("col"));
    auto &data = column.data();
    data[0] = 5;
    data[1] = 7;
    data[2] = 3;
    data[3] = 12;
    data[4] = 1;
    data[5] = 6;
    data[6] = 3;
    data[7] = 2;
    data[8] = 0;
    data[9] = 12;

    auto indexVec = makeIndexVector(column.size());
    std::vector<std::pair<size_t, size_t>> eqRanges;
    bool ascending = true;
    column.sortIndex(ascending, 0, column.size(), indexVec, eqRanges);

    TS_ASSERT_EQUALS(data[0], 5.0);
    TS_ASSERT_EQUALS(data[1], 7.0);
    TS_ASSERT_EQUALS(data[2], 3.0);
    TS_ASSERT_EQUALS(data[3], 12.0);
    TS_ASSERT_EQUALS(data[4], 1.0);
    TS_ASSERT_EQUALS(data[5], 6.0);
    TS_ASSERT_EQUALS(data[6], 3.0);
    TS_ASSERT_EQUALS(data[7], 2.0);
    TS_ASSERT_EQUALS(data[8], 0.0);
    TS_ASSERT_EQUALS(data[9], 12.0);

    TS_ASSERT_EQUALS(data[indexVec[0]], 0);
    TS_ASSERT_EQUALS(data[indexVec[1]], 1);
    TS_ASSERT_EQUALS(data[indexVec[2]], 2);
    TS_ASSERT_EQUALS(data[indexVec[3]], 3);
    TS_ASSERT_EQUALS(data[indexVec[4]], 3);
    TS_ASSERT_EQUALS(data[indexVec[5]], 5);
    TS_ASSERT_EQUALS(data[indexVec[6]], 6);
    TS_ASSERT_EQUALS(data[indexVec[7]], 7);
    TS_ASSERT_EQUALS(data[indexVec[8]], 12);
    TS_ASSERT_EQUALS(data[indexVec[9]], 12);

    TS_ASSERT_EQUALS(eqRanges.size(), 2);
    TS_ASSERT_EQUALS(eqRanges[0].first, 3);
    TS_ASSERT_EQUALS(eqRanges[0].second, 5);
    TS_ASSERT_EQUALS(eqRanges[1].first, 8);
    TS_ASSERT_EQUALS(eqRanges[1].second, 10);
  }

  void test_sortValues_Ascending() {
    const size_t n = 10;
    TableWorkspace ws(n);
    ws.addColumn("int", "col1");
    ws.addColumn("str", "col2");
    auto column = static_cast<TableColumn<int> &>(*ws.getColumn("col1"));
    auto &data = column.data();
    data[0] = 5;
    data[1] = 7;
    data[2] = 3;
    data[3] = 12;
    data[4] = 1;
    data[5] = 6;
    data[6] = 3;
    data[7] = 2;
    data[8] = 0;
    data[9] = 12;

    auto column2 = static_cast<TableColumn<std::string> &>(*ws.getColumn("col2"));
    auto &data2 = column2.data();
    data2[0] = "five";
    data2[1] = "seven";
    data2[2] = "three (1)";
    data2[3] = "twelve (1)";
    data2[4] = "one";
    data2[5] = "six";
    data2[6] = "three (2)";
    data2[7] = "two";
    data2[8] = "zero";
    data2[9] = "twelve (2)";

    auto indexVec = makeIndexVector(column.size());
    std::vector<std::pair<size_t, size_t>> eqRanges;
    bool ascending = true;
    column.sortIndex(ascending, 0, column.size(), indexVec, eqRanges);

    column.sortValues(indexVec);

    TS_ASSERT_EQUALS(data[0], 0);
    TS_ASSERT_EQUALS(data[1], 1);
    TS_ASSERT_EQUALS(data[2], 2);
    TS_ASSERT_EQUALS(data[3], 3);
    TS_ASSERT_EQUALS(data[4], 3);
    TS_ASSERT_EQUALS(data[5], 5);
    TS_ASSERT_EQUALS(data[6], 6);
    TS_ASSERT_EQUALS(data[7], 7);
    TS_ASSERT_EQUALS(data[8], 12);
    TS_ASSERT_EQUALS(data[9], 12);

    column2.sortValues(indexVec);

    TS_ASSERT_EQUALS(data2[0], "zero");
    TS_ASSERT_EQUALS(data2[1], "one");
    TS_ASSERT_EQUALS(data2[2], "two");
    TS_ASSERT_EQUALS(data2[3], "three (1)");
    TS_ASSERT_EQUALS(data2[4], "three (2)");
    TS_ASSERT_EQUALS(data2[5], "five");
    TS_ASSERT_EQUALS(data2[6], "six");
    TS_ASSERT_EQUALS(data2[7], "seven");
    TS_ASSERT_EQUALS(data2[8], "twelve (1)");
    TS_ASSERT_EQUALS(data2[9], "twelve (2)");
  }

  void test_sortValues_Descending() {
    const size_t n = 10;
    TableWorkspace ws(n);
    ws.addColumn("int", "col1");
    ws.addColumn("str", "col2");
    auto column = static_cast<TableColumn<int> &>(*ws.getColumn("col1"));
    auto &data = column.data();
    data[0] = 5;
    data[1] = 7;
    data[2] = 3;
    data[3] = 12;
    data[4] = 1;
    data[5] = 6;
    data[6] = 3;
    data[7] = 2;
    data[8] = 0;
    data[9] = 12;

    auto column2 = static_cast<TableColumn<std::string> &>(*ws.getColumn("col2"));
    auto &data2 = column2.data();
    data2[0] = "five";
    data2[1] = "seven";
    data2[2] = "three (1)";
    data2[3] = "twelve (1)";
    data2[4] = "one";
    data2[5] = "six";
    data2[6] = "three (2)";
    data2[7] = "two";
    data2[8] = "zero";
    data2[9] = "twelve (2)";

    auto indexVec = makeIndexVector(column.size());
    std::vector<std::pair<size_t, size_t>> eqRanges;
    bool ascending = false;
    column.sortIndex(ascending, 0, column.size(), indexVec, eqRanges);

    column.sortValues(indexVec);

    TS_ASSERT_EQUALS(data[0], 12);
    TS_ASSERT_EQUALS(data[1], 12);
    TS_ASSERT_EQUALS(data[2], 7);
    TS_ASSERT_EQUALS(data[3], 6);
    TS_ASSERT_EQUALS(data[4], 5);
    TS_ASSERT_EQUALS(data[5], 3);
    TS_ASSERT_EQUALS(data[6], 3);
    TS_ASSERT_EQUALS(data[7], 2);
    TS_ASSERT_EQUALS(data[8], 1);
    TS_ASSERT_EQUALS(data[9], 0);

    column2.sortValues(indexVec);

    TS_ASSERT_EQUALS(data2[0], "twelve (1)");
    TS_ASSERT_EQUALS(data2[1], "twelve (2)");
    TS_ASSERT_EQUALS(data2[2], "seven");
    TS_ASSERT_EQUALS(data2[3], "six");
    TS_ASSERT_EQUALS(data2[4], "five");
    TS_ASSERT_EQUALS(data2[5], "three (1)");
    TS_ASSERT_EQUALS(data2[6], "three (2)");
    TS_ASSERT_EQUALS(data2[7], "two");
    TS_ASSERT_EQUALS(data2[8], "one");
    TS_ASSERT_EQUALS(data2[9], "zero");
  }
  void test_clone_table_column() {
    const size_t n = 2;
    TableWorkspace ws(n);
    ws.addColumn("int", "col1");
    ws.addColumn("str", "col2");
    auto clonedColInt = std::unique_ptr<Mantid::API::Column>(ws.getColumn("col1")->clone());
    auto clonedColStr = std::unique_ptr<Mantid::API::Column>(ws.getColumn("col2")->clone());
    TS_ASSERT_EQUALS(clonedColInt->type(), "int");
    TS_ASSERT_EQUALS(clonedColStr->type(), "str");
  }
  void test_sortValues_by_two_keys() {
    const size_t n = 10;
    TableWorkspace ws(n);
    ws.addColumn("int", "col1");
    ws.addColumn("str", "col2");
    auto column1 = static_cast<TableColumn<int> &>(*ws.getColumn("col1"));
    auto &data1 = column1.data();
    data1[0] = 5;
    data1[1] = 7;
    data1[2] = 3;
    data1[3] = 12;
    data1[4] = 1;
    data1[5] = 3;
    data1[6] = 3;
    data1[7] = 2;
    data1[8] = 0;
    data1[9] = 12;

    auto column2 = static_cast<TableColumn<std::string> &>(*ws.getColumn("col2"));
    auto &data2 = column2.data();
    data2[0] = "five";
    data2[1] = "seven";
    data2[2] = "three (1)";
    data2[3] = "twelve (2)";
    data2[4] = "one";
    data2[5] = "three (3)";
    data2[6] = "three (2)";
    data2[7] = "two";
    data2[8] = "zero";
    data2[9] = "twelve (1)";

    auto indexVec = makeIndexVector(column1.size());
    std::vector<std::pair<size_t, size_t>> eqRanges;
    bool ascending = true;
    column1.sortIndex(ascending, 0, column1.size(), indexVec, eqRanges);

    TS_ASSERT_EQUALS(data1[indexVec[0]], 0);
    TS_ASSERT_EQUALS(data1[indexVec[1]], 1);
    TS_ASSERT_EQUALS(data1[indexVec[2]], 2);
    TS_ASSERT_EQUALS(data1[indexVec[3]], 3);
    TS_ASSERT_EQUALS(data1[indexVec[4]], 3);
    TS_ASSERT_EQUALS(data1[indexVec[5]], 3);
    TS_ASSERT_EQUALS(data1[indexVec[6]], 5);
    TS_ASSERT_EQUALS(data1[indexVec[7]], 7);
    TS_ASSERT_EQUALS(data1[indexVec[8]], 12);
    TS_ASSERT_EQUALS(data1[indexVec[9]], 12);

    TS_ASSERT_EQUALS(data2[indexVec[0]], "zero");
    TS_ASSERT_EQUALS(data2[indexVec[1]], "one");
    TS_ASSERT_EQUALS(data2[indexVec[2]], "two");
    TS_ASSERT_EQUALS(data2[indexVec[3]], "three (1)");
    TS_ASSERT_EQUALS(data2[indexVec[4]], "three (3)");
    TS_ASSERT_EQUALS(data2[indexVec[5]], "three (2)");
    TS_ASSERT_EQUALS(data2[indexVec[6]], "five");
    TS_ASSERT_EQUALS(data2[indexVec[7]], "seven");
    TS_ASSERT_EQUALS(data2[indexVec[8]], "twelve (2)");
    TS_ASSERT_EQUALS(data2[indexVec[9]], "twelve (1)");

    std::vector<std::pair<size_t, size_t>> eqRanges2;
    column2.sortIndex(ascending, eqRanges[0].first, eqRanges[0].second, indexVec, eqRanges2);
    TS_ASSERT(eqRanges2.empty());
    column2.sortIndex(ascending, eqRanges[1].first, eqRanges[1].second, indexVec, eqRanges2);
    TS_ASSERT(eqRanges2.empty());

    column1.sortValues(indexVec);
    column2.sortValues(indexVec);

    TS_ASSERT_EQUALS(data1[0], 0);
    TS_ASSERT_EQUALS(data1[1], 1);
    TS_ASSERT_EQUALS(data1[2], 2);
    TS_ASSERT_EQUALS(data1[3], 3);
    TS_ASSERT_EQUALS(data1[4], 3);
    TS_ASSERT_EQUALS(data1[5], 3);
    TS_ASSERT_EQUALS(data1[6], 5);
    TS_ASSERT_EQUALS(data1[7], 7);
    TS_ASSERT_EQUALS(data1[8], 12);
    TS_ASSERT_EQUALS(data1[9], 12);

    TS_ASSERT_EQUALS(data2[0], "zero");
    TS_ASSERT_EQUALS(data2[1], "one");
    TS_ASSERT_EQUALS(data2[2], "two");
    TS_ASSERT_EQUALS(data2[3], "three (1)");
    TS_ASSERT_EQUALS(data2[4], "three (2)");
    TS_ASSERT_EQUALS(data2[5], "three (3)");
    TS_ASSERT_EQUALS(data2[6], "five");
    TS_ASSERT_EQUALS(data2[7], "seven");
    TS_ASSERT_EQUALS(data2[8], "twelve (1)");
    TS_ASSERT_EQUALS(data2[9], "twelve (2)");
  }
  void test_strCannotBeConvertedToDouble() {
    TableWorkspace ws;
    ws.addColumn("str", "col");
    const auto col = ws.getColumn("col");
    TS_ASSERT(!col->isNumber());
  }
  void test_intCanBeConvertedToDouble() {
    TableWorkspace ws;
    ws.addColumn("int", "col");
    const auto col = ws.getColumn("col");
    TS_ASSERT(col->isNumber());
  }

  void test_equals_pass() {
    const size_t n = 10;
    TableWorkspace ws(n);
    ws.addColumn("int", "col1");

    auto column = static_cast<TableColumn<int> &>(*ws.getColumn("col1"));
    auto &data = column.data();
    data[0] = 5;
    data[1] = 7;
    data[2] = 3;
    data[3] = 12;
    data[4] = 1;
    data[5] = 6;
    data[6] = 3;
    data[7] = 2;
    data[8] = 0;
    data[9] = 12;

    auto column2 = std::unique_ptr<Mantid::API::Column>(column.clone());

    TS_ASSERT(column.equals(*column2, 0));
  }

  void test_equals_fail() {
    const size_t n = 10;
    TableWorkspace ws(n);
    ws.addColumn("int", "col1");

    auto column = static_cast<TableColumn<int> &>(*ws.getColumn("col1"));
    auto &data = column.data();
    data[0] = 5;
    data[1] = 7;
    data[2] = 3;
    data[3] = 12;
    data[4] = 1;
    data[5] = 6;
    data[6] = 3;
    data[7] = 2;
    data[8] = 0;
    data[9] = 12;

    auto column2 = std::unique_ptr<Mantid::API::Column>(column.clone());

    data[0] = 9;

    TS_ASSERT(!column.equals(*column2, 0));
  }

  void test_equals_fail_wrong_type() {
    const size_t n = 10;
    TableWorkspace ws(n);
    ws.addColumn("int", "col1");
    ws.addColumn("vector_int", "col2");
    auto column = static_cast<TableColumn<int> &>(*ws.getColumn("col1"));
    auto column2 = ws.getColumn("col2");
    TS_ASSERT(!column.equals(*column2, 0));
  }

  void test_equals_tolerance_normal_case() {
    const size_t n = 10;
    TableWorkspace ws(n);
    ws.addColumn("int", "col1");
    ws.addColumn("int", "col2");
    auto column = static_cast<TableColumn<int> &>(*ws.getColumn("col1"));
    auto column2 = static_cast<TableColumn<int> &>(*ws.getColumn("col2"));
    auto &data = column.data();
    data[0] = 5;
    data[1] = 7;
    data[2] = 3;
    data[3] = 12;
    data[4] = 1;
    data[5] = 6;
    data[6] = 3;
    data[7] = 2;
    data[8] = 0;
    data[9] = 12;
    auto &data2 = column2.data();
    data2[0] = 6;
    data2[1] = 6;
    data2[2] = 4;
    data2[3] = 13;
    data2[4] = 2;
    data2[5] = 5;
    data2[6] = 2;
    data2[7] = 1;
    data2[8] = 1;
    data2[9] = 11;
    TS_ASSERT(column.equals(column2, 1));
  }

  void test_equalsRelErr() {
    const size_t n = 10;
    TableWorkspace ws(n);
    ws.addColumn("int", "col1");
    ws.addColumn("int", "col2");
    auto column = static_cast<TableColumn<int> &>(*ws.getColumn("col1"));
    auto column2 = static_cast<TableColumn<int> &>(*ws.getColumn("col2"));
    auto &data = column.data();
    data[0] = 100;
    data[1] = 7;
    data[2] = 3;
    data[3] = 12;
    data[4] = 1;
    data[5] = 6;
    data[6] = 3;
    data[7] = 2;
    data[8] = 0;
    data[9] = 12;
    auto &data2 = column2.data();
    data2[0] = 90;
    data2[1] = 6;
    data2[2] = 3;
    data2[3] = 12;
    data2[4] = 1;
    data2[5] = 7;
    data2[6] = 2;
    data2[7] = 3;
    data2[8] = 0;
    data2[9] = 12;
    TS_ASSERT(column.equalsRelErr(column2, 1));
  }

  void test_equals_tolerance_int64() {
    const size_t n = 10;
    TableWorkspace ws(n);
    ws.addColumn("long64", "col1");
    ws.addColumn("long64", "col2");
    auto column = static_cast<TableColumn<int64_t> &>(*ws.getColumn("col1"));
    auto column2 = static_cast<TableColumn<int64_t> &>(*ws.getColumn("col2"));
    auto &data = column.data();
    data[0] = 165538;
    auto &data2 = column2.data();
    data2[0] = 165539;
    TS_ASSERT(column.equals(column2, 1));
  }

  void test_equals_string() {
    const size_t n = 10;
    TableWorkspace ws(n);
    ws.addColumn("str", "col1");
    auto column = static_cast<TableColumn<std::string> &>(*ws.getColumn("col1"));
    auto &data = column.data();
    data[0] = "hello";
    auto column2 = std::unique_ptr<Mantid::API::Column>(column.clone());
    TS_ASSERT(column.equals(*column2, 0));
  }

  void test_equals_string_tolerance_fail() {
    const size_t n = 10;
    TableWorkspace ws(n);
    ws.addColumn("str", "col1");
    auto column = static_cast<TableColumn<std::string> &>(*ws.getColumn("col1"));
    auto &data = column.data();
    data[0] = "1";
    auto column2 = std::unique_ptr<Mantid::API::Column>(column.clone());
    data[0] = "2";
    TS_ASSERT(!column.equals(*column2, 1));
  }

  void test_equals_general_tolerance_fail() {
    const size_t n = 10;
    TableWorkspace ws(n);
    ws.addColumn("int", "col1");
    ws.addColumn("int", "col2");
    auto column = static_cast<TableColumn<int> &>(*ws.getColumn("col1"));
    auto column2 = static_cast<TableColumn<int> &>(*ws.getColumn("col2"));
    auto &data = column.data();
    data[0] = 5;
    data[1] = 7;
    data[2] = 3;
    data[3] = 12;
    data[4] = 1;
    data[5] = 6;
    data[6] = 3;
    data[7] = 2;
    data[8] = 0;
    data[9] = 12;
    auto &data2 = column2.data();
    data2[0] = 7;
    data2[1] = 6;
    data2[2] = 4;
    data2[3] = 13;
    data2[4] = 2;
    data2[5] = 5;
    data2[6] = 2;
    data2[7] = 1;
    data2[8] = 1;
    data2[9] = 11;
    TS_ASSERT(!column.equals(*ws.getColumn("col2"), 1));
  }

  void test_equals_nan_and_double_fail() {
    const size_t n = 1;
    TableWorkspace ws(n);
    ws.addColumn("double", "col1");
    ws.addColumn("double", "col2");
    auto column = static_cast<TableColumn<double> &>(*ws.getColumn("col1"));
    auto column2 = static_cast<TableColumn<double> &>(*ws.getColumn("col2"));
    auto &data = column.data();
    data[0] = 5.0;
    auto &data2 = column2.data();
    data2[0] = std::numeric_limits<double>::quiet_NaN();
    TS_ASSERT(!column.equals(*ws.getColumn("col2"), 1));
  }

  void test_equals_two_nans_fail() {
    const size_t n = 1;
    TableWorkspace ws(n);
    ws.addColumn("double", "col1");
    ws.addColumn("double", "col2");
    auto column = static_cast<TableColumn<double> &>(*ws.getColumn("col1"));
    auto column2 = static_cast<TableColumn<double> &>(*ws.getColumn("col2"));
    auto &data = column.data();
    data[0] = std::numeric_limits<double>::quiet_NaN();
    auto &data2 = column2.data();
    data2[0] = std::numeric_limits<double>::quiet_NaN();
    TS_ASSERT(!column.equals(*ws.getColumn("col2"), 1));
  }

  void test_equals_two_nans_pass_with_flag() {
    const size_t n = 1;
    TableWorkspace ws(n);
    ws.addColumn("double", "col1");
    ws.addColumn("double", "col2");
    auto column = static_cast<TableColumn<double> &>(*ws.getColumn("col1"));
    auto column2 = static_cast<TableColumn<double> &>(*ws.getColumn("col2"));
    auto &data = column.data();
    data[0] = std::numeric_limits<double>::quiet_NaN();
    auto &data2 = column2.data();
    data2[0] = std::numeric_limits<double>::quiet_NaN();
    TS_ASSERT(!column.equals(*ws.getColumn("col2"), 1, true));
  }

private:
  std::vector<size_t> makeIndexVector(size_t n) {
    std::vector<size_t> vec(n);
    for (auto i = vec.begin() + 1; i != vec.end(); ++i) {
      *i = *(i - 1) + 1;
    }
    return vec;
  }
};
