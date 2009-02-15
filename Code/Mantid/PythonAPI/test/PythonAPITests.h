#ifndef MANTID_PYTHONAPITESTS_H_
#define MANTID_PYTHONAPITESTS_H_

#include <vector>

#include <cxxtest/TestSuite.h>
#include "../../Algorithms/test/WorkspaceCreationHelper.hh"

#include "MantidPythonAPI/PythonInterface.h"
#include "MantidPythonAPI/SimplePythonAPI.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "Poco/File.h"

using namespace Mantid::PythonAPI;
using namespace Mantid::API;

class PythonAPITest : public CxxTest::TestSuite
{

public:
  
  void testGetWorkspaceNames()
  {
    std::vector<std::string> temp = GetWorkspaceNames();
    TS_ASSERT(temp.empty());
    
    AnalysisDataService::Instance().add("outer",WorkspaceCreationHelper::Create2DWorkspace123(10,22,1));

    temp = GetWorkspaceNames();
    TS_ASSERT(!temp.empty());
    TS_ASSERT_EQUALS( temp[0], "outer" )
    FrameworkManager::Instance().deleteWorkspace("outer");
    temp = GetWorkspaceNames();
    TS_ASSERT(temp.empty());
  }
	
  void testGetAlgorithmNames()
  {
    std::vector<std::string> temp = GetAlgorithmNames();

    TS_ASSERT(!temp.empty());
  }

  void testCreatePythonSimpleAPI()
  {
    createPythonSimpleAPI(false);
    Poco::File apimodule(SimplePythonAPI::getModuleName());
    TS_ASSERT( apimodule.exists() );
    TS_ASSERT_THROWS_NOTHING( apimodule.remove() );
    TS_ASSERT( !apimodule.exists() );
  }

};

#endif /*MANTID_PYTHONAPITESTS_H_*/
