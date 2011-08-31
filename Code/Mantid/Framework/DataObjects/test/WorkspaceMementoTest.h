#ifndef MANTID_WORKSPACE_MEMENTO_TEST_H_
#define MANTID_WORKSPACE_MEMENTO_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/WorkspaceMemento.h"
#include "MantidDataObjects/TableWorkspace.h"

using namespace Mantid::DataObjects;

class WorkspaceMementoTest : public CxxTest::TestSuite
{

private:

  boost::shared_ptr<Mantid::API::ITableWorkspace>  ws;

public:

WorkspaceMementoTest() : ws(new TableWorkspace(2))
{
  //Set up a table workspace with one column and one row. then put a single 
  //value integer in that workspace with value 1.
  ws->addColumn("int", "test_col1");
  ws->addColumn("int", "test_col2");
}

void setUp()
{
  ws->getColumn(0)->cell<int>(0) = 1;
  ws->getColumn(1)->cell<int>(0) = 1;
}

void testInvalidWithoutAddingItems()
{
  WorkspaceMemento memento(ws, 0);
  TSM_ASSERT_THROWS("Should not be valid. No items added!", memento.validate(), std::runtime_error);
}

void testAddItems()
{
  WorkspaceMemento memento(ws, 0);
  memento.addItem(new WorkspaceMementoItem<0, int>(ws, 0));
  memento.addItem(new WorkspaceMementoItem<1, int>(ws, 0));
  TS_ASSERT_THROWS_NOTHING(memento.validate());
  TS_ASSERT(!memento.hasChanged());
}

void testItemHasChanged()
{
  WorkspaceMemento memento(ws, 0);
  typedef WorkspaceMementoItem<0, int> ColA;
  typedef WorkspaceMementoItem<1, int> ColB;

  ColA* colA = new ColA(ws, 0);
  ColB* colB = new ColB(ws, 0);

  memento.addItem(colA);
  memento.addItem(colB);
  TS_ASSERT_THROWS_NOTHING(memento.validate());
  TS_ASSERT(!memento.hasChanged());

  colA->setValue(9); //Set to something different.

  TSM_ASSERT("Should have registered that one of the items has changed.",memento.hasChanged());
}

void testItemReverted()
{
  WorkspaceMemento memento(ws, 0);
  typedef WorkspaceMementoItem<0, int> ColA;
  typedef WorkspaceMementoItem<1, int> ColB;

  ColA* colA = new ColA(ws, 0);
  ColB* colB = new ColB(ws, 0);

  memento.addItem(colA);
  memento.addItem(colB);

  colA->setValue(9); //Set to something different.
  memento.rollback(); //Now rollback changes.

  TSM_ASSERT("Should have rolledback everything.", !memento.hasChanged());
}


void testItemCommitted()
{
  WorkspaceMemento memento(ws, 0);
  typedef WorkspaceMementoItem<0, int> ColA;
  typedef WorkspaceMementoItem<1, int> ColB;

  ColA* colA = new ColA(ws, 0);
  ColB* colB = new ColB(ws, 0);

  memento.addItem(colA);
  memento.addItem(colB);

  colA->setValue(9); //Set to something different.
  memento.commit(); //Now commit changes.

  TSM_ASSERT("Should have commited everything.", !memento.hasChanged());
}

void testEquals()
{
  WorkspaceMemento a(ws, 0);
  a.addItem(new WorkspaceMementoItem<0, int>(ws, 0));
  a.addItem(new WorkspaceMementoItem<1, int>(ws, 0));

  WorkspaceMemento b(ws, 0);
  b.addItem(new WorkspaceMementoItem<0, int>(ws, 0));
  b.addItem(new WorkspaceMementoItem<1, int>(ws, 0));

  TS_ASSERT(a.equals(b));
  TS_ASSERT(b.equals(a));
  TS_ASSERT(a == b);
};

void testNotEquals()
{
  WorkspaceMemento a(ws, 0);
  a.addItem(new WorkspaceMementoItem<0, int>(ws, 0));
  a.addItem(new WorkspaceMementoItem<1, int>(ws, 0));

  WorkspaceMemento b(ws, 0);
  WorkspaceMementoItem<0, int>* colA = new WorkspaceMementoItem<0, int>(ws, 0);
  b.addItem(colA);
  colA->setValue(9);// Set item on b to something different.
  b.addItem(new WorkspaceMementoItem<1, int>(ws, 0));

  TS_ASSERT(!a.equals(b));
  TS_ASSERT(!b.equals(a));
  TS_ASSERT(a != b);
};

void testCheckLocking()
{
  WorkspaceMemento a(ws, 0);
  TSM_ASSERT_THROWS_NOTHING("Lock it.", a.lock());
  TSM_ASSERT("Check it's locked", a.locked());
  TSM_ASSERT_THROWS_NOTHING("Unlock it",  a.unlock());
  TSM_ASSERT("Check it's unlocked", !a.locked());
}

void testDuplicateLockThrows()
{
  WorkspaceMemento a(ws, 0);
  TSM_ASSERT_THROWS_NOTHING("Lock it.", a.lock());
  TSM_ASSERT("Check it's locked", a.locked());

  WorkspaceMemento b(ws, 0);
  TSM_ASSERT_THROWS("Already locked, should throw.", b.lock(), std::runtime_error);

  //Clean-up
  a.unlock();
  TSM_ASSERT("Check it's unlocked", !a.locked());
}

void testAutoUnlock()
{
  {
  WorkspaceMemento a(ws, 0);
  TSM_ASSERT_THROWS_NOTHING("Lock it.", a.lock());
  TSM_ASSERT("Check it's locked", a.locked());
  }
  //a is out of scope, should have unlocked itself on destruction.

  WorkspaceMemento b(ws, 0);
  TSM_ASSERT_THROWS_NOTHING("Should be unlocked, so should have obtained lock without throwing!", b.lock());
  b.unlock();
}




};

#endif