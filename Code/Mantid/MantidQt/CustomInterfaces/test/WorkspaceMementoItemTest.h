#ifndef CUSTOM_INTERFACES_MEMENTO_ITEM_TEST_H_
#define CUSTOM_INTERFACES_MEMENTO_ITEM_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtCustomInterfaces/WorkspaceMementoItem.h"
#include "MantidDataObjects/TableWorkspace.h"
#include <string>

using MantidQt::CustomInterfaces::WorkspaceMementoItem;
using Mantid::DataObjects::TableWorkspace;

class WorkspaceMementoItemTest : public CxxTest::TestSuite
{

private:

  boost::shared_ptr<Mantid::API::ITableWorkspace>  ws;

public:

WorkspaceMementoItemTest() : ws(new TableWorkspace(2))
{
  //Set up a table workspace with one column and one row. then put a single 
  //value integer in that workspace with value 1.
  ws->addColumn("int", "test_col1");
  ws->addColumn("int", "test_col2");
  ws->cell<int>(0, 0) = 1;
  ws->cell<int>(0, 1) = 1;
}

void testConstructor()
{
  //Integer Item
  WorkspaceMementoItem<0, int> a(ws, 0);
  size_t colindex = a.ColIndex;
  TS_ASSERT_EQUALS(0, colindex);
  TS_ASSERT_EQUALS(1, a.getValue());
}

void testEqualsThrows()
{
  typedef WorkspaceMementoItem<0, int> TypeA;
  typedef WorkspaceMementoItem<1, int> TypeB; //Different column number constitutes a different type.
  TypeA A(ws, 0);
  TypeB B(ws, 0);

  TSM_ASSERT_THROWS("Should throw if types on which equals are called are not compatible.", A.equals(B), std::runtime_error);
}

void testEquals()
{
  WorkspaceMementoItem<0, int> a(ws, 0);
  a.setValue(2);
  WorkspaceMementoItem<0, int> b(ws, 0);
  b.setValue(2);

  TS_ASSERT(a.equals(b));
  TS_ASSERT(b.equals(a));
  TS_ASSERT_EQUALS(a, b);
}

void testNotEquals()
{
  WorkspaceMementoItem<0, int> a(ws, 0);
  a.setValue(2);
  WorkspaceMementoItem<0, int> b(ws, 0);
  b.setValue(3);

  TS_ASSERT(!a.equals(b));
  TS_ASSERT(!b.equals(a));
  TS_ASSERT_DIFFERS(a, b);
  TS_ASSERT(a.operator!=(b));
}

void testCopy()
{
  WorkspaceMementoItem<0, int> item(ws, 0);
  item.setValue(3);
  WorkspaceMementoItem<0, int> copy(item);

  TS_ASSERT_EQUALS(item, copy);
}

void testAssign()
{
  WorkspaceMementoItem<0, int> a(ws, 0);
  a.setValue(3);
  WorkspaceMementoItem<0, int> b(ws, 0);
  b.setValue(4);
  b = a;
  TS_ASSERT_EQUALS(a, b);
  TS_ASSERT_EQUALS(3, b.getValue());
}

void testSetValue()
{
  WorkspaceMementoItem<0, int> item(ws, 0);
  item.setValue(2);
  TS_ASSERT_EQUALS(2, item.getValue());
}

void testHasChanged()
{
  //create  a mementoitem pointing at the same cell in the table workspace.
  WorkspaceMementoItem<0, int> item(ws, 0);
  TS_ASSERT(!item.hasChanged());
  item.setValue(2000);
  TS_ASSERT(item.hasChanged());
}

void testApplyChanges()
{

  //create  a mementoitem pointing at the same cell in the table workspace.
  WorkspaceMementoItem<0, int> item(ws, 0);
  item.setValue(2);

  //Apply changes in memento over to the table workspace.
  TS_ASSERT_THROWS_NOTHING(item.commit());

  //Check that the chanes arrive.
  TS_ASSERT_EQUALS(2, ws->cell<int>(0, 0));
  TSM_ASSERT("Changes have been applied. Should not indicate outstanding!", !item.hasChanged())
}

void testRevertChanges()
{

  //create  a mementoitem pointing at the same cell in the table workspace.
  WorkspaceMementoItem<0, int> item(ws, 0);
  item.setValue(2);

  //Apply changes in memento over to the table workspace.
  TS_ASSERT_THROWS_NOTHING(item.rollback());
  TSM_ASSERT("Changes have been reverted. Should not indicate outstanding!", !item.hasChanged())
}



};

#endif