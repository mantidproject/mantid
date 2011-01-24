#ifndef CP_REBINNING_KEEPPIX_TEST
#define CP_REBINNING_KEEPPIX_TEST
#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MDWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidMDAlgorithms/CenterpieceRebinning.h"
#include "MDDataObjects/MD_FileFormatFactory.h"
#include <boost/shared_ptr.hpp>


using namespace Mantid;
using namespace API;
using namespace Kernel;
using namespace MDDataObjects;
using namespace MDAlgorithms;


MDWorkspace_sptr load_fake_workspace(std::string &wsName){
 // get the workspace and overwrite it if it already exists
    MDWorkspace_sptr inputWS;

    inputWS = MDWorkspace_sptr(new MDWorkspace());
    AnalysisDataService::Instance().addOrReplace(wsName, inputWS);
 

	// get the file reader providing test data
  std::auto_ptr<IMD_FileFormat> pFile = MD_FileFormatFactory::getFileReader(wsName.c_str(),test_data);
  if(!pFile.get()){
          throw(Kernel::Exception::FileError("can not get proper file reader",wsName));
   }
   std::auto_ptr<Geometry::MDGeometryBasis> pBasis = std::auto_ptr<Geometry::MDGeometryBasis>(new Geometry::MDGeometryBasis());
   
   pFile->read_basis(*pBasis);

   Geometry::MDGeometryDescription geomDescr(pBasis->getNumDims(),pBasis->getNumReciprocalDims());

   // read the geometry description
    pFile->read_MDGeomDescription(geomDescr);

	// obtain the MDPoint description now (and MDPointsDescription in a future)
	MDPointDescription pd = pFile->read_pointDescriptions();

    // clears pFile giving responsibility for it to the workspace
    inputWS->init(pFile,pBasis,geomDescr,pd);
 
	// get file reader for read operations
    IMD_FileFormat *pf = inputWS->get_pFileReader();

    pf->read_MDImg_data(*inputWS->get_spMDImage());

    inputWS->get_spMDDPoints()->init_pix_locations();


    return inputWS;

}

class CPRebinKeepPixTest :    public CxxTest::TestSuite
{
       std::string InputWorkspaceName;
   // test centerpiece rebinning 
       CenterpieceRebinning cpr;

       MDWorkspace_sptr spInputWS;
 public:
    void testRebinInit(void){

     InputWorkspaceName = "CPRebinKeepPixTestIn";

     TS_ASSERT_THROWS_NOTHING(cpr.initialize());
     TS_ASSERT( cpr.isInitialized() );

     TSM_ASSERT("We should be able to load the initial workspace successfully",(spInputWS=load_fake_workspace(InputWorkspaceName)).get()!=NULL);

   
      cpr.setPropertyValue("Input", InputWorkspaceName);      
      cpr.setPropertyValue("Result","CPRebinKeepPixTestOut");
      cpr.setProperty("KeepPixels",true);
      // set slicing property for the target workspace to the size and shape of the current workspace
      cpr.init_slicing_property();

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
		pSlicing->pDimDescription("ent")->cut_max = 1;
        pSlicing->pDimDescription("ent")->nBins   = 5;
        // sill too big; cut another 10%
        pSlicing->pDimDescription("qzt")->cut_min = r0;
		pSlicing->pDimDescription("qzt")->cut_max = r0+1;
        pSlicing->pDimDescription("qzt")->nBins   = 1;

//        pSlicing->pDimDescription("qyt")->cut_min = ;
		pSlicing->pDimDescription("qyt")->cut_max = 10;
        //pSlicing->pDimDescription("qyt")->nBins   = 1;

    
        TSM_ASSERT_THROWS_NOTHING("Good rebinning should not throw",cpr.execute());
    }
};


#endif
