#ifndef IMDWORKSPACE_PROXY_TEST_H_
#define IMDWORKSPACE_PROXY_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/IMDWorkspaceProxy.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

class GeometryProxyTest: public CxxTest::TestSuite
{
private:

  //  typedef boost::shared_ptr<Mantid::Geometry::MDDimension> MDDimension_sptr;
  //  typedef std::vector<MDDimension_sptr> MDDimension_vec;
  //
  //Helper methods purely to keep dimension ids consistent.
  static std::string getXDimId()
  {
    return "qx";
  }
  static std::string getYDimId()
  {
    return "qy";
  }
  static std::string getZDimId()
  {
    return "qz";
  }
  static std::string getTDimId()
  {
    return "en";
  }

  //Helper type to generate unique numbers from i, j, k, combinations.
  //Assume arguments are between 0 and 9. Crude, but sufficient for these test scenarios
  struct uniqueArgumentCombination
  {
    double operator()(int i, int j, int k, int t)
    {
      return i * 1000 + j * 100 + k * 10 + t;
    }
  };

  class MockIMDDimension: public Mantid::Geometry::IMDDimension
  {
  public:
    MOCK_CONST_METHOD0(getName,
        std::string());
    MOCK_CONST_METHOD0(getDimensionId,
        std::string());
    MOCK_CONST_METHOD0(getMaximum,
        double());
    MOCK_CONST_METHOD0(getMinimum,
        double());
    MOCK_CONST_METHOD0(getNBins,
        size_t());
    MOCK_CONST_METHOD0(toXMLString,
        std::string());
    MOCK_CONST_METHOD1(getX,
        double(size_t ind));
  };

  class MockIMDWorkspace: public Mantid::API::IMDWorkspace
  {
  public:

    MOCK_CONST_METHOD0(id, const std::string());
    MOCK_CONST_METHOD0(getMemorySize, size_t());
    MOCK_CONST_METHOD1(getPoint,const Mantid::Geometry::SignalAggregate&(unsigned int index));
    MOCK_CONST_METHOD1(getCell,const Mantid::Geometry::SignalAggregate&(unsigned int dim1Increment));
    MOCK_CONST_METHOD2(getCell,const Mantid::Geometry::SignalAggregate&(unsigned int dim1Increment, unsigned int dim2Increment));
    MOCK_CONST_METHOD3(getCell,const Mantid::Geometry::SignalAggregate&(unsigned int dim1Increment, unsigned int dim2Increment, unsigned int dim3Increment));
    MOCK_CONST_METHOD4(getCell,const Mantid::Geometry::SignalAggregate&(unsigned int dim1Increment, unsigned int dim2Increment, unsigned int dim3Increment, unsigned int dim4Increment));

    MOCK_CONST_METHOD0(getWSLocation,std::string());
    MOCK_CONST_METHOD0(getGeometryXML,std::string());

    MOCK_CONST_METHOD0(getXDimension,boost::shared_ptr<const Mantid::Geometry::IMDDimension>());
    MOCK_CONST_METHOD0(getYDimension,boost::shared_ptr<const Mantid::Geometry::IMDDimension>());
    MOCK_CONST_METHOD0(getZDimension,boost::shared_ptr<const Mantid::Geometry::IMDDimension>());
    MOCK_CONST_METHOD0(getTDimension,boost::shared_ptr<const Mantid::Geometry::IMDDimension>());
    MOCK_CONST_METHOD1(getDimension,boost::shared_ptr<const Mantid::Geometry::IMDDimension>(std::string id));
    MOCK_METHOD1(getDimensionNum,boost::shared_ptr<Mantid::Geometry::IMDDimension>(size_t index));
    MOCK_CONST_METHOD0(getDimensionIDs,const std::vector<std::string>());
    MOCK_CONST_METHOD0(getNPoints, uint64_t());
    MOCK_CONST_METHOD0(getNumDims, size_t());

    const Mantid::Geometry::SignalAggregate& getCell(...) const
    {
      throw std::runtime_error("Not Implemented");
    }
  };

  /// Helper method. Creates a mock x Dimension by assigning a specified id to the getDimensionId return type on the mock object created.
  static Mantid::Geometry::IMDDimension* createXDimension()
  {
    using namespace testing;
    MockIMDDimension* p_xDim = new MockIMDDimension;
    EXPECT_CALL(*p_xDim, getDimensionId()).Times(AtLeast(1)).WillRepeatedly(Return(getXDimId()));
    return p_xDim;
  }

