#ifndef H_TEST_DND
#define H_TEST_DND


#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceFactory.h"
#include "MDDataObjects/MDData.h"
#include "find_mantid.h"

using namespace Mantid;
using namespace API;
using namespace MDDataObjects;

class tDND : public MDData
{
public:
    tDND(unsigned int nDims):MDData(nDims){};  
    void read_mdd(const char *file_name){MDData::read_mdd(file_name);}
};

class testDND :    public CxxTest::TestSuite
{
    tDND *pDND;
    std::vector<point3D> img;
    std::vector<unsigned int> selection;
    std::string test_file;
public:
    void testMDDataContstructor(void){
        std::string path=FileFinder::Instance().getFullPath("Mantid.properties");
        std::cout << " test properties location: "<< path<< std::endl;

        test_file=findTestFileLocation();
 
      // define an 5D (wrong) object 
        TS_ASSERT_THROWS_NOTHING(pDND=new tDND(5));

       // 5-dim object;
        TS_ASSERT_EQUALS(pDND->getNumDims(),5);
    }
    void testIMDRegistration(){
         IMDWorkspace_sptr ws;
         MDGeometryDescription data;
         TS_ASSERT_THROWS_NOTHING( ws = API::WorkspaceFactory::Instance().create("MDWorkspacet",data));
 
    }
    void testDNDPrivateRead(void){
 
        // read correct object
        TS_ASSERT_THROWS_NOTHING(pDND->read_mdd(test_file.c_str()));

        TS_ASSERT_EQUALS(pDND->getNumDims(),4);
    }
    void testDNDGet2DData(void){
        this->selection.assign(2,1);
   
        // returns 2D image
        TS_ASSERT_THROWS_NOTHING(pDND->getPointData(selection,img) );
        TS_ASSERT_EQUALS(img.size(),2500);

        // fails as we select 5 dimensions but the dataset is actually 4-D
        selection.assign(5,20);
        TS_ASSERT_THROWS_ANYTHING(pDND->getPointData(selection,img) );
    }
    void testGet3DData(void){

        // returns 3D image with 4-th dimension selected at 20;
        selection.assign(1,20);
        TS_ASSERT_THROWS_NOTHING(pDND->getPointData(selection,img) );
        TS_ASSERT_EQUALS(img.size(),50*50*50); 
    }
    void testGet1Ddata(void){
        // this should return single point at (20,20,20,20)
        selection.assign(4,20);
        TS_ASSERT_THROWS_NOTHING(pDND->getPointData(selection,img) );
        TS_ASSERT_EQUALS(img.size(),1);
    }
   void testGet2Ddata(void){
        // this should return line of size 50 
        selection.assign(3,10);
        TS_ASSERT_THROWS_NOTHING(pDND->getPointData(selection,img) );
        TS_ASSERT_EQUALS(img.size(),50);
    }
 
     testDND(void):pDND(NULL){}
    ~testDND(void){
        if(this->pDND)delete pDND;
        pDND=NULL;
    }
};
#endif