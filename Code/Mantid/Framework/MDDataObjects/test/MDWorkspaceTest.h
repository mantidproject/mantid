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

  /// Fake geometry. Simply required so that dimensions can be created with ranges on-the-spot.
  class FakeMDGeometry : public Mantid::Geometry::MDGeometry
  {
  public:
    FakeMDGeometry(const Mantid::Geometry::MDGeometryBasis& basis) : MDGeometry(basis)
    {
    }
	// This function can not be exposed to anywhere withing MD workspace, except testing MDGeometry, 
	// as changing number of bins invalidates MD image structures and MDDataPoints structures
    void setNumberOfBins(const int indexOfDimension, const int nBins)
    {
      //Range min/max are not important, but the number of bins is!
      this->getDimension(indexOfDimension)->setRange(0, 10, nBins);
    }
    /// Convenience member function for working with cells in a IMDWorkspace.
    size_t getTotalNumberOfBins()
    {
      size_t sum = 0;
      for(size_t i = 0; i < this->getDimensions().size(); i++)
      {
        sum += this->getDimension(i)->getNBins();
      }
      return sum;
    }
  };

  //Helper constructional method sets-up a MDGeometry with a valid MDGeometryBasis instance.
  static Mantid::Geometry::MDGeometry* constructMDGeometry()
  {
    using namespace Mantid::Geometry;
    std::set<MDBasisDimension> basisDimensions;
    basisDimensions.insert(MDBasisDimension("q0", true, 0));
    basisDimensions.insert(MDBasisDimension("q1", true, 1));
    basisDimensions.insert(MDBasisDimension("q2", true, 2));
    basisDimensions.insert(MDBasisDimension("u3", false, 3));

	boost::shared_ptr<OrientedLattice> spCell = boost::shared_ptr<OrientedLattice>(new OrientedLattice(2.87,2.87,2.87));
    FakeMDGeometry* geometry = new FakeMDGeometry(MDGeometryBasis(basisDimensions,spCell));
    geometry->setNumberOfBins(0, 4);
    geometry->setNumberOfBins(1, 4);
    geometry->setNumberOfBins(2, 4);
    geometry->setNumberOfBins(3, 4);
    return geometry;
  }

  //Helper mock type for file format.
  class MockFileFormat : public Mantid::MDDataObjects::IMD_FileFormat
  {
  private:
    int m_ncells;
    int m_npoints;
  public:
	MockFileFormat(const char *file_name, int ncells, int npoints):IMD_FileFormat(file_name), m_ncells(ncells), m_npoints(npoints){};
    MOCK_CONST_METHOD0(getFileName, std::string());
    MOCK_CONST_METHOD0(is_open, bool());
    MOCK_METHOD5(read_pix_subset, size_t(const Mantid::MDDataObjects::MDImage &dnd,const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer));
    MOCK_METHOD1(write_mdd,void(const Mantid::MDDataObjects::MDImage&));
    MOCK_METHOD1(read_MDGeomDescription,void(Mantid::Geometry::MDGeometryDescription &));
    MOCK_METHOD1(read_basis,void(Mantid::Geometry::MDGeometryBasis &));
    MOCK_CONST_METHOD0(read_pointDescriptions,Mantid::MDDataObjects::MDPointDescription(void));

    //Implementation loosely taken from MD_File_hdfMatlab. Enables verification that MDCells can be read out correctly.
    void read_MDImg_data(Mantid::MDDataObjects::MDImage& image)
    {
      using namespace Mantid::MDDataObjects;
      MD_image_point *pImageCell = new MD_image_point[m_ncells];

      for(int i=0;i<m_ncells;i++)
      {
        pImageCell[i].s   = i+1;//Just some computable value.
        pImageCell[i].err = i; //Just some computable value.
      }
      image.get_pMDImgData()->data = pImageCell;
    }

    //Implementation loosely taken from MD_File_hdfMatlab. Just want to be able to load pixels into workspace
    bool read_pix(Mantid::MDDataObjects::MDDataPoints& sqw,bool nothrow)
    {
	  UNUSED_ARG(nothrow);

      using namespace Mantid::MDDataObjects;
      const int size = 9;
      std::vector<std::string> dataTags(size);
      std::string tagNames[] = {"qx","qy","qz","en","S","err","runID","pixID","enID"};
      std::copy ( tagNames, tagNames + size, dataTags.begin() );

      MDPointStructure defaultPixel;
      defaultPixel.DimIDlength =4;
      defaultPixel.SignalLength=4;
      defaultPixel.NumPixCompressionBits=0;

      MDPointDescription PixSignature(defaultPixel,dataTags);
      char *buf(NULL);
      MDDataPointEqual<float,uint32_t,float> packer(buf,PixSignature);

      std::vector<char> *outData = sqw.get_pBuffer((size_t)this->getNPix());
      packer.setBuffer(&(*outData)[0]);

      float    s_dim_fields[6];
      uint32_t  ind_fields[3];

      for(uint64_t i=0;i<this->getNPix();i++)
      {
        s_dim_fields[0] =          (float) i;
        s_dim_fields[1] =          (float) i; // sqw.pix_array[i].qy
        s_dim_fields[2] =           (float)i; // sqw.pix_array[i].qz
        s_dim_fields[3] =           (float)i; // sqw.pix_array[i].En
        ind_fields[0]   =  (uint32_t)i;    // sqw.pix_array[i].irun
        ind_fields[1]   =  (uint32_t)i;   // sqw.pix_array[i].idet
        ind_fields[2]   =  (uint32_t)i; // sqw.pix_array[i].ien
        s_dim_fields[4] =           (float)i+1; // sqw.pix_array[i].s
        s_dim_fields[5] =           (float)i;  // sqw.pix_array[i].err

        packer.setData(i,s_dim_fields,ind_fields);
       }
      return true;
    }
    uint64_t getNPix()
    {
      return (uint64_t)m_npoints;
    }
    virtual ~MockFileFormat(void){};
  };

  //Helper stock constructional method.
  static Mantid::MDDataObjects::MDWorkspace* constructMDWorkspace()
  {
    using namespace Mantid::MDDataObjects;
    using namespace Mantid::Geometry;

    MDWorkspace* workspace = new MDWorkspace;
    MockFileFormat* mockFile = new MockFileFormat("",256,256);
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
    TSM_ASSERT_EQUALS("MDWorkspace::getNPoints(). Implementation should return 256 ",256, workspace->getNPoints());
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetDimension()
  {
    using namespace Mantid::API;

    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());

    std::string id = "q0";
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
    using namespace Mantid::MDDataObjects;

    //Get as a MDWorkspace so that the read pixels can be called. This uses a fake method on a mock object to
    //generate an array of pixels to read.
    MDWorkspace* mdWorkspace = constructMDWorkspace();

	// 3 rows equivalent of mdWorkspace->read_pix();
	boost::shared_ptr<Mantid::MDDataObjects::MDDataPoints> spMDPoints = mdWorkspace->get_spMDDPoints();
    IMD_FileFormat  & file_reader =  mdWorkspace->get_const_FileReader();
	bool read_successfully=file_reader.read_pix(*spMDPoints,true);
    
	TSM_ASSERT_EQUALS("test data should be retrieved successfully",true,read_successfully);

    //Pass workspace over as IMDWorkspace. Ensures we are testing IMDWorkspace aspects.
    boost::scoped_ptr<IMDWorkspace> workspace(mdWorkspace);

    //There should be 10 pixels available. This is tested elsewhere.
    //Test that the first and last pixels give the correct results.
    TSM_ASSERT_EQUALS("The signal value for the first pixel is incorrect", 1, workspace->getPoint(0).getSignal());
    TSM_ASSERT_EQUALS("The error value for the first pixel is incorrect", 0, workspace->getPoint(0).getError());
    TSM_ASSERT_EQUALS("The signal value for the pixel is incorrect", 10, workspace->getPoint(9).getSignal());
    TSM_ASSERT_EQUALS("The error value for the pixel is incorrect", 9, workspace->getPoint(9).getError());
    TSM_ASSERT_THROWS("Index should be out of bounds", workspace->getPoint(256), std::range_error);
    TSM_ASSERT_THROWS("Index should be out of bounds", workspace->getPoint(-1), std::range_error);
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetNonIntegratedDimensions()
  {
    using namespace Mantid::API;
    using namespace Mantid::MDDataObjects;

    MDWorkspace* mdWorkspace = constructMDWorkspace();
    boost::scoped_ptr<IMDWorkspace> workspace(mdWorkspace);

    TSM_ASSERT_EQUALS("In test example all dimensions should be non-integrated.", 4, workspace->getNonIntegratedDimensions().size());
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetCellOneArgument()
  {
    using namespace Mantid::API;
    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
	// done to see if getCell throws
    const SignalAggregate& cell = workspace->getCell(0);
    TSM_ASSERT_EQUALS("The first MDCell's signal value is incorrect", 1, cell.getSignal());
    TSM_ASSERT_EQUALS("The first MDCell's error value is incorrect", 0, cell.getError());
    TSM_ASSERT_EQUALS("The MDCell's signal value is incorrect", 4, workspace->getCell(3).getSignal());
    TSM_ASSERT_EQUALS("The MDCell's error value is incorrect", 3, workspace->getCell(3).getError());
    TSM_ASSERT_EQUALS("Wrong number of vertexes generated.", 2, workspace->getCell(3).getVertexes().size());
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetCellTwoArgument()
  {
    using namespace Mantid::API;
    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
	// done to see if getCell throws
    const SignalAggregate& cell = workspace->getCell(0,0);
    TSM_ASSERT_EQUALS("The first MDCell's signal value is incorrect", 1, cell.getSignal());
    TSM_ASSERT_EQUALS("The first MDCell's error value is incorrect", 0, cell.getError());
    TSM_ASSERT_EQUALS("The MDCell's signal value is incorrect", 2, workspace->getCell(1, 0).getSignal());
    TSM_ASSERT_EQUALS("The MDCell's error value is incorrect", 1, workspace->getCell(1, 0).getError());
    TSM_ASSERT_EQUALS("Wrong number of vertexes generated.", 4, workspace->getCell(1, 0).getVertexes().size());
    TSM_ASSERT_THROWS("The cell requested should be out of bounds", workspace->getCell(4, 4), std::range_error);
    TSM_ASSERT_THROWS("The cell requested should be out of bounds", workspace->getCell(0,-1), std::runtime_error);
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetCellThreeArgument()
  {
    using namespace Mantid::API;
    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
	// done to see if getCell throws
    const SignalAggregate& cell = workspace->getCell(0,0,0);
    TSM_ASSERT_EQUALS("The first MDCell's signal value is incorrect", 1, cell.getSignal());
    TSM_ASSERT_EQUALS("The first MDCell's error value is incorrect", 0, cell.getError());
    TSM_ASSERT_EQUALS("The MDCell's signal value is incorrect", 2, workspace->getCell(1, 0, 0).getSignal());
    TSM_ASSERT_EQUALS("The MDCell's error value is incorrect", 1, workspace->getCell(1, 0, 0).getError());
    TSM_ASSERT_EQUALS("Wrong number of vertexes generated.", 8, workspace->getCell(1, 0, 0).getVertexes().size());
    TSM_ASSERT_THROWS("The cell requested should be out of bounds", workspace->getCell(4, 4, 4), std::range_error);
    TSM_ASSERT_THROWS("The cell requested should be out of bounds", workspace->getCell(0, 0, -1), std::runtime_error);
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetCellFourArgument()
  {
    using namespace Mantid::API;
    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
// done to see if getCell throws
    const SignalAggregate& cell = workspace->getCell(0,0,0,0);
    TSM_ASSERT_EQUALS("The first MDCell's signal value is incorrect", 1, cell.getSignal());
    TSM_ASSERT_EQUALS("The first MDCell's error value is incorrect", 0, cell.getError());
    TSM_ASSERT_EQUALS("The MDCell's signal value is incorrect", 2, workspace->getCell(1, 0, 0, 0).getSignal());
    TSM_ASSERT_EQUALS("The MDCell's error value is incorrect", 1, workspace->getCell(1, 0, 0, 0).getError());
    TSM_ASSERT_EQUALS("Wrong number of vertexes generated.", 16, workspace->getCell(1, 0, 0, 0).getVertexes().size());
    TSM_ASSERT_THROWS("The cell requested should be out of bounds", workspace->getCell(4, 4, 4, 4), std::range_error);
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetCellNArgument()
  {
    using namespace Mantid::API;
    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
    TSM_ASSERT_THROWS("MDWorkspace::getCell() is not yet implemented. Should have thrown runtime exception", workspace->getCell(1, 1, 1, 1, 1), std::runtime_error);
  }

  void testGetSignalNormalisedAt1D()
  {
    using namespace Mantid::API;
    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
    TS_ASSERT_EQUALS(workspace->getSignalAt(0), workspace->getSignalNormalizedAt(0));
    TS_ASSERT_EQUALS(workspace->getSignalAt(1), workspace->getSignalNormalizedAt(1));
    TS_ASSERT_EQUALS(workspace->getSignalAt(2), workspace->getSignalNormalizedAt(2));
  }

  void testGetSignalNormalisedAt2D()
  {
    using namespace Mantid::API;
    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
    TS_ASSERT_EQUALS(workspace->getSignalAt(0, 0), workspace->getSignalNormalizedAt(0, 0));
    TS_ASSERT_EQUALS(workspace->getSignalAt(1, 1), workspace->getSignalNormalizedAt(1, 1));
    TS_ASSERT_EQUALS(workspace->getSignalAt(2, 2), workspace->getSignalNormalizedAt(2, 2));
  }

  void testGetSignalNormalisedAt3D()
  {
    using namespace Mantid::API;
    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
    TS_ASSERT_EQUALS(workspace->getSignalAt(0, 0, 0), workspace->getSignalNormalizedAt(0, 0, 0));
    TS_ASSERT_EQUALS(workspace->getSignalAt(1, 1, 1), workspace->getSignalNormalizedAt(1, 1, 1));
    TS_ASSERT_EQUALS(workspace->getSignalAt(2, 2, 2), workspace->getSignalNormalizedAt(2, 2, 2));
  }

  void testGetSignalNormalisedAt4D()
  {
    using namespace Mantid::API;
    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
    TS_ASSERT_EQUALS(workspace->getSignalAt(0, 0, 0, 0), workspace->getSignalNormalizedAt(0, 0, 0, 0));
    TS_ASSERT_EQUALS(workspace->getSignalAt(1, 1, 1, 1), workspace->getSignalNormalizedAt(1, 1, 1, 1));
    TS_ASSERT_EQUALS(workspace->getSignalAt(2, 2, 2, 2), workspace->getSignalNormalizedAt(2, 2, 2, 2));
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetXDimension()
  {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
    boost::shared_ptr<const IMDDimension> dimension = workspace->getXDimension();
    TSM_ASSERT_EQUALS("The x-dimension returned was not the expected alignment.", "q0", dimension->getDimensionId());

  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetYDimension()
  {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
    boost::shared_ptr<const IMDDimension> dimension = workspace->getYDimension();
    TSM_ASSERT_EQUALS("The y-dimension returned was not the expected alignment.", "q1", dimension->getDimensionId());
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetZDimension()
  {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
    boost::shared_ptr<const IMDDimension> dimension = workspace->getZDimension();
    TSM_ASSERT_EQUALS("The y-dimension returned was not the expected alignment.", "q2", dimension->getDimensionId());
  }

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGettDimension()
  {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    boost::scoped_ptr<IMDWorkspace> workspace(constructMDWorkspaceAsIMDWorkspace());
    boost::shared_ptr<const IMDDimension> dimension = workspace->getTDimension();
    TSM_ASSERT_EQUALS("The t-dimension returned was not the expected alignment.", "u3", dimension->getDimensionId());
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

  /*   // this method does not exits and will not presumably exist in a future
  void t__tReadPixSubset()
  {
    using namespace Mantid::MDDataObjects;
    boost::scoped_ptr<MDWorkspace> workspace(new MDWorkspace());

    MockFileFormat* mockFile = new MockFileFormat("",1,1);
    EXPECT_CALL(*mockFile, read_pix_subset(testing::_, testing::_, testing::_, testing::_, testing::_)).Times(1);

    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), constructMDGeometry());

	// 3 rows equivalent of mdWorkspace->read_pix_selection();
	boost::shared_ptr<Mantid::MDDataObjects::MDDataPoints> spMDPoints = mdWorkspace->get_spMDDPoints();
    IMD_FileFormat  & file_reader =  mdWorkspace->get_const_FileReader();
	bool read_successfully=file_reader.read_pix(*spMDPoints,true);
 

    size_t size_param = 1;
    std::vector<size_t> empty1;
    std::vector<char> empty2;
    workspace->read_pix_selection(empty1, size_param, empty2, size_param);
    TSM_ASSERT("MDWorkspace::read_pix_selection failed to call appropriate method on nested component.", testing::Mock::VerifyAndClearExpectations(mockFile));
  }

  void t__tReadPixSubsetThrows()
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

  void  t__tReadThrows(void)
  {
    using namespace Mantid::MDDataObjects;
    boost::scoped_ptr<MDWorkspace> workspace(new MDWorkspace());
    MockFileFormat* mockFile = NULL;
    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), constructMDGeometry());

    TSM_ASSERT_THROWS("The file reader has not been provided, so should throw bad allocation", workspace->read_pix(), std::runtime_error);
  }

  void tt__tWriteMDDWriteFile(void)
  {
    using namespace Mantid::MDDataObjects;
    boost::scoped_ptr<MDWorkspace> workspace(new MDWorkspace());

    MockFileFormat* mockFile = new MockFileFormat("", 1, 1);
    EXPECT_CALL(*mockFile, write_mdd(testing::_)).Times(1);

    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), constructMDGeometry());

    workspace->write_mdd();
    TSM_ASSERT("MDWorkspace::read_pix() failed to call appropriate method on nested component.", testing::Mock::VerifyAndClearExpectations(mockFile));
  }

  void t__tWriteMDDthrows()
  {
    using namespace Mantid::MDDataObjects;
    boost::scoped_ptr<MDWorkspace> workspace(new MDWorkspace());
    MockFileFormat* mockFile = NULL;
    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), constructMDGeometry());

    TSM_ASSERT_THROWS("The file reader has not been provided, so should throw bad allocation", workspace->write_mdd(), std::runtime_error);
  }

*/
  void testProperInitalisation()
  {
    using namespace Mantid::MDDataObjects;
    using namespace Mantid::Geometry;

    boost::scoped_ptr<MDWorkspace> workspace(new MDWorkspace());

    MockFileFormat* mockFile = new MockFileFormat("", 1, 1);
    MDGeometry* geometry = constructMDGeometry();

    workspace->init(boost::shared_ptr<IMD_FileFormat>(mockFile), geometry);

    //check that constructed components are now accessible.
    TSM_ASSERT_THROWS_NOTHING("The const ImageData getter is not wired-up correctly",  workspace->get_const_MDImage());
    TSM_ASSERT_THROWS_NOTHING("The const MDDataPoints getter is not wired-up correctly", workspace->get_const_MDDPoints());


    TSM_ASSERT("The ImageData getter is not wired-up correctly", workspace->get_spMDImage().get() != NULL);
	TSM_ASSERT_EQUALS("The const geometry getter is not wired-up correctly", workspace->get_const_MDImage().getGeometry(), *geometry);
    TSM_ASSERT("The MDDataPoints getter is not wired-up correctly", workspace->get_spMDDPoints().get() != NULL);
  }

  void testGetWorkspaceLocation()
  {
    using namespace Mantid::MDDataObjects;
    using namespace Mantid::Geometry;

    boost::scoped_ptr<MDWorkspace> workspace(new MDWorkspace());

    MockFileFormat* mockFile = new MockFileFormat("", 1, 1);
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

    MockFileFormat* mockFile = new MockFileFormat("", 1, 1);
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
