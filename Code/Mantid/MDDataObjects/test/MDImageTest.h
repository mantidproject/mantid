#ifndef H_TEST_DND
#define H_TEST_DND


#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MDDataObjects/MDImage.h"
#include "MantidAPI/FileFinder.h"
#include "MDDataObjects/MDDataPoints.h"
#include "Poco/Path.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace Mantid::MDDataObjects;
using namespace Mantid::Geometry;

class tesMDImage :    public CxxTest::TestSuite
{

private:

  class MockFileFormat : public IMD_FileFormat
  {
  public:

    MOCK_CONST_METHOD0(is_open, bool());
    virtual void read_mdd(Mantid::MDDataObjects::MDImage& dnd)
    {
       Mantid::Geometry::MDGeometryDescription dnd_shape(4);
       for(int i=0;i<4;i++){
         dnd_shape.setNumBins(i,50);
       }
       dnd.alloc_mdd_arrays(dnd_shape);
    }
    MOCK_METHOD1(read_pix, bool(Mantid::MDDataObjects::MDDataPoints&)); 
    size_t read_pix_subset(const MDImage &dnd,const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer)
    {
      return 0;
    }
    MOCK_METHOD0(getNPix, hsize_t());
    void write_mdd(const MDImage &)
    {
    }
    virtual ~MockFileFormat(void){};
  };

  static std::auto_ptr<MDGeometry> getMDGeometry()
  {
    std::set<MDBasisDimension> basisDimensions;
    basisDimensions.insert(MDBasisDimension("q1", true, 1));
    basisDimensions.insert(MDBasisDimension("q2", true, 2));
    basisDimensions.insert(MDBasisDimension("q3", true, 3));
    basisDimensions.insert(MDBasisDimension("u1", false, 5));

    UnitCell cell;
    return std::auto_ptr<MDGeometry>(new MDGeometry(MDGeometryBasis(basisDimensions, cell)));
  } 

  std::vector<point3D> img;
  std::vector<unsigned int> selection;

public:

  void testMDImageGet2DData(void){

    this->selection.assign(2,1);

    MDImage* pImageData = new MDImage(getMDGeometry().release());
    std::auto_ptr<MDImage>pDND=std::auto_ptr<MDImage>(pImageData);
    MockFileFormat file;
    file.read_mdd(*pDND);

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
    MDImage* pImageData = new MDImage(getMDGeometry().release());
    std::auto_ptr<MDImage>pDND=std::auto_ptr<MDImage>(pImageData);

    file.read_mdd(*pDND);

    pDND->getPointData(selection,img);
    TS_ASSERT_EQUALS(img.size(),50*50*50); 
  }

  void testGet1Ddata(void){
    // this should return single point at (20,20,20,20)
    selection.assign(4,20);
    std::auto_ptr<MDImage> pDND=std::auto_ptr<MDImage>(new MDImage(getMDGeometry().release()));
    MockFileFormat file;
    file.read_mdd(*pDND);
    pDND->getPointData(selection,img);
    TS_ASSERT_EQUALS(img.size(),1);
  }

  void testGet2Ddata(void){
    // this should return line of size 50 
    selection.assign(3,10);
    std::auto_ptr<MDImage> pDND=std::auto_ptr<MDImage>(new MDImage(getMDGeometry().release()));
    MockFileFormat file;
    file.read_mdd(*pDND);
    pDND->getPointData(selection,img);
    TS_ASSERT_EQUALS(img.size(),50);
  }

  void test_alloc_mdd_arrays()
  {
    std::auto_ptr<MDImage>pMDImage = std::auto_ptr<MDImage>(new MDImage(getMDGeometry().release()));
    MDGeometryDescription tt;

    pMDImage->alloc_mdd_arrays(tt);
    TSM_ASSERT("The Multi-dimensional image data structure should not be returned as null.", pMDImage->get_pMDImgData() != NULL);
    TSM_ASSERT("The Multi-dimensional point data should not be returned as null.", pMDImage->get_pData() != NULL);
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