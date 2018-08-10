#ifndef MANTID_DATAOBJECTS_MEMENTOTABLEWORKSPACETEST_H_
#define MANTID_DATAOBJECTS_MEMENTOTABLEWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/MementoTableWorkspace.h"

using namespace Mantid::DataObjects;
using namespace Mantid::API;

class MementoTableWorkspaceTest : public CxxTest::TestSuite {
public:
  // Note that other tests for this type are covered in base class
  // TableWorkspace test.

  void testFetchInstanceFromFactory() {
    ITableWorkspace_sptr product;
    TSM_ASSERT_THROWS_NOTHING(
        "WS Factory could not create a MementoTableWorkspace",
        product =
            WorkspaceFactory::Instance().createTable("MementoTableWorkspace"));
    TSM_ASSERT("Product is not a MementoTableWorkspace",
               nullptr != dynamic_cast<MementoTableWorkspace *>(product.get()));
  }

  void testConstruction() {
    MementoTableWorkspace ws;
    TableWorkspace *pWs = &ws; // This will break if ws is not a tableworkspace.
    TSM_ASSERT_EQUALS("Wrong number of columns constructed", 11,
                      pWs->columnCount());
  }

  void testClone() {
    MementoTableWorkspace tw(1);

    tw.getColumn(0)->cell<std::string>(0) = "a";
    tw.getColumn(1)->cell<std::string>(0) = "b";
    tw.getColumn(2)->cell<int>(0) = 421;
    tw.getColumn(3)->cell<std::string>(0) = "c";
    tw.getColumn(4)->cell<double>(0) = 1.1;
    tw.getColumn(5)->cell<double>(0) = 2.1;
    tw.getColumn(6)->cell<double>(0) = 3.1;
    tw.getColumn(7)->cell<double>(0) = 4.1;
    tw.getColumn(8)->cell<double>(0) = 5.1;
    tw.getColumn(9)->cell<double>(0) = 6.1;
    tw.getColumn(10)->cell<std::string>(0) = "d";

    auto cloned = tw.clone();

    // Check clone is same as original.
    TS_ASSERT_EQUALS(tw.columnCount(), cloned->columnCount());
    TS_ASSERT_EQUALS(tw.rowCount(), cloned->rowCount());
    TS_ASSERT_EQUALS("a", cloned->getColumn(0)->cell<std::string>(0));
    TS_ASSERT_EQUALS("b", cloned->getColumn(1)->cell<std::string>(0));
    TS_ASSERT_EQUALS(421, cloned->getColumn(2)->cell<int>(0));
    TS_ASSERT_EQUALS("c", cloned->getColumn(3)->cell<std::string>(0));
    TS_ASSERT_EQUALS(1.1, cloned->getColumn(4)->cell<double>(0));
    TS_ASSERT_EQUALS(2.1, cloned->getColumn(5)->cell<double>(0));
    TS_ASSERT_EQUALS(3.1, cloned->getColumn(6)->cell<double>(0));
    TS_ASSERT_EQUALS(4.1, cloned->getColumn(7)->cell<double>(0));
    TS_ASSERT_EQUALS(5.1, cloned->getColumn(8)->cell<double>(0));
    TS_ASSERT_EQUALS(6.1, cloned->getColumn(9)->cell<double>(0));
    TS_ASSERT_EQUALS("d", cloned->getColumn(10)->cell<std::string>(0));
  }

  void testCompareWithWrongNColumns() {
    // Create a table workspace with too few columns.
    TableWorkspace ws; // Candidate workspace.
    TSM_ASSERT("Too few columns to be a MementoTableWorkspace",
               !MementoTableWorkspace::isMementoWorkspace(ws));
  }

  void testCompareWithWrongColumnType() {
    MementoTableWorkspace standard;
    TableWorkspace ws; // Candidate workspace.
    ws.addColumn(
        "double",
        standard.getColumn(0)->name()); // Copy the name, but not the type.
    TSM_ASSERT("Wrong column type, should have been identified.",
               !MementoTableWorkspace::isMementoWorkspace(ws));
  }

  void testCompareWithWrongColumnName() {
    MementoTableWorkspace standard;
    TableWorkspace ws; // Candidate workspace.
    ws.addColumn(standard.getColumn(0)->type(),
                 "?"); // Copy the type, but not the name.
    TSM_ASSERT("Wrong column name, should have been identified.",
               !MementoTableWorkspace::isMementoWorkspace(ws));
  }

  void testCorrectComparison() {
    MementoTableWorkspace standard;
    TSM_ASSERT("Should have been identified as MementoTableWorkspace.",
               MementoTableWorkspace::isMementoWorkspace(standard));
  }
};

#endif