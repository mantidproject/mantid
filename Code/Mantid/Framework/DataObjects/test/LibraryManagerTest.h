#ifndef MANTID_LIBRARYMANAGERTEST_H_
#define MANTID_LIBRARYMANAGERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/LibraryManager.h"

class LibraryManagerTest : public CxxTest::TestSuite
{
public:

  void testOpenLibrary()
  {
    using namespace Mantid::Kernel;
    // This first line can go once scons does
    int libsOpened = LibraryManager::Instance().OpenAllLibraries("../../Build/Plugins/", false);
    libsOpened += LibraryManager::Instance().OpenAllLibraries(getDirectoryOfExecutable(), false);

    TSM_ASSERT ( "No shared libraries could be loaded", libsOpened ); 
  }

  void testLoadedAlgorithm()
  {
    try
    {
      Mantid::API::FrameworkManager::Instance().createAlgorithm("HelloWorldAlgorithm");

      TS_ASSERT_THROWS_NOTHING(Mantid::API::FrameworkManager::Instance().exec("HelloWorldAlgorithm", ""));
    }
    catch (...)
    {
      //Probably failed because testOpenLibrary failed!
      TS_FAIL("Could not create HelloWorldAlgorithm");
    }
  }

};

#endif /*MANTID_LIBRARYMANAGERTEST_H_*/

