#ifndef MANTID_LIBRARYMANAGERTEST_H_
#define MANTID_LIBRARYMANAGERTEST_H_

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
	TS_ASSERT(mgr->OpenLibrary("TestAlgorithms"));
  }
  
  void testLoadedAlgorithm()
  {
	FrameworkManager manager;
	manager.initialize();
	  
	IAlgorithm* alg= manager.createAlgorithm("TestAlgorithm");
	  
	StatusCode result = alg->execute();
	  
	TS_ASSERT( !result.isFailure() );
	
	delete alg;
  }
  
};


#endif /*MANTID_LIBRARYMANAGERTEST_H_*/

