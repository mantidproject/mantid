#ifndef LOAD_MDWORKSPACE_TEST_H
#define LOAD_MDWORKSPACE_TEST_H
#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MDWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidMDAlgorithms/Load_MDWorkspace.h"
#include "Poco/Path.h"

using namespace Mantid;
using namespace API;
using namespace Kernel;
using namespace MDDataObjects;
using namespace MDAlgorithms;



class Load_MDWorkspaceTest :    public CxxTest::TestSuite
{
   Load_MDWorkspace loader;
   std::string targetWorkspaceName;
 public:
     void testLoadMDWSInit(){
        TSM_ASSERT_THROWS_NOTHING("loader should initialize without throwing",loader.initialize());
        TSM_ASSERT("Loader should be initialized before going any further",loader.isInitialized());
     }
     void testLoadMDWSParams(){
         // Should fail because mandatory parameter has not been set
        TSM_ASSERT_THROWS("The loader should throw now as necessary parameters have not been set",loader.execute(),std::runtime_error);
         
        // this should throw if file does not exist
        TSM_ASSERT_THROWS("This file should not exist",loader.setPropertyValue("inFilename","../Test/AutoTestData/test_horace_reader.sqw"),std::invalid_argument);
        // and so this
        TSM_ASSERT_THROWS_NOTHING("The test file should exist",loader.setPropertyValue("inFilename","../../../../Test/AutoTestData/test_horace_reader.sqw"));
     }
     void testMDWSExec(){
         // does it add it to analysis data service? -- no
         targetWorkspaceName = "MyTestWorkspace";
         loader.setPropertyValue("MDWorkspace",targetWorkspaceName);

         TSM_ASSERT_THROWS_NOTHING("workspace loading should not throw",loader.execute());
     }
     void testMDWSDoneWell(){
         // now verify if we loaded the right thing
         Workspace_sptr result;
         TSM_ASSERT_THROWS_NOTHING("We should retrieve loaded workspace without throwing",result=AnalysisDataService::Instance().retrieve(targetWorkspaceName));

         MDWorkspace_sptr loadedWS=boost::dynamic_pointer_cast<MDWorkspace>(result);
         TSM_ASSERT("MD workspace has not been casted coorectly",loadedWS.get()!=0);
         //
         TSM_ASSERT_EQUALS("The workspace should be 4D",4,loadedWS->getNumDims());

         TSM_ASSERT_EQUALS("The number of pixels contributed into this workspace should be 1523850",1523850,loadedWS->getNPoints());

         TSM_ASSERT_EQUALS("The MD image in this workspace has to had 64 data cells",64,loadedWS->get_const_MDImage().getDataSize());
     }

};
#endif
