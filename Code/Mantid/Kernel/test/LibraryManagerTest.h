#ifndef MANTID_LIBRARYMANAGERTEST_H_
#define MANTID_LIBRARYMANAGERTEST_H_

#include <iostream>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/LibraryManager.h"

using namespace Mantid::Kernel;

class LibraryManagerTest : public CxxTest::TestSuite
{

private:
	LibraryManager* mgr;	

public: 

  LibraryManagerTest()
  {
	mgr = LibraryManager::Instance();
  }
  
  void testOpenLibrary()
  {
	TS_ASSERT_LESS_THAN(0, mgr->OpenAllLibraries("../../Bin/Shared/", false));
  }
  
  void testLoadedAlgorithm()
  {
	try
	{
		Mantid::API::FrameworkManager* manager = Mantid::API::FrameworkManager::Instance();
		manager->initialize();
	  
		manager->createAlgorithm("HelloWorldAlgorithm");
	  
		TS_ASSERT_THROWS_NOTHING(manager->exec("HelloWorldAlgorithm", ""));				
	}
	catch (...)
	{
		//Probably failed because testOpenLibrary failed!
		TS_ASSERT( false );
	}
  }
  
};


#endif /*MANTID_LIBRARYMANAGERTEST_H_*/

