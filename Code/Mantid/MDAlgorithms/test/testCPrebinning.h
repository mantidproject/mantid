#ifndef H_CP_REBINNING
#define H_CP_REBINNING
#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MDWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidMDAlgorithms/CenterpieceRebinning.h"
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

     
    //    TS_ASSERT_THROWS_NOTHING(pOrigin->read_mdd(location.c_str()));
         AnalysisDataService::Instance().add("InWorkspace", pOrigin);


        CenterpieceRebinning cpr;

         TS_ASSERT_THROWS_NOTHING(cpr.initialize());
         TS_ASSERT( cpr.isInitialized() );

         cpr.setPropertyValue("Filename", "../../../../Test/VATES/fe_demo.sqw");
         cpr.setPropertyValue("Input", "InWorkspace");      
         cpr.setPropertyValue("Result","OutWorkspace");

      


         //Geometry::MDGeometryDescription *const pSlicing=cpr.pSlicingProperty();

        TS_ASSERT_THROWS_NOTHING(cpr.init_source(pOrigin));

      //  std::string i1,i2;
      //  cpr.set_from_VISIT(i1,i2);
        Geometry::MDGeometryDescription *pSlicing = dynamic_cast< Geometry::MDGeometryDescription *>((Property *)(cpr.getProperty("SlicingData")));
        if(!pSlicing){
            throw(std::runtime_error("can not cast managed property to a slicing data"));
        }

        double r0=0;
        pSlicing->setCutMin("q1",r0);
        pSlicing->setCutMax("q1",r0+2);
        pSlicing->setCutMin("q2",r0);
        pSlicing->setCutMax("q2",r0+2);
        pSlicing->setCutMin("q3",r0);
        pSlicing->setCutMax("q3",r0+2);
        pSlicing->setCutMax("en",50);

   
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