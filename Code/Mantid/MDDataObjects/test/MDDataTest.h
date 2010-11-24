#ifndef H_TEST_DND
#define H_TEST_DND


#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MDDataObjects/MDImageData.h"
#include "MantidAPI/FileFinder.h"
#include "Poco/Path.h"

using namespace Mantid;
using namespace API;
using namespace MDDataObjects;

class tesMDImageData :    public CxxTest::TestSuite
{
  MDImageData *pDND;
  std::vector<point3D> img;
  std::vector<unsigned int> selection;
  std::string test_file;
// std::string findTestFileLocation(void);
public:
  void testMDDataContstructor(void){
    std::string path=FileFinder::Instance().getFullPath("Mantid.properties");
    std::cout << " test properties location: "<< path<< std::endl;

    test_file=this->findTestFileLocation();
    
    pDND=new MDImageData();

    // 5-dim object;
    TSM_ASSERT_EQUALS("The expected number of dimensions were not found.", pDND->getNumDims(), 4);
  }

  void testRead_mdd(void){
    
    // read correct object, new 4D matlab format reader
    pDND->read_mdd(test_file.c_str(),false);

    TS_ASSERT_EQUALS(pDND->getNumDims(),4);
  }

  void testMDImageDataPrivateRead(void){

    // read correct object old 4D matlab format reader
    TS_ASSERT_THROWS_NOTHING(pDND->read_mdd(test_file.c_str(),true));

    TS_ASSERT_EQUALS(pDND->getNumDims(),4);
  }

  void testMDImageDataGet2DData(void){
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

  void test_alloc_mdd_arrays()
  {
    MDImageData imageData;
    MDGeometryDescription tt;

    imageData.alloc_mdd_arrays(tt);
    TSM_ASSERT("The Multi-dimensional image data structure should not be returned as null.", imageData.get_pMDData() != NULL);
    TSM_ASSERT("The Multi-dimensional point data should not be returned as null.", imageData.get_pData() != NULL);
  }

  tesMDImageData(void):pDND(NULL){}
  ~tesMDImageData(void){
    if(this->pDND)delete pDND;
    pDND=NULL;
  }
private:
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
};
#endif