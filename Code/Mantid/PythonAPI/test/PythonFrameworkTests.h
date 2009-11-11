#ifndef  MANTID_PYTHONFRAMEWORKTESTS_H_
#define  MANTID_PYTHONFRAMEWORKTESTS_H_

#include <vector>

#include <cxxtest/TestSuite.h>
#include "../../Algorithms/test/WorkspaceCreationHelper.hh"

#include "MantidPythonAPI/FrameworkManagerProxy.h"
#include "MantidPythonAPI/SimplePythonAPI.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "Poco/File.h"

using namespace Mantid::PythonAPI;

bool FrameworkManagerProxy::m_gil_required = false;

class FrameworkManagerProxyTest : public CxxTest::TestSuite
{

private:
  Mantid::PythonAPI::FrameworkManagerProxy* mgr;

public:

  FrameworkManagerProxyTest()
  {
    mgr = new Mantid::PythonAPI::FrameworkManagerProxy;
    Mantid::Kernel::LibraryManager::Instance().OpenAllLibraries("../../Debug");
  }

  void testCreateAlgorithmMethod1()
  {
    Mantid::API::IAlgorithm* alg = mgr->createAlgorithm("HelloWorldAlgorithm");
    TS_ASSERT_EQUALS(alg->name(), "HelloWorldAlgorithm");
  }

  void testCreateAlgorithmNotFoundThrows()
  {
    TS_ASSERT_THROWS_ANYTHING(mgr->createAlgorithm("Rubbish!"));
  }

  void testGetDeleteWorkspace()
  {
    Mantid::API::AnalysisDataService::Instance().add("TestWorkspace1",WorkspaceCreationHelper::Create2DWorkspace123(10,22,1));
    Mantid::API::MatrixWorkspace_sptr ws = mgr->retrieveMatrixWorkspace("TestWorkspace1");

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 22);
    TS_ASSERT(mgr->deleteWorkspace("TestWorkspace1"));
  }
	
  void testCreateAlgorithmMethod2()
  {
    Mantid::API::IAlgorithm* alg = mgr->createAlgorithm("PropertyAlgorithm", "10;9.99");
	  
    TS_ASSERT( alg->isInitialized() );
    TS_ASSERT( !alg->isExecuted() );
    TS_ASSERT_EQUALS( alg->getPropertyValue("IntValue"), "10" );
    TS_ASSERT_EQUALS( alg->getPropertyValue("DoubleValue"), "9.99" );
  }
	
  void testExecuteAlgorithmMethod()
  {
    Mantid::API::IAlgorithm* alg = mgr->execute("PropertyAlgorithm", "8;8.88");
    TS_ASSERT_EQUALS( alg->getPropertyValue("IntValue"), "8" );
    TS_ASSERT( alg->isExecuted() );
  }


  void testgetWorkspaceNames()
  {
    std::set<std::string> temp = mgr->getWorkspaceNames();
    TS_ASSERT(temp.empty());
    const std::string name = "outer";
    Mantid::API::AnalysisDataService::Instance().add(name,WorkspaceCreationHelper::Create2DWorkspace123(10,22,1));

    temp = mgr->getWorkspaceNames();
    TS_ASSERT(!temp.empty());
    TS_ASSERT( temp.count(name) )
      mgr->deleteWorkspace(name);
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

  void testDoesWorkspaceExist()
  {
    const std::string name = "outer";
    TS_ASSERT_EQUALS(mgr->workspaceExists(name), false);
    //Add the workspace
    Mantid::API::AnalysisDataService::Instance().add(name,WorkspaceCreationHelper::Create2DWorkspace123(10,22,1));

    TS_ASSERT_EQUALS(mgr->workspaceExists(name), true);
    //Remove it to clean up properly
    Mantid::API::AnalysisDataService::Instance().remove(name);

  }

};

#endif /*MANTID_PYTHONFRAMEWORKTESTS_H_*/

