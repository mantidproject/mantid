#ifndef  MANTID_PYTHONFRAMEWORKTESTS_H_
#define  MANTID_PYTHONFRAMEWORKTESTS_H_

#include <vector>

#include <cxxtest/TestSuite.h>
#include "../../Algorithms/test/WorkspaceCreationHelper.hh"

#include "MantidPythonAPI/FrameworkManager.h"
#include "MantidPythonAPI/SimplePythonAPI.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid;
using namespace Mantid::PythonAPI;

class PythonFrameworkTest : public CxxTest::TestSuite
{

private:
	Mantid::PythonAPI::FrameworkManager* mgr;

public:

	PythonFrameworkTest()
	{
		mgr = new Mantid::PythonAPI::FrameworkManager;
        Mantid::Kernel::LibraryManager::Instance().OpenAllLibraries("../../Debug");
	}

	void testCreateAlgorithmMethod1()
	{
		API::IAlgorithm* alg = mgr->createAlgorithm("HelloWorldAlgorithm");
		TS_ASSERT_EQUALS(alg->name(), "HelloWorldAlgorithm");
	}

	void testCreateAlgorithmNotFoundThrows()
	{
		TS_ASSERT_THROWS_ANYTHING(mgr->createAlgorithm("Rubbish!"));
	}

	void testGetDeleteWorkspace()
	{
    API::AnalysisDataService::Instance().add("TestWorkspace1",WorkspaceCreationHelper::Create2DWorkspace123(10,22,1));
    API::MatrixWorkspace* ws = dynamic_cast<API::MatrixWorkspace*>(mgr->getMatrixWorkspace("TestWorkspace1"));

		TS_ASSERT_EQUALS(ws->getNumberHistograms(), 22);
		TS_ASSERT(mgr->deleteWorkspace("TestWorkspace1"));
	}
	
	void testCreateAlgorithmMethod2()
	{
		API::IAlgorithm* alg = mgr->createAlgorithm("PropertyAlgorithm", "10;9.99");

    TS_ASSERT( alg->isInitialized() )
    TS_ASSERT( !alg->isExecuted() )
    TS_ASSERT_EQUALS( alg->getPropertyValue("IntValue"), "10" )
    TS_ASSERT_EQUALS( alg->getPropertyValue("DoubleValue"), "9.99" )
	}
	
	void testExecuteAlgorithmMethod()
	{
		API::IAlgorithm* alg = mgr->execute("PropertyAlgorithm", "8;8.88");
    TS_ASSERT_EQUALS( alg->getPropertyValue("IntValue"), "8" )
    TS_ASSERT( alg->isExecuted() )
	}


  void testgetWorkspaceNames()
  {
    std::vector<std::string> temp = mgr->getWorkspaceNames();
    TS_ASSERT(temp.empty());
    
    Mantid::API::AnalysisDataService::Instance().add("outer",WorkspaceCreationHelper::Create2DWorkspace123(10,22,1));

    temp = mgr->getWorkspaceNames();
    TS_ASSERT(!temp.empty());
    TS_ASSERT_EQUALS( temp[0], "outer" )
    mgr->deleteWorkspace("outer");
    temp = mgr->getWorkspaceNames();
    TS_ASSERT(temp.empty());
  }
	
  void testGetAlgorithmNames()
  {
    std::vector<std::string> temp = mgr->getAlgorithmNames();

    TS_ASSERT(!temp.empty());
  }

  void testCreatePythonSimpleAPI()
  {
    mgr->createPythonSimpleAPI(false);
    Poco::File apimodule(SimplePythonAPI::getModuleName());
    TS_ASSERT( apimodule.exists() );
    TS_ASSERT_THROWS_NOTHING( apimodule.remove() );
    TS_ASSERT( !apimodule.exists() );
  }


};

#endif /*MANTID_PYTHONFRAMEWORKTESTS_H_*/

