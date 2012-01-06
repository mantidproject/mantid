#ifndef CUSTOM_INTERFACES_WORKSPACE_ON_DISK_TEST_H_
#define CUSTOM_INTERFACES_WORKSPACE_ON_DISK_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtCustomInterfaces/WorkspaceOnDisk.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

using namespace MantidQt::CustomInterfaces;

//using namespace Mantid::DataObjects;
using namespace Mantid::API;

class WorkspaceOnDiskTest : public CxxTest::TestSuite
{
private:

  static std::string getSuitableFileNamePath()
  {
    return Mantid::API::FileFinder::Instance().getFullPath("LOQ48127.raw");
  }

public:

  void testConstructorThrowsWithWrongExtension()
  {
    std::string badFile = "MAR11001.rrr"; //Fictional extension
    TSM_ASSERT_THROWS("Unknown extension, should throw.", new WorkspaceOnDisk(badFile), std::invalid_argument);
  }

  void testFileExists()
  {
    WorkspaceOnDisk memento(getSuitableFileNamePath());
    TSM_ASSERT("File should be present", memento.checkStillThere()); 
  }

  void testConstructThrowsWhenFileDoesntExist()
  {
    TSM_ASSERT_THROWS("Unknown file, should throw.", new WorkspaceOnDisk("MadeUp.raw"), std::runtime_error);
  }

  void testFetchItSucceedsWhenFileExists()
  {
    WorkspaceOnDisk memento(getSuitableFileNamePath());
    TSM_ASSERT("File should be present", memento.checkStillThere()); 
    MatrixWorkspace_sptr result = boost::dynamic_pointer_cast<MatrixWorkspace>(memento.fetchIt());
    TSM_ASSERT("Should have fetched the workspace", result);
  }

  void testNoExistingUB()
  {
    WorkspaceOnDisk memento(getSuitableFileNamePath());
    TS_ASSERT_EQUALS(WorkspaceMemento::NoOrientedLattice, memento.generateStatus());
  }

  void testApplyActions()
  {
    WorkspaceOnDisk memento(getSuitableFileNamePath());
    memento.setUB(0,0,2,0,4,0,-8,0,0);
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(memento.applyActions());
    std::vector<double> ub = ws->sample().getOrientedLattice().getUB().get_vector();
    TS_ASSERT_EQUALS(0, ub[0]);
    TS_ASSERT_EQUALS(0, ub[1]);
    TS_ASSERT_EQUALS(2, ub[2]);
    TS_ASSERT_EQUALS(0, ub[3]);
    TS_ASSERT_EQUALS(4, ub[4]);
    TS_ASSERT_EQUALS(0, ub[5]);
    TS_ASSERT_EQUALS(-8, ub[6]);
    TS_ASSERT_EQUALS(0, ub[7]);
    TS_ASSERT_EQUALS(0, ub[8]);
  }

};

#endif