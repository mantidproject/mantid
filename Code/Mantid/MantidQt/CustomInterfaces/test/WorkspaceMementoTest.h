#ifndef CUSTOM_INTERFACES_WORKSPACE_MEMENTO_TEST_H_
#define CUSTOM_INTERFACES_WORKSPACE_MEMENTO_TEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtCustomInterfaces/WorkspaceMemento.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

//using namespace Mantid::DataObjects;
//using namespace Mantid::API;


//=====================================================================================
// Functional tests
//=====================================================================================
class WorkspaceMementoTest : public CxxTest::TestSuite
{

private:

/// Helper type. Concrete Workspace Memento.
class ConcreteWorkspaceMemento : public WorkspaceMemento
{
public:

  virtual std::string getId() const
  {
    throw std::runtime_error("Not implemented");
  }

  virtual std::string locationType() const 
  {
    throw std::runtime_error("Not implemented");
  }

  virtual bool checkStillThere() const
  {
    throw std::runtime_error("Not implemented");
  }

  virtual Mantid::API::Workspace_sptr fetchIt() const
  {
    throw std::runtime_error("Not implemented");
  }

  virtual void cleanUp()
  {
    throw std::runtime_error("Not implemented");
  }

  virtual Mantid::API::Workspace_sptr applyActions()
  {
    throw std::runtime_error("Not implemented");
  }
};


public:

  void testGetEmptyUB()
  {
    ConcreteWorkspaceMemento memento;
    TSM_ASSERT("Should be empty if no ub provided.", memento.getUB().empty());
  }

  void testSetUB()
  {
    ConcreteWorkspaceMemento memento;
    memento.setUB(1, 2, 3, 4, 5, 6, 7, 8, 9);
    std::vector<double> ub = memento.getUB();
    TS_ASSERT_EQUALS(1, ub[0]);
    TS_ASSERT_EQUALS(2, ub[1]);
    TS_ASSERT_EQUALS(3, ub[2]);
    TS_ASSERT_EQUALS(4, ub[3]);
    TS_ASSERT_EQUALS(5, ub[4]);
    TS_ASSERT_EQUALS(6, ub[5]);
    TS_ASSERT_EQUALS(7, ub[6]);
    TS_ASSERT_EQUALS(8, ub[7]);
    TS_ASSERT_EQUALS(9, ub[8]);
  }

  void testStatusWithoutUB()
  {
    ConcreteWorkspaceMemento memento;
    TS_ASSERT_EQUALS(WorkspaceMemento::NoOrientedLattice, memento.generateStatus());
  }

  void testStatusWithUB()
  {
    ConcreteWorkspaceMemento memento;
    memento.setUB(1, 2, 3, 4, 5, 6, 7, 8, 9);
    TS_ASSERT_EQUALS(WorkspaceMemento::Ready, memento.generateStatus());
  }

};

#endif