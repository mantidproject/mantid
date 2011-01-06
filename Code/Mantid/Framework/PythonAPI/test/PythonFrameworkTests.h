#ifndef  MANTID_PYTHONFRAMEWORKTESTS_H_
#define  MANTID_PYTHONFRAMEWORKTESTS_H_

#include <vector>

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidPythonAPI/FrameworkManagerProxy.h"
#include "MantidPythonAPI/SimplePythonAPI.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "Poco/File.h"

using namespace Mantid::PythonAPI;

bool FrameworkManagerProxy::g_gil_required = false;

class PythonFrameworkTests : public CxxTest::TestSuite
{

private:
  Mantid::PythonAPI::FrameworkManagerProxy* mgr;

public:

  PythonFrameworkTests()
  {
    mgr = new Mantid::PythonAPI::FrameworkManagerProxy;
  }

  void testCreateAlgorithmMethod1()
  {
    Mantid::API::IAlgorithm* alg = mgr->createAlgorithm("ConvertUnits");
    TS_ASSERT_EQUALS(alg->name(), "ConvertUnits");
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
    Mantid::API::MatrixWorkspace_sptr ws = WorkspaceCreationHelper::Create2DWorkspace123(10,22,1);
    ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");
    Mantid::API::AnalysisDataService::Instance().add("TestWorkspace1",ws);

    Mantid::API::IAlgorithm* alg = mgr->createAlgorithm("ConvertUnits", "TestWorkspace1;TestWorkspace1;DeltaE;Direct;10.5;0");
	  
    TS_ASSERT( alg->isInitialized() );
    TS_ASSERT( !alg->isExecuted() );
    TS_ASSERT_EQUALS( alg->getPropertyValue("Target"), "DeltaE" );
    TS_ASSERT_EQUALS( alg->getPropertyValue("EFixed"), "10.5" );

    Mantid::API::AnalysisDataService::Instance().clear();
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
	
  void testCreatePythonSimpleAPI()
  {
    mgr->createPythonSimpleAPI(false);
    Poco::File apimodule(SimplePythonAPI::getModuleFilename());
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

