#ifndef H_TEST_DND
#define H_TEST_DND


#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MDDataObjects/MDImage.h"
#include "MantidAPI/FileFinder.h"
#include "MDDataObjects/MDDataPoints.h"
#include <Poco/Path.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace Mantid::MDDataObjects;
using namespace Mantid::Geometry;

class MDImageTest :    public CxxTest::TestSuite
{

private:

  class MockFileFormat : public IMD_FileFormat
  {
  public:
  
	MockFileFormat(const char *file_name):IMD_FileFormat(file_name){};
    MOCK_CONST_METHOD0(is_open, bool());
    virtual void read_MDImg_data(Mantid::MDDataObjects::MDImage& dnd)
    {
		// this function fills data arrays with values obtained from Hdd
    }
    MOCK_METHOD1(read_pix, bool(Mantid::MDDataObjects::MDDataPoints&)); 
    size_t read_pix_subset(const MDImage &dnd,const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer)
    {
      return 0;
    }
    MOCK_METHOD0(getNPix, uint64_t());
    void write_mdd(const MDImage &)
    {
    }
   MOCK_METHOD1(read_basis,void(Mantid::Geometry::MDGeometryBasis &));
   void read_MDGeomDescription(Mantid::Geometry::MDGeometryDescription &description){
	  using namespace Mantid::Geometry;
	  for(int i=0;i<description.getNumDims();i++){
		  description.pDimDescription(i)->nBins=50;
	  }

   }
  
   MOCK_CONST_METHOD0(read_pointDescriptions,Mantid::MDDataObjects::MDPointDescription(void));

    virtual ~MockFileFormat(void){};
  };

  static std::auto_ptr<MDGeometry> getMDGeometry()
  {
    std::set<MDBasisDimension> basisDimensions;
    basisDimensions.insert(MDBasisDimension("q1", true, 0));
    basisDimensions.insert(MDBasisDimension("q2", true, 1));
    basisDimensions.insert(MDBasisDimension("q3", true, 2));
    basisDimensions.insert(MDBasisDimension("u1", false, 3));

    UnitCell cell;
    return std::auto_ptr<MDGeometry>(new MDGeometry(MDGeometryBasis(basisDimensions, cell)));
  } 
//
  void setFakeImageValuesIncompletely(){
	  MD_image_point *pData = pImage->get_pData();
	  size_t nCells =         pImage->getDataSize();
      for(size_t  j=0;j<nCells;j++){
		  	pData[j].s=(double)j;
			pData[j].npix=j;
	   }

  }

  std::vector<point3D> img;
  std::vector<unsigned int> selection;
  std::auto_ptr<MDImage> pImage;

public:
	void testMDImageConstructorEmptyDefault(){
		std::auto_ptr<MDImage> pImg;
		TSM_ASSERT_THROWS_NOTHING("MDImage constructor builds empty object and should not throw",pImg = std::auto_ptr<MDImage>(new MDImage()));

		TSM_ASSERT_EQUALS("emtpy image should not be initialized",false,pImg->is_initialized());
	}
	void testMDImageConstructorFromEmptyGeometry(){
		// an image initiated by an emtpy geometry is not empty and consists of one cell
		std::auto_ptr<MDImage> pImg;
		std::auto_ptr<MDGeometry> pGeom = getMDGeometry();
		TSM_ASSERT_THROWS_NOTHING("MDImage constructor builds Image object and should not throw",pImg = std::auto_ptr<MDImage>(new MDImage(pGeom.get())));
		// the responsibility for geometry is now with Image
		pGeom.release();
		//
		TSM_ASSERT_EQUALS("Image with emtpy geometry should be initialized",true,pImg->is_initialized());
		TSM_ASSERT_EQUALS("An image with empty geometry should be size 1 ",1,pImg->getDataSize());
	}
	void testMDImageWrongInitiation(){
		// this construction wis used nelow
	    pImage = std::auto_ptr<MDImage>(new MDImage(getMDGeometry().release()));
		Mantid::Geometry::MDGeometryDescription geom_description(5,3);	
		TSM_ASSERT_THROWS("Geometry is initiated by 4x3 basis and should not be possible to initiate it with different N-dims",pImage->initialize(geom_description),std::invalid_argument);

		TSM_ASSERT_EQUALS("Image with emtpy geometry should be initialized",true,pImage->is_initialized());
		TSM_ASSERT_EQUALS("An image with empty geometry should be size 1 ",1,pImage->getDataSize());


	}
	void testMDImageReadDescription(){
		MockFileFormat file("");

        Mantid::Geometry::MDGeometryDescription geom_description(4,3);
		file.read_MDGeomDescription(geom_description);
		TS_ASSERT_THROWS_NOTHING(pImage->initialize(geom_description));

		TSM_ASSERT_EQUALS("An image with this geometry should be specific size ",50*50*50*50,pImage->getDataSize());
	}
  void testGet2DData(void){

    this->selection.assign(2,1);

    // returns 2D image 
  
    pImage->getPointData(selection,img);
    TS_ASSERT_EQUALS(img.size(),2500);
  }

  void testExpandedSelectionFails(){

    // fails as we select 5 dimensions but the dataset is actually 4-D
    selection.assign(5,20);
    TS_ASSERT_THROWS_ANYTHING(pImage->getPointData(selection,img) );
  }

  void testGet3DData(void){

    // returns 3D image with 4-th dimension selected at 20;
    selection.assign(1,20);

  
    pImage->getPointData(selection,img);
    TS_ASSERT_EQUALS(img.size(),50*50*50); 
  }

  void testGet0Ddata(void){
    // this should return single point at (20,20,20,20)
    selection.assign(4,20);

    pImage->getPointData(selection,img);
    TS_ASSERT_EQUALS(img.size(),1);
  }

  void testGet1Ddata(void){
    // this should return line of size 50 
    selection.assign(3,10);
 
    pImage->getPointData(selection,img);
    TS_ASSERT_EQUALS(img.size(),50);
  }
  void testSetValues(){
	  this->setFakeImageValuesIncompletely();
	  TSM_ASSERT_THROWS("This validation should throw as the npix control sum has not been set properly",pImage->validateNPix(),std::invalid_argument);
  }
  void testValidationSetCorrect(){
	  TSM_ASSERT_THROWS_NOTHING("This should not throw as the previous check set correct number of pixels contributed",pImage->validateNPix());
  }
  void testNpixCorrect(){
	  size_t nCells = pImage->getDataSize();
	  uint64_t nPix = ((uint64_t)nCells)*(nCells-1)/2;
	  TSM_ASSERT_EQUALS("setFakeImage function set number of contributing pixels which is not consistent with the number expected or getNMDDPoints returned wrong number",nPix,pImage->getNMDDPoints());
  }

private:

};
#endif
