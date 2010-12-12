#ifndef H_CP_REBINNING
#define H_CP_REBINNING
#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MDWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidMDAlgorithms/CenterpieceRebinning.h"
#include "MDDataObjects/MD_FileFormatFactory.h"
#include "Poco/Path.h"

using namespace Mantid;
using namespace API;
using namespace Kernel;
using namespace MDDataObjects;
using namespace MDAlgorithms;

std::string findTestFileLocation(void);

class testCPRebinning :    public CxxTest::TestSuite
{
       MDWorkspace_sptr pOrigin;
 public:
    void testRebinInit(void){
       
        TS_ASSERT_THROWS_NOTHING(pOrigin = MDWorkspace_sptr(new MDWorkspace(4)));
        if(!pOrigin)return;

     
   
         AnalysisDataService::Instance().add("InWorkspace", pOrigin);

		 // should go to Load workspace algorithm ----->
		 std::auto_ptr<IMD_FileFormat> pFile = MD_FileFormatFactory::getFileReader("../../../../Test/VATES/fe_demo_bin.sqw");
		 boost::shared_ptr<IMD_FileFormat> spFile = boost::shared_ptr<IMD_FileFormat>(pFile.get());
		 pFile.release();

		 pOrigin->load_workspace(spFile);
		 //<----- should go to Load workspace algorithm 

        CenterpieceRebinning cpr;

         TS_ASSERT_THROWS_NOTHING(cpr.initialize());
         TS_ASSERT( cpr.isInitialized() );

         cpr.setPropertyValue("Filename", "new_datafile.sqw");
         cpr.setPropertyValue("Input", "InWorkspace");      
         cpr.setPropertyValue("Result","OutWorkspace");

      


         //Geometry::MDGeometryDescription *const pSlicing=cpr.pSlicingProperty();

        TS_ASSERT_THROWS_NOTHING(cpr.init_property(pOrigin));

		// get property from property service;
      //  std::string i1,i2;
      //  cpr.set_from_VISIT(i1,i2);
        Geometry::MDGeometryDescription *pSlicing = dynamic_cast< Geometry::MDGeometryDescription *>((Property *)(cpr.getProperty("SlicingData")));
        if(!pSlicing){
            throw(std::runtime_error("can not cast managed property to a slicing data"));
        }

        double r0=0;
		pSlicing->dimDescription("qx").cut_min = r0;
		pSlicing->dimDescription("qx").cut_min = r0+2;
		pSlicing->dimDescription("qy").cut_min = r0;
		pSlicing->dimDescription("qy").cut_max = r0+2;
		pSlicing->dimDescription("qz").cut_min = r0;
		pSlicing->dimDescription("qz").cut_max = r0+2;
		pSlicing->dimDescription("en").cut_max = 50;


   
        TS_ASSERT_THROWS_NOTHING(cpr.execute());
         pSlicing=NULL; 
    }
};

std::string findTestFileLocation(void){

       std::string path = Mantid::Kernel::getDirectoryOfExecutable();
        
        char pps[2];
        pps[0]=Poco::Path::separator();
        pps[1]=0; 
        std::string sps(pps);

        std::string root_path;
        size_t   nPos = path.find("Mantid"+sps+"Code");
        if(nPos==std::string::npos){
            std::cout <<" can not identify application location\n";
            root_path="../../../../Test/VATES/fe_demo.sqw";
        }else{
            root_path=path.substr(0,nPos)+"Mantid/Test/VATES/fe_demo.sqw";
        }
        std::cout << " test file location: "<< root_path<< std::endl;
        return root_path;
}

#endif