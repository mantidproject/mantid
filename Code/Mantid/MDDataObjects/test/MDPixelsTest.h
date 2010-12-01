#ifndef H_TEST_SQW
#define H_TEST_SQW


#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MDDataPoints.h"
#include "MDDataObjects/IMD_FileFormat.h"
#include "MDDataObjects/MDImage.h"
#include "MDDataObjects/MDWorkspace.h"
#include "MantidAPI/FileFinder.h"
#include "MDDataObjects/point3D.h"
#include "Poco/Path.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/shared_ptr.hpp>

using namespace Mantid::MDDataObjects;

class testMultiDimensionalDataPoints :    public CxxTest::TestSuite
{

private:

  class MockFileFormat : public IMD_FileFormat
  {
  public:

    MOCK_CONST_METHOD0(is_open, bool());
    MOCK_METHOD1(read_mdd, void(Mantid::MDDataObjects::MDImage&)); 
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

  //Helper constructional method.
  static std::auto_ptr<Mantid::MDDataObjects::MDImage> constructMDGeometry()
  {
    using namespace Mantid::Geometry;
    std::set<MDBasisDimension> basisDimensions;
    basisDimensions.insert(MDBasisDimension("q1", true, 1));
    basisDimensions.insert(MDBasisDimension("q2", true, 2));
    basisDimensions.insert(MDBasisDimension("q3", true, 3));
    basisDimensions.insert(MDBasisDimension("u1", false, 4));

    UnitCell cell;
    MDGeometry* pGeometry = new MDGeometry(MDGeometryBasis(basisDimensions, cell));
    return std::auto_ptr<MDImage>(new MDImage(pGeometry));
  }

public:

  void testGetPixels(void)
  {
    using namespace Mantid::Geometry;
    using namespace Mantid::MDDataObjects;

    MockFileFormat* mockFileFormat = new MockFileFormat;
    EXPECT_CALL(*mockFileFormat, getNPix()).Times(1).WillOnce(testing::Return(100));

    MDDataPoints* points=new MDDataPoints(boost::shared_ptr<MDImage>(constructMDGeometry().release()));
    TSM_ASSERT_EQUALS("The number of pixels returned is not correct.", 100, points->getNumPixels(boost::shared_ptr<IMD_FileFormat>(mockFileFormat)));
    delete points;
  }

  void testConstructedBufferSize(void)
  {
    using namespace Mantid::Geometry;
    using namespace Mantid::MDDataObjects;

    MockFileFormat* mockFileFormat = new MockFileFormat;
    MDDataPoints* points=new MDDataPoints(boost::shared_ptr<MDImage>(constructMDGeometry().release()));

    TSM_ASSERT_EQUALS("The memory buffer size following construction is not correct.", 0, points->getMemorySize());
    delete points;
  }

  void testIsMemoryBased(void)
  {
    using namespace Mantid::Geometry;
    using namespace Mantid::MDDataObjects;

    MockFileFormat* mockFileFormat = new MockFileFormat;
    MDDataPoints* points=new MDDataPoints(boost::shared_ptr<MDImage>(constructMDGeometry().release()));

    TSM_ASSERT_EQUALS("The MDDataPoints should not be in memory.", false, points->isMemoryBased());
    delete points;
  }

  void testAllocation(void)
  {
    using namespace Mantid::Geometry;
    using namespace Mantid::MDDataObjects;

    MockFileFormat* mockFileFormat = new MockFileFormat;
    EXPECT_CALL(*mockFileFormat, getNPix()).Times(1).WillOnce(testing::Return(2));
    MDDataPoints* points=new MDDataPoints(boost::shared_ptr<MDImage>(constructMDGeometry().release()));
    points->alloc_pix_array(boost::shared_ptr<IMD_FileFormat>(mockFileFormat));
    TSM_ASSERT_EQUALS("The memory size is not the expected value after allocation.", 2, points->getMemorySize());
    delete points;
  }



};

#endif