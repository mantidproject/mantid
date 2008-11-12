#ifndef MANTID_PYTHONAPITESTS_H_
#define MANTID_PYTHONAPITESTS_H_

#include <vector>

#include <cxxtest/TestSuite.h>

#include "MantidPythonAPI/PythonInterface.h"
#include "MantidPythonAPI/SimplePythonAPI.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "boost/filesystem.hpp"

using namespace Mantid::PythonAPI;
using namespace Mantid::API;

class PythonAPITest : public CxxTest::TestSuite
{

public:
  
  PythonAPITest()
  {
  }
  
  void testGetWorkspaceNames()
  {
    std::vector<std::string> temp = GetWorkspaceNames();
    TS_ASSERT(temp.empty());
    
    //Run an algorithm to create a workspace
    IAlgorithm* loader = FrameworkManager::Instance().createAlgorithm("LoadRaw");
    loader->setPropertyValue("Filename", "../../../../Test/Data/GEM38370.raw");
    loader->setPropertyValue("OutputWorkspace", "outer");    
    loader->execute();

    temp = GetWorkspaceNames();
    TS_ASSERT(!temp.empty());
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
    createPythonSimpleAPI();
    TS_ASSERT(boost::filesystem::exists(SimplePythonAPI::getModuleName()));
    boost::filesystem::remove_all(SimplePythonAPI::getModuleName());
  }

};

#endif /*MANTID_PYTHONAPITESTS_H_*/
