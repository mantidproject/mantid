#ifndef H_CP_REBINNING
#define H_CP_REBINNING
#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MDWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidMDAlgorithms/CenterpieceRebinning.h"
#include "MantidMDAlgorithms/Load_MDWorkspace.h"


using namespace Mantid;
using namespace API;
using namespace Kernel;
using namespace MDDataObjects;
using namespace MDAlgorithms;

std::string findTestFileLocation(void);

class testCPRebinning :    public CxxTest::TestSuite
{
       MDWorkspace*  pOrigin;
 public:
    void testRebinInit(void){
 	//	 std::auto_ptr<IMD_FileFormat> pFile = MD_FileFormatFactory::getFileReader("../../../../Test/VATES/fe_demo.sqw",old_4DMatlabReader);
//    std::string dataFileName("../../../../Test/VATES/fe_demo.sqw");
//    std::string dataFileName("../../../../Test/VATES/fe_demo_bin.sqw");
    std::string dataFileName("../../../../Test/AutoTestData/test_horace_reader.sqw");

    Load_MDWorkspace loader;
    loader.initialize();
    loader.setPropertyValue("inFilename",dataFileName);
    std::string InputWorkspaceName = "MyTestMDWorkspace";
    loader.setPropertyValue("MDWorkspace",InputWorkspaceName);
    loader.execute();

    Workspace_sptr result=AnalysisDataService::Instance().retrieve(InputWorkspaceName);
    pOrigin = dynamic_cast<MDWorkspace *>(result.get());
    // no workspace loaded -- no point to continue
    if(!pOrigin)return;

    // test centerpiece rebinning 
     CenterpieceRebinning cpr;

     TS_ASSERT_THROWS_NOTHING(cpr.initialize());
     TS_ASSERT( cpr.isInitialized() );

   
      cpr.setPropertyValue("Input", InputWorkspaceName);      
      cpr.setPropertyValue("Result","OutWorkspace");

      
      // set slicing property to the size and shape of current workspace
      cpr.init_slicing_property();


    // retrieve slicing property for modifications
      Geometry::MDGeometryDescription *pSlicing = dynamic_cast< Geometry::MDGeometryDescription *>((Property *)(cpr.getProperty("SlicingData")));
      if(!pSlicing){
            throw(std::runtime_error("can not obtain slicing property from the property manager"));
      }

     // now modify it as we need;
        double r0=0;
        pSlicing->pDimDescription("qx")->cut_min = r0;
		pSlicing->pDimDescription("qx")->cut_max = r0+1;
		pSlicing->pDimDescription("qy")->cut_min = r0;
		pSlicing->pDimDescription("qy")->cut_max = r0+1;
		pSlicing->pDimDescription("qz")->cut_min = r0;
		pSlicing->pDimDescription("qz")->cut_max = r0+1;
		pSlicing->pDimDescription("en")->cut_max = 50;
        
   
        TS_ASSERT_THROWS_NOTHING(cpr.execute());
        pSlicing=NULL; 
    }
};


#endif