  /// Helper method. Creates a mock y Dimension by assigning a specified id to the getDimensionId return type on the mock object created.
  static Mantid::Geometry::IMDDimension* createYDimension()
  {
    using namespace testing;
    MockIMDDimension* p_yDim = new MockIMDDimension;
    EXPECT_CALL(*p_yDim, getDimensionId()).Times(AtLeast(1)).WillRepeatedly(Return(getYDimId()));
    return p_yDim;
  }

  /// Helper method. Creates a mock z Dimension by assigning a specified id to the getDimensionId return type on the mock object created.
  static Mantid::Geometry::IMDDimension* createZDimension()
  {
    using namespace testing;
    MockIMDDimension* p_zDim = new MockIMDDimension;
    EXPECT_CALL(*p_zDim, getDimensionId()).Times(AtLeast(1)).WillRepeatedly(Return(getZDimId()));
    return p_zDim;
  }

  /// Helper method. Creates a mock t Dimension by assigning a specified id to the getDimensionId return type on the mock object created.
  static Mantid::Geometry::IMDDimension* createTDimension()
  {
    using namespace testing;
    MockIMDDimension* p_tDim = new MockIMDDimension;
    EXPECT_CALL(*p_tDim, getDimensionId()).Times(AtLeast(1)).WillRepeatedly(Return(getTDimId()));
    return p_tDim;
  }

  static Mantid::API::IMDWorkspace* createMockIMDWorkspace()
    {
    using namespace testing;
        using Mantid::API::IMDWorkspace_sptr;
        using Mantid::VATES::IMDWorkspaceProxy;
        using Mantid::VATES::Dimension_const_sptr;

        MockIMDWorkspace* pWorkspace = new MockIMDWorkspace;
        EXPECT_CALL(*pWorkspace, getXDimension()).WillRepeatedly(Return(Dimension_const_sptr(
            createXDimension())));
        EXPECT_CALL(*pWorkspace, getYDimension()).WillRepeatedly(Return(Dimension_const_sptr(
            createYDimension())));
        EXPECT_CALL(*pWorkspace, getZDimension()).WillRepeatedly(Return(Dimension_const_sptr(
            createZDimension())));
        EXPECT_CALL(*pWorkspace, getTDimension()).WillRepeatedly(Return(Dimension_const_sptr(
            createTDimension())));
        return pWorkspace;
    }

  /* Helper method. Generates a simple IMDWorkspaceProxy object, on which initalize is called (via ::New()). Useful for tests where dimension mappings are
   an unrelated detail.*/
  static Mantid::API::IMDWorkspace_sptr createAnyProxyIMDWorkspace()
  {
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr workspace(createMockIMDWorkspace());

    Dimension_const_sptr xDim(createXDimension());
    Dimension_const_sptr yDim(createYDimension());
    Dimension_const_sptr zDim(createZDimension());
    Dimension_const_sptr tDim(createTDimension());

    return IMDWorkspaceProxy::New(workspace, xDim, yDim, zDim, tDim);
  }

public:

  void testGetCellElipsisThrows()
  {
    using namespace testing;
    using Mantid::API::IMDWorkspace_sptr;

    IMDWorkspace_sptr proxy = createAnyProxyIMDWorkspace();
    TSM_ASSERT_THROWS("This method is deliberately not implemented on proxy. It should throw.", proxy->getCell(1,1,1,1,1), std::runtime_error);
  }

  void testGetWorkspaceLocationThrows()
  {
    using namespace testing;
    using Mantid::API::IMDWorkspace_sptr;

    IMDWorkspace_sptr proxy = createAnyProxyIMDWorkspace();
    TSM_ASSERT_THROWS("This method is deliberately not implemented on proxy. It should throw.", proxy->getWSLocation(), std::runtime_error);
  }

  void testGetGeometryXMLThrows()
  {
    using namespace testing;
    using Mantid::API::IMDWorkspace_sptr;

    IMDWorkspace_sptr proxy = createAnyProxyIMDWorkspace();
    TSM_ASSERT_THROWS("This method is deliberately not implemented on proxy. It should throw.", proxy->getGeometryXML(), std::runtime_error);
  }

