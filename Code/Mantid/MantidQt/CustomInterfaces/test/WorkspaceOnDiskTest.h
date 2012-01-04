#ifndef CUSTOM_INTERFACES_WORKSPACE_ON_DISK_TEST_H_
#define CUSTOM_INTERFACES_WORKSPACE_ON_DISK_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtCustomInterfaces/WorkspaceOnDisk.h"
#include "MantidAPI/FileFinder.h"

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
    MatrixWorkspace_sptr result = memento.fetchIt();
    TSM_ASSERT("Should have fetched the workspace", result);
  }

  void testNoExistingUB()
  {
    WorkspaceOnDisk memento(getSuitableFileNamePath());
    TS_ASSERT_EQUALS(WorkspaceMemento::NoOrientedLattice, memento.generateStatus());
  }

};

#endif