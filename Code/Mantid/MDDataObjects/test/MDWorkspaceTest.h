
#ifndef H_TEST_MDWORKSPACE
#define H_TEST_MDWORKSPACE

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MDDataObjects/MDImage.h"
#include "MDDataObjects/MDDataPoints.h"
#include "MDDataObjects/MDWorkspace.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class tesMDWorkspace :    public CxxTest::TestSuite
{
private:

  //Helper mock type for file format.
  class MockFileFormat : public Mantid::MDDataObjects::IMD_FileFormat
  {
  public:

    MOCK_CONST_METHOD0(is_open, bool());
    MOCK_METHOD1(read_mdd, void(Mantid::MDDataObjects::MDImage&)); 
    MOCK_METHOD1(read_pix, bool(Mantid::MDDataObjects::MDDataPoints&)); 
    MOCK_METHOD5(read_pix_subset, size_t(const Mantid::MDDataObjects::MDImage &dnd,const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer));
    hsize_t getNPix()
    {
      return 0;
    }
    MOCK_METHOD1(write_mdd,void(const Mantid::MDDataObjects::MDImage&));
  };

  //Helper constructional method sets-up a MDGeometry with a valid MDGeometryBasis instance.
  static std::auto_ptr<Mantid::Geometry::MDGeometry> constructMDGeometry()
  {
    using namespace Mantid::Geometry;
    std::set<MDBasisDimension> basisDimensions;
    basisDimensions.insert(MDBasisDimension("q1", true, 1));
    basisDimensions.insert(MDBasisDimension("q2", true, 2));
    basisDimensions.insert(MDBasisDimension("q3", true, 3));
    basisDimensions.insert(MDBasisDimension("u1", false, 4));

    UnitCell cell;
    return std::auto_ptr<MDGeometry>(new MDGeometry(MDGeometryBasis(basisDimensions, cell)));
  }

  //Helper stock constructional method.
  static std::auto_ptr<Mantid::MDDataObjects::MDWorkspace> constructMDWorkspace()
  {
    using namespace Mantid::MDDataObjects;
    using namespace Mantid::Geometry;

    MDWorkspace* workspace = new MDWorkspace;
    MockFileFormat* mockFile = new MockFileFormat;
    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), constructMDGeometry().release());
    return std::auto_ptr<MDWorkspace>(workspace);
  }

  //Helper constructional method provides mdworkspace as IMDWorkspace in order to test this axis of the implementation.
  static std::auto_ptr<Mantid::API::IMDWorkspace> constructMDWorkspaceAsIMDWorkspace()
  {
    return std::auto_ptr<Mantid::API::IMDWorkspace>(constructMDWorkspace().release());
  }



