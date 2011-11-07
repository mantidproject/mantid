#ifndef CUSTOM_INTERFACES_MEMENTO_ITEM_TEST_H_
#define CUSTOM_INTERFACES_MEMENTO_ITEM_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtCustomInterfaces/WorkspaceMementoItem.h"
#include "MantidDataObjects/TableWorkspace.h"
#include <string>

using namespace MantidQt::CustomInterfaces;
using Mantid::DataObjects::TableWorkspace;

class WorkspaceMementoItemTest : public CxxTest::TestSuite
{

private:

  boost::shared_ptr<Mantid::API::ITableWorkspace>  ws;

public:

void setUp()
{
  ws =  boost::shared_ptr<Mantid::API::ITableWorkspace>(new TableWorkspace(2));
  //Set up a table workspace with one column and one row. then put a single 
  //value integer in that workspace with value 1.
  ws->addColumn("int", "test_col1");
  ws->addColumn("int", "test_col2");
  ws->addColumn("str", "test_col3");
  ws->cell<int>(0, 0) = 1;
  ws->cell<int>(0, 1) = 1;
  ws->cell<std::string>(0, 2) = "val";
}

//=====================================================================================
// Functional tests
//=====================================================================================

void testConstructor()
{
  //Integer Item
  WorkspaceMementoItem<int> a(ws, Row(0), Column(0));
  TS_ASSERT_EQUALS(1, a.getValue());
  TSM_ASSERT_EQUALS("Should be default using this constructor", false, a.getIsNewDefinition());
}

void testOtherConstructor()
{
  WorkspaceMementoItem<std::string> a(ws, Row(0), Column(2), true);
  TS_ASSERT_EQUALS("val", a.getValue());
  TSM_ASSERT("New definition was set to true in constructor", true == a.getIsNewDefinition());
}

void testEqualsThrows()
{
  typedef WorkspaceMementoItem<int> TypeA;
  typedef WorkspaceMementoItem<std::string> TypeB; //Different column number constitutes a different type.
  TypeA A(ws, Row(0), Column(0));
  TypeB B(ws, Row(0), Column(2));

  TSM_ASSERT_THROWS("Should throw if types on which equals are called are not compatible.", A.equals(B), std::runtime_error);
}

void testEquals()
{
  WorkspaceMementoItem<int> a(ws, Row(0), Column(0));
  a.setValue(2);
  WorkspaceMementoItem<int> b(ws, Row(0), Column(0));
  b.setValue(2);

  TS_ASSERT(a.equals(b));
  TS_ASSERT(b.equals(a));
  TS_ASSERT_EQUALS(a, b);
}

void testNotEquals()
{
  WorkspaceMementoItem<int> a(ws, Row(0), Column(0));
  a.setValue(2);
  WorkspaceMementoItem<int> b(ws, Row(0), Column(0));
  b.setValue(3);

  TS_ASSERT(!a.equals(b));
  TS_ASSERT(!b.equals(a));
  TS_ASSERT_DIFFERS(a, b);
  TS_ASSERT(a.operator!=(b));
}

void testCopy()
{
  WorkspaceMementoItem<int> item(ws, Row(0), Column(0));
  item.setValue(3);
  WorkspaceMementoItem<int> copy(item);

  TS_ASSERT_EQUALS(item, copy);
}

void testAssign()
{
  WorkspaceMementoItem<int> a(ws, Row(0), Column(0));
  a.setValue(3);
  WorkspaceMementoItem<int> b(ws, Row(0), Column(0));
  b.setValue(4);
  b = a;
  TS_ASSERT_EQUALS(a, b);
  TS_ASSERT_EQUALS(3, b.getValue());
}

void testSetValue()
{
  WorkspaceMementoItem<int> item(ws, Row(0), Column(0));
  item.setValue(2);
  TS_ASSERT_EQUALS(2, item.getValue());
}

void testHasChanged()
{
  //create  a mementoitem pointing at the same cell in the table workspace.
  WorkspaceMementoItem<int> item(ws, Row(0), Column(0));
  TS_ASSERT(!item.hasChanged());
  item.setValue(2000);
  TS_ASSERT(item.hasChanged());
}

void testApplyChanges()
{

  //create  a mementoitem pointing at the same cell in the table workspace.
  WorkspaceMementoItem<int> item(ws, Row(0), Column(0));
  item.setValue(2);

  //Apply changes in memento over to the table workspace.
  TS_ASSERT_THROWS_NOTHING(item.commit());

  //Check that the chanes arrive.
  TS_ASSERT_EQUALS(2, ws->cell<int>(Row(0), Column(0)));
  TSM_ASSERT("Changes have been applied. Should not indicate outstanding!", !item.hasChanged())
}

void testRevertChanges()
{

  //create  a mementoitem pointing at the same cell in the table workspace.
  WorkspaceMementoItem<int> item(ws, Row(0), Column(0));
  item.setValue(2);

  //Apply changes in memento over to the table workspace.
  TS_ASSERT_THROWS_NOTHING(item.rollback());
  TSM_ASSERT("Changes have been reverted. Should not indicate outstanding!", !item.hasChanged())
}

void testRevertChangesWithNewDefintion()
{

  /*
  For this test schenario, we're indicating that column index == 2 is a new definition and should therefore be
  culled from the underlying workspace as part of the roll-back.
  */
  bool isNewDefintion = true;
  WorkspaceMementoItem<std::string> item(ws, Row(0), Column(2), isNewDefintion);

  //Apply changes in memento over to the table workspace.
  TS_ASSERT_THROWS_NOTHING(item.rollback());
  TSM_ASSERT("Changes have been reverted. Should not indicate outstanding!", !item.hasChanged());
  TSM_ASSERT("Table workspace should have one fewer columns after roll-back", 2 == ws->columnCount());
  TSM_ASSERT_THROWS_NOTHING("Calling commit after this type of rollback should gracefully do nothing", item.commit());
}

void testGetName()
{
  //create  a mementoitem pointing at the same cell in the table workspace.
  WorkspaceMementoItem<int> itemA(ws, Row(0), Column(0));
  WorkspaceMementoItem<int> itemB(ws, Row(0), Column(1));
  WorkspaceMementoItem<std::string> itemC(ws, Row(0), Column(2));

  TS_ASSERT_EQUALS(ws->getColumn(0)->name(), itemA.getName());
  TS_ASSERT_EQUALS(ws->getColumn(1)->name(), itemB.getName());
  TS_ASSERT_EQUALS(ws->getColumn(2)->name(), itemC.getName());

}



};

#endif