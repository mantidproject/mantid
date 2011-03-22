#ifndef CP_REBINNING_KEEPPIX_TEST
#define CP_REBINNING_KEEPPIX_TEST
#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MDWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidMDAlgorithms/CenterpieceRebinning.h"
#include "MDDataObjects/MD_FileFormatFactory.h"
#include "MDDataObjects/MDTestWorkspace.h"
#include <boost/shared_ptr.hpp>


using namespace Mantid;
using namespace API;
using namespace Kernel;
using namespace MDDataObjects;
using namespace MDAlgorithms;




class CPRebinKeepPixTest :    public CxxTest::TestSuite
{
       std::string InputWorkspaceName;
   // test centerpiece rebinning 
       CenterpieceRebinning cpr;

       MDWorkspace_sptr spInputWS;
 public:
    void testRebinInit(void){
	// build test workspace
      std::auto_ptr<MDTestWorkspace> tw = std::auto_ptr<MDTestWorkspace>(new MDTestWorkspace());
    // get usual workspace from the test workspace
      spInputWS = tw->get_spWS();

      InputWorkspaceName = "CPRebinKeepPixTestIn";
      AnalysisDataService::Instance().addOrReplace(InputWorkspaceName,spInputWS);

     TS_ASSERT_THROWS_NOTHING(cpr.initialize());
     TS_ASSERT( cpr.isInitialized() );

   
      cpr.setPropertyValue("Input", InputWorkspaceName);      
      cpr.setPropertyValue("Result","CPRebinKeepPixTestOut");
      cpr.setProperty("KeepPixels",true);
      // set slicing property for the target workspace to the size and shape of the current workspace
      cpr.setTargetGeomDescrEqSource();

    }

 void testCPRebinKeepPixels(){
   // now rebin into slice
  // retrieve slicing property for modifications
      Geometry::MDGeometryDescription *pSlicing = dynamic_cast< Geometry::MDGeometryDescription *>((Property *)(cpr.getProperty("SlicingData")));
      TSM_ASSERT("Slicing property should be easy obtainable from property manager",pSlicing!=0)


   // now modify it as we need/want;
        double r0=0;
  //      pSlicing->pDimDescription("qx")->cut_min = r0;
		//pSlicing->pDimDescription("qx")->cut_max = r0+1;
		//pSlicing->pDimDescription("qy")->cut_min = r0;
		//pSlicing->pDimDescription("qy")->cut_max = r0+1;
		//pSlicing->pDimDescription("qz")->cut_min = r0;
		//pSlicing->pDimDescription("qz")->cut_max = r0+1;
      // All data go from -1 to 49;
      // take 10%
		pSlicing->pDimDescription("ent")->cut_max = 0;
        pSlicing->pDimDescription("ent")->nBins   = 5;
        // sill too big; cut another 10%
//        pSlicing->pDimDescription("qzt")->cut_min = r0;
		pSlicing->pDimDescription("qzt")->cut_max = 1;
        pSlicing->pDimDescription("qzt")->nBins   = 1;

//        pSlicing->pDimDescription("qyt")->cut_min = ;
//		pSlicing->pDimDescription("qyt")->cut_max = 10;
        //pSlicing->pDimDescription("qyt")->nBins   = 1;

    
        TSM_ASSERT_THROWS_NOTHING("Good rebinning should not throw",cpr.execute());
    }
	~CPRebinKeepPixTest(){
	}
};


#endif
