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

    // clears pFile
    inputWS->init(pFile,pBasis,geomDescr,pd);
 
    IMD_FileFormat *pf = inputWS->get_pFileReader();

    pf->read_MDImg_data(*inputWS->get_spMDImage());
    //TODO: should be moved into datapoints;
    inputWS->get_spMDImage()->identify_SP_points_locations();


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

     InputWorkspaceName = "testRebinMDWorkspace";

     TS_ASSERT_THROWS_NOTHING(cpr.initialize());
     TS_ASSERT( cpr.isInitialized() );

     TSM_ASSERT("We should be able to load the initial workspace successfully",(spInputWS=load_fake_workspace(InputWorkspaceName)).get()!=NULL);

   
      cpr.setPropertyValue("Input", InputWorkspaceName);      
      cpr.setPropertyValue("Result","OutWorkspace");
      cpr.setProperty("KeepPixels",true);
      // set slicing property for the target workspace to the size and shape of the current workspace
      cpr.init_slicing_property();

    }
};


#endif
