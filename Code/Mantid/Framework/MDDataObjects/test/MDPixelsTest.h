#ifndef H_TEST_SQW
#define H_TEST_SQW


#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MDDataPoints.h"
#include "MDDataObjects/IMD_FileFormat.h"
#include "MDDataObjects/MDImage.h"
#include "MDDataObjects/MDWorkspace.h"
#include "MantidAPI/FileFinder.h"
#include "MDDataObjects/MDImageDatatypes.h"
#include <Poco/Path.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/shared_ptr.hpp>

using namespace Mantid;
using namespace Mantid::MDDataObjects;

class MDPixelsTest :    public CxxTest::TestSuite
{

private:

  class  MockFileFormat : public IMD_FileFormat
  {
  public:
	  
	MockFileFormat(const char *file_name):IMD_FileFormat(file_name){}; 

	bool is_open()const{return false;}
	//MOCK_CONST_METHOD0(is_open, bool());           //
 //   MOCK_METHOD1(read_MDImg_data, void(MDImage&));  //
//   MOCK_METHOD1(read_pix, bool(MDDataPoints&));  //
	void read_MDImg_data(MDImage &){
	}

	bool read_pix(MDDataPoints&){
		return false;
	}
      
    size_t read_pix_subset(const MDImage &dnd,const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer)
    {
      return 0;
    }
//    MOCK_METHOD0(getNPix, uint64_t());
	uint64_t getNPix(){
		return 20000;
	}
    void read_MDGeomDescription(Geometry::MDGeometryDescription &){
	}
	void read_basis(Geometry::MDGeometryBasis &){
	}
	Mantid::MDDataObjects::MDPointDescription read_pointDescriptions(void)const{
		Mantid::MDDataObjects::MDPointDescription descr;
		return descr;
	}
	void write_mdd(const MDImage &){
	}
   //MOCK_METHOD1(read_MDGeomDescription,void(Geometry::MDGeometryDescription &));
   //MOCK_METHOD1(read_basis,void(Geometry::MDGeometryBasis &));
   //MOCK_CONST_METHOD0(read_pointDescriptions,Mantid::MDDataObjects::MDPointDescription(void));
   //MOCK_METHOD1(write_mdd,void(const MDImage &));


    virtual ~MockFileFormat(void){};
  };

  //Helper constructional method.
  static std::auto_ptr<MDImage> constructMDImage()
  {
    using namespace Mantid::Geometry;
    std::set<MDBasisDimension> basisDimensions;
    basisDimensions.insert(MDBasisDimension("q0", true, 0));
    basisDimensions.insert(MDBasisDimension("q1", true, 1));
    basisDimensions.insert(MDBasisDimension("q2", true, 2));
    basisDimensions.insert(MDBasisDimension("u3", false, 3));

    UnitCell cell;
    MDGeometry* pGeometry = new MDGeometry(MDGeometryBasis(basisDimensions, cell));
    return std::auto_ptr<MDImage>(new MDImage(pGeometry));
  }
  // helper variable providing default pixels description
  MDPointDescription pixDescription;
  std::auto_ptr<MDDataPoints> points;
public:
	void testGetBufferThrow(){
		points=std::auto_ptr<MDDataPoints>(new MDDataPoints(pixDescription));
	

		TSM_ASSERT_THROWS("non-initiated pointers should throw while trying get data buffer",points->get_pBuffer(1000),Kernel::Exception::NullPointerException);
	}


	void testInitExistingFile(){
       boost::shared_ptr<IMD_FileFormat> mockFileFormat = boost::shared_ptr<IMD_FileFormat>(new MockFileFormat("mock"));
	  // EXPECT_CALL(*dynamic_cast<MockFileFormat*>(mockFileFormat.get()), getNPix()).Times(1).WillOnce(testing::Return(20000));
	
	   std::auto_ptr<MDImage> img = constructMDImage();
	   TSM_ASSERT_THROWS_NOTHING("initialisation should not throw",points->initialize(boost::shared_ptr<MDImage>(img.release()),mockFileFormat));

	    std::vector<char> *data_buf;
		TSM_ASSERT_THROWS_NOTHING("allocating data buffer from initiated dataPoints should not throw",data_buf  = points->get_pBuffer(1000));

	}
	void testReallocateBuffer(){

		std::vector<char> *data_buf;
		TSM_ASSERT_THROWS_NOTHING("retrievimng allocated memory should not throw",data_buf  = points->get_pBuffer(1000));
		(*data_buf)[0]='B';
		// this should do nothing as nPoints = 0;
		std::vector<char> *buf_again;
	    TSM_ASSERT_THROWS_NOTHING("this should not change the buffer and should not throw",buf_again  = points->get_pBuffer(500));
		TSM_ASSERT_EQUALS("The data in the buffer should not change too",(*data_buf)[0],(*buf_again)[0]);
		TSM_ASSERT_EQUALS("The data in the buffer should not change",'B',(*buf_again)[0]);


		TSM_ASSERT_THROWS_NOTHING("memory should be re-allocated here and old data copied into it",buf_again  = points->get_pBuffer(2000));
		// but buffer will be nullified;
		TSM_ASSERT_EQUALS("The buffers have to be still the same",data_buf,buf_again);
		//TSM_ASSERT_EQUALS("The buffers size should change after allocation ",1000,points->get_pix_bufSize());
	}
 void testIsMemoryBased(void)
  {
    using namespace Mantid::Geometry;
    using namespace Mantid::MDDataObjects;
  
    MDDataPoints* ppoints=new MDDataPoints(pixDescription);

    TSM_ASSERT_EQUALS("Empty MDDataPoints should be in memory.", true, ppoints->isMemoryBased());
    delete ppoints;
  }

 /* void t__tGetPixels(void)
  {
    using namespace Mantid::Geometry;
    using namespace Mantid::MDDataObjects;

    MockFileFormat* mockFileFormat = new MockFileFormat("");
    EXPECT_CALL(*mockFileFormat, getNPix()).Times(1).WillOnce(testing::Return(100));

    MDDataPoints* points=new MDDataPoints(boost::shared_ptr<MDImage>(constructMDGeometry().release()),pixDescription);
    TSM_ASSERT_EQUALS("The number of pixels returned is not correct.", 100, points->getNumPixels(boost::shared_ptr<IMD_FileFormat>(mockFileFormat)));
    delete points;
  }*/

 /* void t__tConstructedBufferSize(void)
  {
    using namespace Mantid::Geometry;
    using namespace Mantid::MDDataObjects;

    MockFileFormat* mockFileFormat = new MockFileFormat("");
    MDDataPoints* points=new MDDataPoints(boost::shared_ptr<MDImage>(constructMDGeometry().release()),pixDescription);

    TSM_ASSERT_EQUALS("The memory buffer size following construction is not correct.", 0, points->getMemorySize());
    delete points;
  }*/

 

  //void t__tAllocation(void)
  //{
  //  using namespace Mantid::Geometry;
  //  using namespace Mantid::MDDataObjects;

  //  MockFileFormat* mockFileFormat = new MockFileFormat("");
  //  EXPECT_CALL(*mockFileFormat, getNPix()).Times(1).WillOnce(testing::Return(2));
  //  MDDataPoints* points=new MDDataPoints(boost::shared_ptr<MDImage>(constructMDGeometry().release()),pixDescription);
  //  points->alloc_pix_array(boost::shared_ptr<IMD_FileFormat>(mockFileFormat));
  //  TSM_ASSERT_EQUALS("The memory size is not the expected value after allocation.", 2, points->getMemorySize());
  //  delete points;
  //}



};

#endif
