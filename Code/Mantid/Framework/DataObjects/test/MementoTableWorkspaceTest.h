#ifndef MANTID_DATAOBJECTS_MEMENTOTABLEWORKSPACETEST_H_
#define MANTID_DATAOBJECTS_MEMENTOTABLEWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/MementoTableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"

using namespace Mantid::DataObjects;
using namespace Mantid::API;

class MementoTableWorkspaceTest : public CxxTest::TestSuite
{
public:
  
  //Note that other tests for this type are covered in base class TableWorkspace test.

  void testFetchInstanceFromFactory()
  {
    ITableWorkspace_sptr product;
    TSM_ASSERT_THROWS_NOTHING("WS Factory could not create a MementoTableWorkspace", product = WorkspaceFactory::Instance().createTable("MementoTableWorkspace"));
    TSM_ASSERT("Product is not a MementoTableWorkspace", NULL != dynamic_cast<MementoTableWorkspace*>(product.get()));
  }

  void testConstruction()
  {
    MementoTableWorkspace ws;
    TableWorkspace* pWs = &ws; //This will break if ws is not a tableworkspace.
    TSM_ASSERT_EQUALS("Wrong number of columns constructed", 11, pWs->columnCount());
  }

  void testCompareWithWrongNColumns()
  {
    //Create a table workspace with too few columns.
    TableWorkspace ws; //Candidate workspace.
    TSM_ASSERT("Too few columns to be a MementoTableWorkspace", !MementoTableWorkspace::isMementoWorkspace(ws));
  }

  void testCompareWithWrongColumnType()
  {
    MementoTableWorkspace standard;
    TableWorkspace ws; //Candidate workspace.
    ws.addColumn("double", standard.getColumn(0)->name()); //Copy the name, but not the type.
    TSM_ASSERT("Wrong column type, should have been identified.", !MementoTableWorkspace::isMementoWorkspace(ws));
  }

  void testCompareWithWrongColumnName()
  {
    MementoTableWorkspace standard;
    TableWorkspace ws; //Candidate workspace.
    ws.addColumn(standard.getColumn(0)->type(), "?"); //Copy the type, but not the name.
    TSM_ASSERT("Wrong column name, should have been identified.", !MementoTableWorkspace::isMementoWorkspace(ws));
  }

  void testCorrectComparison()
  {
    MementoTableWorkspace standard;
    TSM_ASSERT("Should have been identified as MementoTableWorkspace.", MementoTableWorkspace::isMementoWorkspace(standard));
  }
  
};
	
#endif