  void testNormalDimensionMappings()
  {
    using namespace testing;
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr(createMockIMDWorkspace());

    Dimension_const_sptr xDim(createXDimension());
    Dimension_const_sptr yDim(createYDimension());
    Dimension_const_sptr zDim(createZDimension());
    Dimension_const_sptr tDim(createTDimension());

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    TS_ASSERT_EQUALS(getXDimId(), proxy->getXDimension()->getDimensionId());
    TS_ASSERT_EQUALS(getYDimId(), proxy->getYDimension()->getDimensionId());
    TS_ASSERT_EQUALS(getZDimId(), proxy->getZDimension()->getDimensionId());
    TS_ASSERT_EQUALS(getTDimId(), proxy->getTDimension()->getDimensionId());
  }

  void testRemappedDimensions()
  {
    using namespace testing;
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createTDimension());//New alignment
    Dimension_const_sptr yDim(createZDimension());//New alignment
    Dimension_const_sptr zDim(createYDimension());//New alignment
    Dimension_const_sptr tDim(createXDimension());//New alignment

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    TS_ASSERT_EQUALS(getTDimId(), proxy->getXDimension()->getDimensionId());
    TS_ASSERT_EQUALS(getZDimId(), proxy->getYDimension()->getDimensionId());
    TS_ASSERT_EQUALS(getXDimId(), proxy->getZDimension()->getDimensionId());
    TS_ASSERT_EQUALS(getYDimId(), proxy->getTDimension()->getDimensionId());
  }
