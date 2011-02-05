#ifndef H_TEST_MDWORKSPACE
#define H_TEST_MDWORKSPACE

#include <cxxtest/TestSuite.h>
#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MDDataObjects/MDImage.h"
#include "MDDataObjects/MDDataPoints.h"
#include "MDDataObjects/MDWorkspace.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/scoped_ptr.hpp>
#include <boost/regex.hpp>


using namespace Mantid::API;
using namespace Mantid::Geometry;


class MDWorkspaceTest :    public CxxTest::TestSuite
{
private:

  //Helper mock type for file format.
  class MockFileFormat : public Mantid::MDDataObjects::IMD_FileFormat
  {
  public:
	MockFileFormat(const char *file_name):IMD_FileFormat(file_name){}; 
    MOCK_CONST_METHOD0(getFileName, std::string());
    MOCK_CONST_METHOD0(is_open, bool());
    MOCK_METHOD1(read_MDImg_data, void(Mantid::MDDataObjects::MDImage&)); 
    MOCK_METHOD1(read_pix, bool(Mantid::MDDataObjects::MDDataPoints&)); 
    MOCK_METHOD5(read_pix_subset, size_t(const Mantid::MDDataObjects::MDImage &dnd,const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer));
    uint64_t getNPix()
    {
      return 0;
    }
    MOCK_METHOD1(write_mdd,void(const Mantid::MDDataObjects::MDImage&));
    MOCK_METHOD1(read_MDGeomDescription,void(Mantid::Geometry::MDGeometryDescription &));
    MOCK_METHOD1(read_basis,void(Mantid::Geometry::MDGeometryBasis &));
    MOCK_CONST_METHOD0(read_pointDescriptions,Mantid::MDDataObjects::MDPointDescription(void));

    virtual ~MockFileFormat(void){};
  };
     ///
  //Helper constructional method sets-up a MDGeometry with a valid MDGeometryBasis instance.
  static Mantid::Geometry::MDGeometry* constructMDGeometry()
  {
    using namespace Mantid::Geometry;
    std::set<MDBasisDimension> basisDimensions;
    basisDimensions.insert(MDBasisDimension("q1", true, 0));
    basisDimensions.insert(MDBasisDimension("q2", true, 1));
    basisDimensions.insert(MDBasisDimension("q3", true, 2));
    basisDimensions.insert(MDBasisDimension("u1", false, 3));

    UnitCell cell;
    return new MDGeometry(MDGeometryBasis(basisDimensions, cell));
  }

  //Helper stock constructional method.
  static Mantid::MDDataObjects::MDWorkspace* constructMDWorkspace()
  {
    using namespace Mantid::MDDataObjects;
    using namespace Mantid::Geometry;

    MDWorkspace* workspace;
    try
    {
    workspace = new MDWorkspace;
    }
    catch(std::exception& ex)
    {
      std::string what = ex.what();
    }
    MockFileFormat* mockFile = new MockFileFormat("");
    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), constructMDGeometry());
    return workspace;
  }

  //Helper constructional method provides mdworkspace as IMDWorkspace in order to test this axis of the implementation.
  static Mantid::API::IMDWorkspace* constructMDWorkspaceAsIMDWorkspace()
  {
    return  constructMDWorkspace();
  }



