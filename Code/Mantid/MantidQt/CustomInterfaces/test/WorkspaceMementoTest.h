#ifndef CUSTOM_INTERFACES_WORKSPACE_MEMENTO_TEST_H_
#define CUSTOM_INTERFACES_WORKSPACE_MEMENTO_TEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtCustomInterfaces/WorkspaceMemento.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoItem.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoLock.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoService.h"
#include "MantidQtCustomInterfaces/LoanedMemento.h"
#include "MantidDataObjects/MementoTableWorkspace.h"
#include "MantidAPI/TableRow.h" 

using namespace MantidQt::CustomInterfaces;
using namespace testing;

using namespace Mantid::DataObjects;
using namespace Mantid::API;

class WorkspaceMementoTest : public CxxTest::TestSuite
{
private:

  //Mockup of a Locking object.
  class MockWorkspaceMementoLock : public MantidQt::CustomInterfaces::WorkspaceMementoLock
  {
  public:
      MOCK_METHOD0(lock, void());
      MOCK_METHOD0(unlock, bool());
      MOCK_CONST_METHOD0(locked, bool());
      ~MockWorkspaceMementoLock(){}
  };

  // Helper method. Make a table workspace formatted to contain a workspace on each row.
  static TableWorkspace_sptr makeTableWS()
  {
    TableWorkspace_sptr ws(new MementoTableWorkspace(1));
    TableRow row = ws->getRow(0);
    row << "TestWSRow" << "CNCS" << 1 << "SampleXML" << 1.0 << 1.0 << 1.0 << 90.0 << 90.0 << 90.0 << "Not Ready";
    return ws;
  }

  // Add items to the workspace. This is a job performed by WorkspaceMementoCollection when fully assembled.
  static void doAddItems(TableWorkspace_sptr ws, WorkspaceMemento& memento)
  {
    LoanedMemento managed(&memento);
    WorkspaceMementoService<LoanedMemento> service(managed);
    service.addAllItems(ws, 0);
  }

public:

//=====================================================================================
// Functional tests
//=====================================================================================

   void testInvalidByDefault()
   {
     TableWorkspace_sptr ws = makeTableWS();
     WorkspaceMemento memento(ws, "TestWSRow", 0);
     //No calls to memento.addItem
     TSM_ASSERT_THROWS("Should be invalid until fully configured via ::addItem", memento.validate(), std::runtime_error)
   }

   void testMakeValid()
   {
     TableWorkspace_sptr ws = makeTableWS();
     WorkspaceMemento memento(ws, "TestWSRow", 0);
     
     //Items added so should be valid now.
     doAddItems(ws, memento);

     TSM_ASSERT_THROWS_NOTHING("Should be valid all items added via ::addItem", memento.validate());
   }

   void testLock()
   {
     MockWorkspaceMementoLock* lock = new MockWorkspaceMementoLock;
     EXPECT_CALL(*lock, lock()).Times(AtLeast(1)); //Check that lock on lock object is used.
     EXPECT_CALL(*lock, unlock()).Times(0);
     EXPECT_CALL(*lock, locked()).Times(0);

     TableWorkspace_sptr ws = makeTableWS();
     WorkspaceMemento memento(ws, "TestWSRow", lock);

     //Lock should use underlying lock object
     memento.lock();
     
     TS_ASSERT(Mock::VerifyAndClearExpectations(lock));
   }
   
   void testUnlock()
   {
     MockWorkspaceMementoLock* lock = new MockWorkspaceMementoLock;
     EXPECT_CALL(*lock, lock()).Times(0); //Check that lock on lock object is used.
     EXPECT_CALL(*lock, unlock()).Times(1);
     EXPECT_CALL(*lock, locked()).Times(0);

     TableWorkspace_sptr ws = makeTableWS();
     WorkspaceMemento memento(ws, "TestWSRow", lock);

     //unlock should use underlying lock object
     memento.unlock();
     
     TS_ASSERT(Mock::VerifyAndClearExpectations(lock));
   }
   
   void testLocked()
   {
     MockWorkspaceMementoLock* lock = new MockWorkspaceMementoLock;
     EXPECT_CALL(*lock, lock()).Times(0); //Check that lock on lock object is used.
     EXPECT_CALL(*lock, unlock()).Times(0);
     EXPECT_CALL(*lock, locked()).Times(1);

     TableWorkspace_sptr ws = makeTableWS();
     WorkspaceMemento memento(ws, "TestWSRow", lock);

     //locked should use underlying lock object
     memento.locked();
     
     TS_ASSERT(Mock::VerifyAndClearExpectations(lock));
   }

   void testHasNotChanged()
   {
     TableWorkspace_sptr ws = makeTableWS();
     WorkspaceMemento memento(ws, "TestWSRow", 0);

     doAddItems(ws, memento);

     TSM_ASSERT("Should indicate no changes in the default state via ::hasChanges", !memento.hasChanged());
   }

   void testHasChanged()
   {
     TableWorkspace_sptr ws = makeTableWS();
     WorkspaceMemento memento(ws, "TestWSRow", 0);

     doAddItems(ws, memento);
     std::string newValue = "New Name";
     memento.getItem(0)->setValue(newValue); //Give the memento a new name.
     
     TSM_ASSERT("Should indicate changes state via ::hasChanges", memento.hasChanged());

   }

   void testRollBack()
   {
     TableWorkspace_sptr ws = makeTableWS();
     WorkspaceMemento memento(ws, "TestWSRow", 0);

     doAddItems(ws, memento);
     std::string newValue = "New Instrument";
     memento.getItem(1)->setValue(newValue); //Give the memento a new instrument.
     TSM_ASSERT("Should indicate changes state via ::hasChanges", memento.hasChanged());
     memento.rollback(); //Roll back the changes.
     TSM_ASSERT("Should indicate no changes after rollBack", !memento.hasChanged());
   }

   void testCommit()
   {
     TableWorkspace_sptr ws = makeTableWS();
     WorkspaceMemento memento(ws, "TestWSRow", 0);

     doAddItems(ws, memento);
     std::string newValue = "New Instrument";
     memento.getItem(1)->setValue(newValue);//Give the memento a new instrument.
     TSM_ASSERT("Should indicate changes state via ::hasChanges", memento.hasChanged());
     memento.commit(); //Commit the changes. Make them stick.
     TSM_ASSERT("Should indicate no changes after commit", !memento.hasChanged());

     std::string actual;
     memento.getItem(1)->getValue(actual);
     TSM_ASSERT_EQUALS("Should fetch new instrument name", "New Instrument", actual);
   }

   void testEquals()
   {
     TableWorkspace_sptr ws = makeTableWS();

     WorkspaceMemento a(ws, "A", 0);
     doAddItems(ws, a);

     WorkspaceMemento b(ws, "B", 0);
     doAddItems(ws, b);

     WorkspaceMemento c(ws, "C", 0);
     doAddItems(ws, c);
     std::string newValue = "New Name";
     c.getItem(0)->setValue(newValue);

     TS_ASSERT(a.equals(b));
     TS_ASSERT(a == b);
     TS_ASSERT(!a.equals(c));
   }

   void testNotEquals()
   {
     TableWorkspace_sptr ws = makeTableWS();

     WorkspaceMemento a(ws, "A", 0);
     doAddItems(ws, a);

     WorkspaceMemento b(ws, "B", 0);
     doAddItems(ws, b);

     WorkspaceMemento c(ws, "C", 0);
     doAddItems(ws, c);
     std::string newValue = "New Name";
     c.getItem(0)->setValue(newValue);

     TS_ASSERT(!(a != b));
     TS_ASSERT(a != c);
   }

};

#endif