public:

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetNPoints()
  {
    using namespace Mantid::API;
    std::auto_ptr<IMDWorkspace> workspace = constructMDWorkspaceAsIMDWorkspace();
    TSM_ASSERT_THROWS("MDWorkspace::getNPoints() is not yet implemented. Should have thrown runtime exception", workspace->getNPoints(), std::runtime_error);
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetDimension()
  {
    using namespace Mantid::API;

    std::auto_ptr<IMDWorkspace> workspace = constructMDWorkspaceAsIMDWorkspace();

    std::string id = "q1";
    boost::shared_ptr<const IMDDimension> dimension = workspace->getDimension(id);
    TSM_ASSERT_EQUALS("The dimension id does not match", id, dimension->getDimensionId());
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetDimensionThrows()
  {
    using namespace Mantid::API;
    std::auto_ptr<IMDWorkspace> workspace = constructMDWorkspaceAsIMDWorkspace();

    std::string id = "::::::";
    TSM_ASSERT_THROWS("The unknown dimension id should have cause and exception to be thrown.", workspace->getDimension(id), std::invalid_argument);
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetPoint()
  {
    using namespace Mantid::API;
    std::auto_ptr<IMDWorkspace> workspace = constructMDWorkspaceAsIMDWorkspace();
    TSM_ASSERT_THROWS("MDWorkspace::getPoint() is not yet implemented. Should have thrown runtime exception", workspace->getPoint(1), std::runtime_error);
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetCellOneArgument()
  {
    using namespace Mantid::API;
    std::auto_ptr<IMDWorkspace> workspace = constructMDWorkspaceAsIMDWorkspace();
    TSM_ASSERT_THROWS("MDWorkspace::getCell() is not yet implemented. Should have thrown runtime exception", workspace->getCell(1), std::runtime_error);
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetCellTwoArgument()
  {
    using namespace Mantid::API;
    std::auto_ptr<IMDWorkspace> workspace = constructMDWorkspaceAsIMDWorkspace();
    TSM_ASSERT_THROWS("MDWorkspace::getCell() is not yet implemented. Should have thrown runtime exception", workspace->getCell(1, 1), std::runtime_error);

  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetCellThreeArgument()
  {
    using namespace Mantid::API;
    std::auto_ptr<IMDWorkspace> workspace = constructMDWorkspaceAsIMDWorkspace();
    TSM_ASSERT_THROWS("MDWorkspace::getCell() is not yet implemented. Should have thrown runtime exception", workspace->getCell(1, 1, 1), std::runtime_error);
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetCellFourArgument()
  {
    using namespace Mantid::API;
    std::auto_ptr<IMDWorkspace> workspace = constructMDWorkspaceAsIMDWorkspace();
    TSM_ASSERT_THROWS("MDWorkspace::getCell() is not yet implemented. Should have thrown runtime exception", workspace->getCell(1, 1, 1, 1), std::runtime_error);
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetCellNArgument()
  {
    using namespace Mantid::API;
    std::auto_ptr<IMDWorkspace> workspace = constructMDWorkspaceAsIMDWorkspace();
    TSM_ASSERT_THROWS("MDWorkspace::getCell() is not yet implemented. Should have thrown runtime exception", workspace->getCell(1, 1, 1, 1, 1), std::runtime_error);
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetXDimension()
  {
    using namespace Mantid::API;
    std::auto_ptr<IMDWorkspace> workspace = constructMDWorkspaceAsIMDWorkspace();
    boost::shared_ptr<const IMDDimension> dimension = workspace->getXDimension();
    TSM_ASSERT_EQUALS("The x-dimension returned was not the expected alignment.", "q1", dimension->getDimensionId());

  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetYDimension()
  {
    using namespace Mantid::API;
    std::auto_ptr<IMDWorkspace> workspace = constructMDWorkspaceAsIMDWorkspace();
    boost::shared_ptr<const IMDDimension> dimension = workspace->getYDimension();
    TSM_ASSERT_EQUALS("The y-dimension returned was not the expected alignment.", "q2", dimension->getDimensionId());
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetZDimension()
  {
    using namespace Mantid::API;
    std::auto_ptr<IMDWorkspace> workspace = constructMDWorkspaceAsIMDWorkspace();
    boost::shared_ptr<const IMDDimension> dimension = workspace->getZDimension();
    TSM_ASSERT_EQUALS("The y-dimension returned was not the expected alignment.", "q3", dimension->getDimensionId());
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGettDimension()
  {
    using namespace Mantid::API;
    std::auto_ptr<IMDWorkspace> workspace = constructMDWorkspaceAsIMDWorkspace();
    boost::shared_ptr<const IMDDimension> dimension = workspace->gettDimension();
    TSM_ASSERT_EQUALS("The t-dimension returned was not the expected alignment.", "u1", dimension->getDimensionId());
  }

  void testGetMemorySize()
  {
    using namespace Mantid::MDDataObjects;

    std::auto_ptr<MDWorkspace> workspace = constructMDWorkspace();
    size_t img_data_size = workspace->get_const_spMDImage()->getMemorySize();
    size_t pix_data_size = workspace->get_const_spMDDPoints()->getMemorySize();
    TSM_ASSERT_EQUALS("Workspace memory size differs from its parts ",pix_data_size+img_data_size , workspace->getMemorySize());
  }
  void testId(){
    using namespace Mantid::MDDataObjects;
    std::auto_ptr<MDWorkspace> workspace = constructMDWorkspace();

    TSM_ASSERT_EQUALS("MD Workspace ID differs from expected ","MD-Workspace", workspace->id());
  }
  ///
  void testGetNumDims(void){
    using namespace Mantid::MDDataObjects;
    std::auto_ptr<MDWorkspace> workspace = constructMDWorkspace();
    TSM_ASSERT_EQUALS("Default number of dimensions in Workspac differs from expected ",4, workspace->getNumDims());
  }

  void testReadPixSubset()
  {
    using namespace Mantid::MDDataObjects;
    std::auto_ptr<MDWorkspace> workspace = std::auto_ptr<MDWorkspace>(new MDWorkspace());

    MockFileFormat* mockFile = new MockFileFormat();
    EXPECT_CALL(*mockFile, read_pix_subset(testing::_, testing::_, testing::_, testing::_, testing::_)).Times(1);

    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), constructMDGeometry().release());
    size_t size_param = 1;
    workspace->read_pix_selection(std::vector<size_t>(), size_param, std::vector<char>(), size_param);
    TSM_ASSERT("MDWorkspace::read_pix_selection failed to call appropriate method on nested component.", testing::Mock::VerifyAndClearExpectations(mockFile));
  }

  void testReadPixSubsetThrows()
  {
    using namespace Mantid::MDDataObjects;
    std::auto_ptr<MDWorkspace> workspace = std::auto_ptr<MDWorkspace>(new MDWorkspace());
    MockFileFormat* mockFile = NULL;
    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), constructMDGeometry().release());
    size_t size_param = 1;
    TSM_ASSERT_THROWS("The file has not been provided, so should throw bad allocation", workspace->read_pix_selection(std::vector<size_t>(), size_param, std::vector<char>(), size_param), std::runtime_error);
  }


  void  testReadPix(void)
  {
    using namespace Mantid::MDDataObjects;
    std::auto_ptr<MDWorkspace> workspace = std::auto_ptr<MDWorkspace>(new MDWorkspace());

    MockFileFormat* mockFile = new MockFileFormat();
    EXPECT_CALL(*mockFile, read_pix(testing::_)).Times(1);

    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), constructMDGeometry().release());

    workspace->read_pix();
    TSM_ASSERT("MDWorkspace::read_pix() failed to call appropriate method on nested component.", testing::Mock::VerifyAndClearExpectations(mockFile));
  }

  void  testReadThrows(void)
  {
    using namespace Mantid::MDDataObjects;
    std::auto_ptr<MDWorkspace> workspace = std::auto_ptr<MDWorkspace>(new MDWorkspace());
    MockFileFormat* mockFile = NULL;
    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), constructMDGeometry().release());

    TSM_ASSERT_THROWS("The file reader has not been provided, so should throw bad allocation", workspace->read_pix(), std::runtime_error);
  }

  void testwriteMDDWriteFile(void)
  {
    using namespace Mantid::MDDataObjects;
    std::auto_ptr<MDWorkspace> workspace = std::auto_ptr<MDWorkspace>(new MDWorkspace());

    MockFileFormat* mockFile = new MockFileFormat();
    EXPECT_CALL(*mockFile, write_mdd(testing::_)).Times(1);

    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), constructMDGeometry().release());

    workspace->write_mdd();
    TSM_ASSERT("MDWorkspace::read_pix() failed to call appropriate method on nested component.", testing::Mock::VerifyAndClearExpectations(mockFile));
  }

  void testwriteMDDthrows()
  {
    using namespace Mantid::MDDataObjects;
    std::auto_ptr<MDWorkspace> workspace = std::auto_ptr<MDWorkspace>(new MDWorkspace());
    MockFileFormat* mockFile = NULL;
    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), constructMDGeometry().release());

    TSM_ASSERT_THROWS("The file reader has not been provided, so should throw bad allocation", workspace->write_mdd(), std::runtime_error);
  }

  void testProperInitalisation()
  {
    using namespace Mantid::MDDataObjects;

    std::auto_ptr<MDWorkspace> workspace = std::auto_ptr<MDWorkspace>(new MDWorkspace());

    MockFileFormat* mockFile = new MockFileFormat();
    MDGeometry* geometry = constructMDGeometry().release();

    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), geometry);

    //check that constructed components are now accessible.
    TSM_ASSERT("The const ImageData getter is not wired-up correctly", workspace->get_const_spMDImage().get() != NULL);
    TSM_ASSERT("The const MDDataPoints getter is not wired-up correctly", workspace->get_const_spMDDPoints().get() != NULL);

    TSM_ASSERT_EQUALS("The const geometry getter is not wired-up correctly", workspace->getGeometry(), geometry);
    TSM_ASSERT("The ImageData getter is not wired-up correctly", workspace->get_spMDImage().get() != NULL);
    TSM_ASSERT("The MDDataPoints getter is not wired-up correctly", workspace->get_spMDDPoints().get() != NULL);
  }


};

#endif