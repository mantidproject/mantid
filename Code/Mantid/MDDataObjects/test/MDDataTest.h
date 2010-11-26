#ifndef H_TEST_DND
#define H_TEST_DND


#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MDDataObjects/MDImageData.h"
#include "MantidAPI/FileFinder.h"
#include "MDDataObjects/MDDataPoints.h"
#include "Poco/Path.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Mantid::API;
using namespace Mantid::MDDataObjects;

class tesMDImageData :    public CxxTest::TestSuite
{

private:
   static boost::shared_ptr<MDGeometry> getMDGeometry()
  {
    std::set<MDBasisDimension> basisDimensions;
    basisDimensions.insert(MDBasisDimension("q1", true, 1));
    basisDimensions.insert(MDBasisDimension("q2", true, 2));
    basisDimensions.insert(MDBasisDimension("q3", true, 3));
    basisDimensions.insert(MDBasisDimension("u1", false, 5));

    MDGeometryDescription description;
    for(unsigned int i=0;i<4;i++){
      description.setNumBins(i,50);
    }
    UnitCell cell;
    return boost::shared_ptr<MDGeometry>(new MDGeometry(MDGeometryBasis(basisDimensions, cell),description));
  } 

  class MockFileFormat : public IMD_FileFormat
  {
  public:

    MOCK_CONST_METHOD0(is_open, bool());
    virtual MDImageData* read_mdd()
    {
      boost::shared_ptr<MDGeometry> geometry = getMDGeometry();
      return new MDImageData(geometry);
    }

    MOCK_METHOD1(read_pix, bool(Mantid::MDDataObjects::MDDataPoints&)); 
    size_t read_pix_subset(const MDImageData &dnd,const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer)
    {
      return 0;
    }
    MOCK_METHOD0(getNPix, hsize_t());
    void write_mdd(const MDImageData &)
    {
    }
    virtual ~MockFileFormat(void){};
  };

 

  std::vector<point3D> img;
  std::vector<unsigned int> selection;

public:

  void testMDImageDataGet2DData(void){

    this->selection.assign(2,1);
    
    MockFileFormat file;
    std::auto_ptr<MDImageData> pDND=  std::auto_ptr<MDImageData>(file.read_mdd());

   
    // returns 2D image
    pDND->getPointData(selection,img);
    TS_ASSERT_EQUALS(img.size(),2500);

    // fails as we select 5 dimensions but the dataset is actually 4-D
    selection.assign(5,20);
    TS_ASSERT_THROWS_ANYTHING(pDND->getPointData(selection,img) );
  }

  void testGet3DData(void){

    // returns 3D image with 4-th dimension selected at 20;
    selection.assign(1,20);

    MockFileFormat file;
    std::auto_ptr<MDImageData>pDND= std::auto_ptr<MDImageData>(file.read_mdd());

//    std::auto_ptr<MDImageData> pDND=std::auto_ptr<MDImageData>(new MDImageData( getMDGeometry()));
//    MockFileFormat file;
//    file.read_mdd(*pDND);

    pDND->getPointData(selection,img);
    TS_ASSERT_EQUALS(img.size(),50*50*50); 
  }
  void testGet1Ddata(void){
    // this should return single point at (20,20,20,20)
    selection.assign(4,20);
    //std::auto_ptr<MDImageData> pDND=std::auto_ptr<MDImageData>(new MDImageData(getMDGeometry()));
    //MockFileFormat file;
    //file.read_mdd(*pDND);

    MockFileFormat file;
    std::auto_ptr<MDImageData>pDND= std::auto_ptr<MDImageData>(file.read_mdd());

    pDND->getPointData(selection,img);
    TS_ASSERT_EQUALS(img.size(),1);
  }
  void testGet2Ddata(void){
    // this should return line of size 50 
    selection.assign(3,10);
    //std::auto_ptr<MDImageData> pDND=std::auto_ptr<MDImageData>(new MDImageData(getMDGeometry()));
    //MockFileFormat file;
    //file.read_mdd(*pDND);
    MockFileFormat file;
    std::auto_ptr<MDImageData>pDND= std::auto_ptr<MDImageData>(file.read_mdd());

    pDND->getPointData(selection,img);
    TS_ASSERT_EQUALS(img.size(),50);
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