//
//  void testRemapPoints_xyzt()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getXDimId(), 1, 2, 3)); //Mirrors geometry x dimension mapping
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getYDimId(), 1, 2, 3)); //Mirrors geometry y dimension mapping
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getZDimId(), 1, 2, 3)); //Mirrors geometry z dimension mapping
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getTDimId(), 1, 2, 3)); //Mirrors geometry t dimension mapping
//
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for xyzt schenario", unique(1, 2, 3, 4), function(1, 2, 3, 4).s);
//  }
//
//  void testRemapPoints_xzyt()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getXDimId(), 1, 2, 3)); // x -> x
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getZDimId(), 1, 2, 3)); // y -> y
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getYDimId(), 1, 2, 3)); // z -> z
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getTDimId(), 1, 2, 3)); // t -> t
//
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for xzyt schenario", unique(1, 3, 2, 4), function(1, 2, 3, 4).s);
//  }
//
//
//  void testRemapPoints_yxzt()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getYDimId(), 1, 2, 3)); // x -> y (so x becomes 2nd argument)
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getXDimId(), 1, 2, 3)); // y -> x (so y becomes 1st argument)
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getZDimId(), 1, 2, 3)); // z -> z (so z becomes 3rd argument)
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getTDimId(), 1, 2, 3)); // t -> t (so t becomes 4th argument)
//    //hence yxzt
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for yxzt schenario", unique(2, 1, 3, 4), function(1, 2, 3, 4).s);
//  }
//
//  void testRemapPoints_yzxt()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getZDimId(), 1, 2, 3)); // x -> z (so becomes 3nd argument)
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getXDimId(), 1, 2, 3)); // y -> x (so becomes 1st argument)
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getYDimId(), 1, 2, 3)); // z -> y (so becomes 2nd argument)
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getTDimId(), 1, 2, 3)); // t -> t (so becomes 4th argument)
//    //hence yzxt
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for yzxt schenario", unique(2, 3, 1, 4), function(1, 2, 3, 4).s);
//  }
//
//  void testRemapPoints_zxyt()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getYDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getZDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getXDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getTDimId(), 1, 2, 3));
//
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim); //zxyt
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for zxyt schenario", unique(3, 1, 2, 4), function(1, 2, 3, 4).s);
//  }
//
//  void testRemapPoints_txyz()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getYDimId(), 1, 2, 3)); // x -> y
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getZDimId(), 1, 2, 3)); // y -> z
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getTDimId(), 1, 2, 3)); // z -> t
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getXDimId(), 1, 2, 3)); // t -> x
//
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim); //txyz
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for txyz schenario", unique(4, 1, 2, 3), function(1, 2, 3, 4).s);
//  }
//
//  void testRemapPoints_txzy()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getYDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getTDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getZDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getXDimId(), 1, 2, 3));
//
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for txzy schenario", unique(4, 1, 3, 2), function(1, 2, 3, 4).s);
//  }
//
//
//  void testRemapPoints_tyxz()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getZDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getYDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getTDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getXDimId(), 1, 2, 3));
//
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for tyxz schenario", unique(4, 2, 1, 3), function(1, 2, 3, 4).s);
//  }
//
//  void testRemapPoints_tyzx()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getTDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getYDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getZDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getXDimId(), 1, 2, 3));
//
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for tyzx schenario", unique(4, 2, 3, 1), function(1, 2, 3, 4).s);
//  }
//
//    void testRemapPoints_tzxy()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getZDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getTDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getYDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getXDimId(), 1, 2, 3));
//
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for tzxy schenario", unique(4, 3, 1, 2), function(1, 2, 3, 4).s);
//  }
//
//        void testRemapPoints_tzyx()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getTDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getZDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getYDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getXDimId(), 1, 2, 3));
//
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for tzyx schenario", unique(4, 3, 2, 1), function(1, 2, 3, 4).s);
//  }
//
//  void testRemapPoints_xtyz()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getXDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getZDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getTDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getYDimId(), 1, 2, 3));
//
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for xtyz schenario", unique(1, 4, 2, 3), function(1, 2, 3, 4).s);
//  }
//
//  void testRemapPoints_xtzy()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getXDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getTDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getZDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getYDimId(), 1, 2, 3));
//
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for xtzy schenario", unique(1, 4, 3, 2), function(1, 2, 3, 4).s);
//  }
//
//  void testRemapPoints_ytxz()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getZDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getXDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getTDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getYDimId(), 1, 2, 3));
//
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for ytxz schenario", unique(2, 4, 1, 3), function(1, 2, 3, 4).s);
//  }
//
//  void testRemapPoints_ytzx()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getTDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getXDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getZDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getYDimId(), 1, 2, 3));
//
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for ytzx schenario", unique(2, 4, 3, 1), function(1, 2, 3, 4).s);
//  }
//
//    void testRemapPoints_ztxy()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getZDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getTDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getXDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getYDimId(), 1, 2, 3));
//
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for ztxy schenario", unique(3, 4, 1, 2), function(1, 2, 3, 4).s);
//  }
//
//  void testRemapPoints_ztyx()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getTDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getZDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getXDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getYDimId(), 1, 2, 3));
//
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for ztyx  schenario", unique(3, 4, 2, 1), function(1, 2, 3, 4).s);
//  }
//
//  void testRemapPoints_xytz()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getXDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getYDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getTDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getZDimId(), 1, 2, 3));
//
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for xytz schenario", unique(1, 2, 4, 3), function(1, 2, 3, 4).s);
//  }
//
//    void testRemapPoints_xzty()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getXDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getTDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getYDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getZDimId(), 1, 2, 3));
//
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for xzty schenario", unique(1, 3, 4, 2), function(1, 2, 3, 4).s);
//  }
//
//   void testRemapPoints_yxtz()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getYDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getXDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getTDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getZDimId(), 1, 2, 3));
//
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for yxtz schenario", unique(2, 1, 4, 3), function(1, 2, 3, 4).s);
//  }
//
//  void testRemapPoints_yztx()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getTDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getXDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getYDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getZDimId(), 1, 2, 3));
//
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for yztx schenario", unique(2, 3, 4, 1), function(1, 2, 3, 4).s);
//  }
//
//  void testRemapPoints_zxty()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getYDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getTDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getXDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getZDimId(), 1, 2, 3));
//
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for zxty schenario", unique(3, 1, 4, 2), function(1, 2, 3, 4).s);
//  }
//
//  void testRemapPoints_zytx()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getTDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getYDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getXDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getZDimId(), 1, 2, 3));
//
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);
//
//    GetPoint_fctn function = proxy->getMappedPointFunction();
//
//    uniqueArgumentCombination unique;
//    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for zytx schenario", unique(3, 2, 4, 1), function(1, 2, 3, 4).s);
//  }
//
//  void testRemapPoints_throws()
//  {
//    using namespace Mantid::VATES;
//    //We are going to use new mappings to describe how dimensions are actually to be used.
//    Mantid::VATES::Dimension_sptr xDim(new FakeDimension("--", 1, 2, 3));
//    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getYDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getXDimId(), 1, 2, 3));
//    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getZDimId(), 1, 2, 3));
//
//    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
//    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);
//
//    TSM_ASSERT_THROWS("Cannot rebind, so should throw, exception.", proxy->getMappedPointFunction(), std::runtime_error);
//  }


};
#endif