public:

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetNPoints()
  {
    using namespace Mantid::API;
    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
    TSM_ASSERT_EQUALS("MDWorkspace::getNPoints() is mainly implemented. this implementation should return 0",0, workspace->getNPoints());
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetDimension()
  {
    using namespace Mantid::API;

    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());

    std::string id = "q1";
    boost::shared_ptr<const IMDDimension> dimension = workspace->getDimension(id);
    TSM_ASSERT_EQUALS("The dimension id does not match", id, dimension->getDimensionId());
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetDimensionThrows()
  {
    using namespace Mantid::API;
    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());

    std::string id = "::::::";
    TSM_ASSERT_THROWS("The unknown dimension id should have cause and exception to be thrown.", workspace->getDimension(id), std::invalid_argument);
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetPoint()
  {
    using namespace Mantid::API;
    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
    TSM_ASSERT_THROWS("MDWorkspace::getPoint() is not yet implemented. Should have thrown runtime exception", workspace->getPoint(1), std::runtime_error);
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetCellOneArgument()
  {
    using namespace Mantid::API;
    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
    TSM_ASSERT_THROWS("MDWorkspace::getCell() is not yet implemented. Should have thrown runtime exception", workspace->getCell(1), std::runtime_error);
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetCellTwoArgument()
  {
    using namespace Mantid::API;
    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
    TSM_ASSERT_THROWS("MDWorkspace::getCell() is not yet implemented. Should have thrown runtime exception", workspace->getCell(1, 1), std::runtime_error);

  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetCellThreeArgument()
  {
    using namespace Mantid::API;
    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
    TSM_ASSERT_THROWS("MDWorkspace::getCell() is not yet implemented. Should have thrown runtime exception", workspace->getCell(1, 1, 1), std::runtime_error);
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetCellFourArgument()
  {
    using namespace Mantid::API;
    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
    TSM_ASSERT_THROWS("MDWorkspace::getCell() is not yet implemented. Should have thrown runtime exception", workspace->getCell(1, 1, 1, 1), std::runtime_error);
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetCellNArgument()
  {
    using namespace Mantid::API;
    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
    TSM_ASSERT_THROWS("MDWorkspace::getCell() is not yet implemented. Should have thrown runtime exception", workspace->getCell(1, 1, 1, 1, 1), std::runtime_error);
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetXDimension()
  {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
    boost::shared_ptr<const IMDDimension> dimension = workspace->getXDimension();
    TSM_ASSERT_EQUALS("The x-dimension returned was not the expected alignment.", "q1", dimension->getDimensionId());

  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetYDimension()
  {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
    boost::shared_ptr<const IMDDimension> dimension = workspace->getYDimension();
    TSM_ASSERT_EQUALS("The y-dimension returned was not the expected alignment.", "q2", dimension->getDimensionId());
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetZDimension()
  {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
    boost::shared_ptr<const IMDDimension> dimension = workspace->getZDimension();
    TSM_ASSERT_EQUALS("The y-dimension returned was not the expected alignment.", "q3", dimension->getDimensionId());
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGettDimension()
  {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
    boost::shared_ptr<const IMDDimension> dimension = workspace->gettDimension();
    TSM_ASSERT_EQUALS("The t-dimension returned was not the expected alignment.", "u1", dimension->getDimensionId());
  }

  void testGetMemorySize()
  {
    using namespace Mantid::MDDataObjects;

    boost::scoped_ptr<MDWorkspace> workspace(constructMDWorkspace());
    size_t img_data_size = workspace->get_const_MDImage().getMemorySize();
    size_t pix_data_size = workspace->get_const_MDDPoints().getMemorySize();
    TSM_ASSERT_EQUALS("Workspace memory size differs from its parts ",pix_data_size+img_data_size , workspace->getMemorySize());
  }
  void testId(){
    using namespace Mantid::MDDataObjects;
    boost::scoped_ptr<MDWorkspace> workspace(constructMDWorkspace());

    TSM_ASSERT_EQUALS("MD Workspace ID differs from expected ","MD-Workspace", workspace->id());
  }
  ///
  void testGetNumDims(void){
    using namespace Mantid::MDDataObjects;
    boost::scoped_ptr<MDWorkspace> workspace(constructMDWorkspace());
    TSM_ASSERT_EQUALS("Default number of dimensions in Workspac differs from expected ",4, workspace->getNumDims());
  }

  void testReadPixSubset()
  {
    using namespace Mantid::MDDataObjects;
    boost::scoped_ptr<MDWorkspace> workspace(new MDWorkspace());

    MockFileFormat* mockFile = new MockFileFormat("");
    EXPECT_CALL(*mockFile, read_pix_subset(testing::_, testing::_, testing::_, testing::_, testing::_)).Times(1);

    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), constructMDGeometry());
    size_t size_param = 1;
    std::vector<size_t> empty1;
    std::vector<char> empty2;
    workspace->read_pix_selection(empty1, size_param, empty2, size_param);
    TSM_ASSERT("MDWorkspace::read_pix_selection failed to call appropriate method on nested component.", testing::Mock::VerifyAndClearExpectations(mockFile));
  }

  void testReadPixSubsetThrows()
  {
    using namespace Mantid::MDDataObjects;
    boost::scoped_ptr<MDWorkspace> workspace(new MDWorkspace());
    MockFileFormat* mockFile = NULL;
    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), constructMDGeometry());
    size_t size_param = 1;
    std::vector<size_t> empty1;
    std::vector<char> empty2;
    TSM_ASSERT_THROWS("The file has not been provided, so should throw bad allocation",
        workspace->read_pix_selection(empty1, size_param, empty2, size_param), std::runtime_error);
  }


  void  testReadPix(void)
  {
    using namespace Mantid::MDDataObjects;
    boost::scoped_ptr<MDWorkspace> workspace(new MDWorkspace());

    MockFileFormat* mockFile = new MockFileFormat("");
    EXPECT_CALL(*mockFile, read_pix(testing::_)).Times(1);

    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), constructMDGeometry());

    workspace->read_pix();
    TSM_ASSERT("MDWorkspace::read_pix() failed to call appropriate method on nested component.", testing::Mock::VerifyAndClearExpectations(mockFile));
  }

  void  testReadThrows(void)
  {
    using namespace Mantid::MDDataObjects;
    boost::scoped_ptr<MDWorkspace> workspace(new MDWorkspace());
    MockFileFormat* mockFile = NULL;
    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), constructMDGeometry());

    TSM_ASSERT_THROWS("The file reader has not been provided, so should throw bad allocation", workspace->read_pix(), std::runtime_error);
  }

  void testwriteMDDWriteFile(void)
  {
    using namespace Mantid::MDDataObjects;
    boost::scoped_ptr<MDWorkspace> workspace(new MDWorkspace());

    MockFileFormat* mockFile = new MockFileFormat("");
    EXPECT_CALL(*mockFile, write_mdd(testing::_)).Times(1);

    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), constructMDGeometry());

    workspace->write_mdd();
    TSM_ASSERT("MDWorkspace::read_pix() failed to call appropriate method on nested component.", testing::Mock::VerifyAndClearExpectations(mockFile));
  }

  void testwriteMDDthrows()
  {
    using namespace Mantid::MDDataObjects;
    boost::scoped_ptr<MDWorkspace> workspace(new MDWorkspace());
    MockFileFormat* mockFile = NULL;
    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), constructMDGeometry());

    TSM_ASSERT_THROWS("The file reader has not been provided, so should throw bad allocation", workspace->write_mdd(), std::runtime_error);
  }

  void testProperInitalisation()
  {
    using namespace Mantid::MDDataObjects;
    using namespace Mantid::Geometry;

    boost::scoped_ptr<MDWorkspace> workspace(new MDWorkspace());

    MockFileFormat* mockFile = new MockFileFormat("");
    MDGeometry* geometry = constructMDGeometry();

    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), geometry);

    //check that constructed components are now accessible.
    TSM_ASSERT_THROWS_NOTHING("The const ImageData getter is not wired-up correctly",  workspace->get_const_MDImage());
    TSM_ASSERT_THROWS_NOTHING("The const MDDataPoints getter is not wired-up correctly", workspace->get_const_MDDPoints());

    TSM_ASSERT_EQUALS("The const geometry getter is not wired-up correctly", workspace->getGeometry(), geometry);
    TSM_ASSERT("The ImageData getter is not wired-up correctly", workspace->get_spMDImage().get() != NULL);
    TSM_ASSERT("The MDDataPoints getter is not wired-up correctly", workspace->get_spMDDPoints().get() != NULL);
  }

  void testGetWorkspaceLocation()
  {
    using namespace Mantid::MDDataObjects;
    using namespace Mantid::Geometry;

    boost::scoped_ptr<MDWorkspace> workspace(new MDWorkspace());

    MockFileFormat* mockFile = new MockFileFormat("");
    EXPECT_CALL(*mockFile, getFileName()).Times(1).WillOnce(testing::Return("somelocalfile.sqw"));
    MDGeometry* geometry = constructMDGeometry();

    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), geometry);

    TSM_ASSERT_EQUALS("Workspace location is empty", "somelocalfile.sqw", workspace->getWSLocation());
  }

  void testGetWorkspaceGeometry()
  {
        using namespace Mantid::MDDataObjects;
    using namespace Mantid::Geometry;

    boost::scoped_ptr<MDWorkspace> workspace(new MDWorkspace());

    MockFileFormat* mockFile = new MockFileFormat("");
    MDGeometry* geometry = constructMDGeometry();

    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), geometry);


    static const boost::regex condition("^<(.)*>$");
    //Quick test for xml like string.
    TSM_ASSERT("Geometry xml string returned does not look like xml", regex_match(workspace->getGeometryXML(), condition));
    //Test against geometry xml to ensure pass through.
    TSM_ASSERT_EQUALS("Geometry xml does not match xml provided by workspace.", geometry->toXMLString(), workspace->getGeometryXML());
 
  }


};

#endif
