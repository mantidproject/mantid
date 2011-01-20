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
   MDWorkspace *pLoadedWS;
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
        TSM_ASSERT_THROWS_NOTHING("The test file should exist",loader.setPropertyValue("inFilename","test_horace_reader.sqw"));
      //  TSM_ASSERT_THROWS_NOTHING("The test file should exist",loader.setPropertyValue("inFilename","fe_demo.sqw"));
     }
     void testMDWSExec(){
         // does it add it to analysis data service? -- no
         targetWorkspaceName = "Load_MDWorkspaceTestWorkspace";
         loader.setPropertyValue("MDWorkspace",targetWorkspaceName);

         TSM_ASSERT_THROWS_NOTHING("workspace loading should not throw",loader.execute());
     }
     void testMDWSDoneWell(){
         // now verify if we loaded the right thing
         Workspace_sptr result;
         TSM_ASSERT_THROWS_NOTHING("We should retrieve loaded workspace without throwing",result=AnalysisDataService::Instance().retrieve(targetWorkspaceName));

         MDWorkspace_sptr sp_loadedWS=boost::dynamic_pointer_cast<MDWorkspace>(result);
         TSM_ASSERT("MD workspace has not been casted coorectly",sp_loadedWS.get()!=0);

         pLoadedWS   = sp_loadedWS.get();

         //
         TSM_ASSERT_EQUALS("The workspace should be 4D",4,pLoadedWS->getNumDims());

         TSM_ASSERT_EQUALS("The number of pixels contributed into this workspace should be 1523850",1523850,pLoadedWS->getNPoints());

         TSM_ASSERT_EQUALS("The MD image in this workspace has to had 64 data cells",64,pLoadedWS->get_const_MDImage().getDataSize());
     }
     void testMDImageCorrect(){
         // if the image we've loaded is correct image (the same as we've put there)
         MDImage &IMG = pLoadedWS->get_const_MDImage();
         std::vector<point3D> img_data;
         std::vector<unsigned int> selection(2,0);

         IMG.getPointData(selection,img_data);
         double sum(0);
         for(size_t i=0;i<img_data.size();i++){
             sum+=img_data[i].S();
         }
         TSM_ASSERT_DELTA("The signal in this cell should be specified value",0.65789,img_data[0 ].S(),1.e-4);
         TSM_ASSERT_DELTA("The signal in this cell should be specified value",0.37786,img_data[10].S(),1.e-4);
         TSM_ASSERT_DELTA("The signal in this cell should be specified value",0.0,    img_data[15].S(),1.e-4);
         TSM_ASSERT_DELTA("The sum of all signals in the signals selection should be specific value",7.3273,    sum,1.e-4);

         selection[0]=1;
         selection[1]=1;
         IMG.getPointData(selection,img_data);

         sum = 0;
         for(size_t i=0;i<img_data.size();i++){
             sum+=img_data[i].S();
         }

         TSM_ASSERT_DELTA("The signal in this cell should be specified value",0,      img_data[ 0].S(),1.e-4);
         TSM_ASSERT_DELTA("The signal in this cell should be specified value",0.25612,img_data[ 1].S(),1.e-4);
         TSM_ASSERT_DELTA("The signal in this cell should be specified value",0.15172,img_data[15].S(),1.e-4);

         TSM_ASSERT_DELTA("The sum of all signals in the signals selection should be specific value",2.52227, sum,1.e-4);

  
     }

~Load_MDWorkspaceTest(){
}

};
#endif
