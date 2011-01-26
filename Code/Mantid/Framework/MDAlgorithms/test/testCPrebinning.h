#ifndef H_CP_REBINNING
#define H_CP_REBINNING
#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MDWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidMDAlgorithms/CenterpieceRebinning.h"
#include "MantidMDAlgorithms/Load_MDWorkspace.h"
#include <boost/shared_ptr.hpp>


using namespace Mantid;
using namespace API;
using namespace Kernel;
using namespace MDDataObjects;
using namespace MDAlgorithms;

std::string findTestFileLocation(void);

bool load_existing_workspace(const std::string &workspace_name){
// helper function to load a workpsace -- something a user should do before rebinning

	//	 std::auto_ptr<IMD_FileFormat> pFile = MD_FileFormatFactory::getFileReader("../../../../../Test/VATES/fe_demo.sqw",old_4DMatlabReader);
//    std::string dataFileName("../../../../../Test/VATES/fe_demo.sqw");
//    std::string dataFileName("../../../../../Test/VATES/fe_demo_bin.sqw");
    std::string dataFileName("test_horace_reader.sqw");
//        std::string dataFileName("fe_E800_8K.sqw");
    Load_MDWorkspace loader;
    loader.initialize();
    loader.setPropertyValue("inFilename",dataFileName);
 
    loader.setPropertyValue("MDWorkspace",workspace_name);
    loader.execute();

    Workspace_sptr result=AnalysisDataService::Instance().retrieve(workspace_name);
    MDWorkspace*  pOrigin = dynamic_cast<MDWorkspace *>(result.get());
    // no workspace loaded -- no point to continue
    if(!pOrigin)return false;

    return true;

}

class testCPrebinning :    public CxxTest::TestSuite
{
       std::string InputWorkspaceName;
       std::string OutWorkspaceName;
   // test centerpiece rebinning 
       CenterpieceRebinning cpr;
 public:
    void testRebinInit(void){

     InputWorkspaceName = "testCPrebinningIn";
     OutWorkspaceName   = "testCPrebinningOut";

     TS_ASSERT_THROWS_NOTHING(cpr.initialize());
     TS_ASSERT( cpr.isInitialized() );

     TSM_ASSERT("We should be able to load the initial workspace successfully",load_existing_workspace(InputWorkspaceName));

   
      cpr.setPropertyValue("Input", InputWorkspaceName);      
      cpr.setPropertyValue("Result",OutWorkspaceName);
      cpr.setProperty("KeepPixels",false);
      // set slicing property for the target workspace to the size and shape of the current workspace
      cpr.setTargetGeomDescrEqSource();

    }
    void testGetSlicingProperty(){      

    // retrieve slicing property for modifications
      Geometry::MDGeometryDescription *pSlicing = dynamic_cast< Geometry::MDGeometryDescription *>((Property *)(cpr.getProperty("SlicingData")));
      TSM_ASSERT("Slicing property should be easy obtainable from property manager",pSlicing!=0)
  
    }
    void testCPRExec(){
        // rebin image into the same grid as an initial image, which should provide the same image as before
        TSM_ASSERT_THROWS_NOTHING("Good rebinning should not throw",cpr.execute());
    }
    void testRebinnedWSExists(){
        // now test if we can get the resulting workspace
        Workspace_sptr rezWS = AnalysisDataService::Instance().retrieve(OutWorkspaceName);

        MDWorkspace_sptr targetWS = boost::dynamic_pointer_cast<MDWorkspace>(rezWS);
        TSM_ASSERT("The workspace obtained is not target MD workspace",targetWS!=0);

    }
    void testEqRebinCorrectness(){
        // we did rebinning on the full image and initial image have to be equal to the target image; Let's compare them
	    MDWorkspace_sptr inputWS, outWS;
        // Get the workspaces
        inputWS = boost::dynamic_pointer_cast<MDWorkspace>(AnalysisDataService::Instance().retrieve(InputWorkspaceName));
        outWS   = boost::dynamic_pointer_cast<MDWorkspace>(AnalysisDataService::Instance().retrieve(OutWorkspaceName));
         
        const MDImage &OldIMG = inputWS->get_const_MDImage();
        const MDImage &NewIMG = outWS->get_const_MDImage();

        for(size_t i=0;i<OldIMG.getDataSize();i++){
            TSM_ASSERT_DELTA("Old and new images points in this case have to be equal",OldIMG.getSignal(i),NewIMG.getSignal(i),1.e-4);
        }
    }

    void testCPRRebinAgainSmaller(){
   // now rebin into slice
  // retrieve slicing property for modifications
      Geometry::MDGeometryDescription *pSlicing = dynamic_cast< Geometry::MDGeometryDescription *>((Property *)(cpr.getProperty("SlicingData")));
      TSM_ASSERT("Slicing property should be easy obtainable from property manager",pSlicing!=0)

   // now modify it as we need/want;
        double r0=-1;
        pSlicing->pDimDescription("qx")->cut_min = r0;
		pSlicing->pDimDescription("qx")->cut_max = r0+1;
		pSlicing->pDimDescription("qy")->cut_min = r0;
		pSlicing->pDimDescription("qy")->cut_max = r0+1;
		pSlicing->pDimDescription("qz")->cut_min = r0;
		pSlicing->pDimDescription("qz")->cut_max = r0+1;
		pSlicing->pDimDescription("en")->cut_max = 50;
  
   
        TSM_ASSERT_THROWS_NOTHING("Good rebinning should not throw",cpr.execute());
    }
};


#endif
