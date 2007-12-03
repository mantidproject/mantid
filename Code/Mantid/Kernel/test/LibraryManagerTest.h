#ifndef MANTID_LIBRARYMANAGERTEST_H_
#define MANTID_LIBRARYMANAGERTEST_H_

#include <iostream>
#include <cxxtest/TestSuite.h>

//#include "MantidKernel/StatusCode.h"
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
	mgr = new LibraryManager;
  }
  
  void testOpenLibrary()
  {
	TS_ASSERT(mgr->OpenLibrary("MantidAlgorithms"));
  }
  
  void testLoadedAlgorithm()
  {
	try
	{
		Mantid::API::FrameworkManager manager;
		manager.initialize();
	  
		Mantid::API::IAlgorithm* alg= manager.createAlgorithm("HelloWorldAlgorithm");
	  
		StatusCode result = alg->execute();
	  
		TS_ASSERT( !result.isFailure() );
	
		delete alg;
		
	}
	catch (...)
	{
		//Probably failed because testOpenLibrary failed!
		TS_ASSERT( false );
	}
  }
  
};


#endif /*MANTID_LIBRARYMANAGERTEST_H_*/

