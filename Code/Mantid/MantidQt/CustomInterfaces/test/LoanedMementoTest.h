#ifndef CUSTOM_INTERFACES_LOANED_MEMENTO_TEST_H_
#define CUSTOM_INTERFACES_LOANED_MEMENTO_TEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtCustomInterfaces/WorkspaceMemento.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoItem.h"
#include "MantidQtCustomInterfaces/LoanedMemento.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoLock.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoService.h"
#include "MantidDataObjects/MementoTableWorkspace.h"
#include "MantidAPI/TableRow.h" 

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace testing;

class LoanedMementoTest : public CxxTest::TestSuite
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
  static void doAddItems(TableWorkspace_sptr ws, WorkspaceMemento* memento)
  {
    int rowIndex = 0;
    LoanedMemento managed(memento);
    WorkspaceMementoService<LoanedMemento> service(managed);
    service.addAllItems(ws, rowIndex);
  }

public:

//=====================================================================================
// Functional tests
//=====================================================================================

  void testThrowsIfWsMementoNULL()
  {
    WorkspaceMemento* memento = NULL;
    TSM_ASSERT_THROWS("Cannot wrap NULL, should throw!", LoanedMemento loan(memento), std::runtime_error); //Should automatically lock
  }

  //Test that smart loaned pointer performs lock/unlock
  void  testAutoLockUnlock()
  {
    MockWorkspaceMementoLock* lock = new MockWorkspaceMementoLock;
    EXPECT_CALL(*lock, lock()).Times(1); //Expecting a lock call
    EXPECT_CALL(*lock, unlock()).Times(1); //Expecting a unlock call
    EXPECT_CALL(*lock, locked()).Times(0);

    TableWorkspace_sptr ws = makeTableWS();
    WorkspaceMemento* memento = new WorkspaceMemento(ws, "TestWSRow", lock);
    {
      LoanedMemento loan(memento); //Should automatically lock
    }

    TS_ASSERT(Mock::VerifyAndClearExpectations(lock));
    delete memento;
  }

  void  testCopyConstructor()
  {
    MockWorkspaceMementoLock* lock = new MockWorkspaceMementoLock;
    EXPECT_CALL(*lock, lock()).Times(AtLeast(1)); //Lock on a, then lock on b.
    EXPECT_CALL(*lock, unlock()).Times(AtLeast(2)); //unlock on a, then unlock on b
    EXPECT_CALL(*lock, locked()).Times(0);

    TableWorkspace_sptr ws = makeTableWS();
    WorkspaceMemento* memento = new WorkspaceMemento(ws, "TestWSRow", lock);
    doAddItems(ws, memento);
    {
      LoanedMemento a(memento); 
      LoanedMemento b(a); //Generates a duplicate wrapper of the same memento ptr.
      TS_ASSERT_THROWS_NOTHING(a->getItem(0));
      TS_ASSERT_THROWS_NOTHING(b->getItem(0));
    }

    TS_ASSERT(Mock::VerifyAndClearExpectations(lock));
    delete memento;
  }

};

#endif