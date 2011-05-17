#ifndef IMDWORKSPACE_PROXY_TEST_H_
#define IMDWORKSPACE_PROXY_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/IMDWorkspaceProxy.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

class IMDWorkspaceProxyTest: public CxxTest::TestSuite
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
    double operator()(size_t i, size_t j, size_t k, size_t t)
    {
      return static_cast<double>(i * 1000 + j * 100 + k * 10 + t);
    }
  };

  class MockIMDDimension: public Mantid::Geometry::IMDDimension
  {
  private:
    std::string m_id;
  public:
    MockIMDDimension(std::string id) : m_id(id) {}
    std::string getName() const {throw std::runtime_error("Not implemented");}
    std::string getUnits() const {throw std::runtime_error("Not implemented");}
    std::string getDimensionId() const {return m_id;}
    double getMaximum() const {throw std::runtime_error("Not implemented");}
    double getMinimum() const {throw std::runtime_error("Not implemented");};
    size_t getNBins() const {throw std::runtime_error("Not implemented");};
    std::string toXMLString() const {throw std::runtime_error("Not implemented");};
    double getX(size_t) const {throw std::runtime_error("Not implemented");};
    virtual ~MockIMDDimension()
    {
    }
  };

  class MockIMDWorkspace: public Mantid::API::IMDWorkspace
  {
  public:

    MOCK_CONST_METHOD0(id, const std::string());
    MOCK_CONST_METHOD0(getMemorySize, size_t());
    MOCK_CONST_METHOD1(getPoint,const Mantid::Geometry::SignalAggregate&(size_t index));
    MOCK_CONST_METHOD1(getCell,const Mantid::Geometry::SignalAggregate&(size_t dim1Increment));
    MOCK_CONST_METHOD2(getCell,const Mantid::Geometry::SignalAggregate&(size_t dim1Increment, size_t dim2Increment));
    MOCK_CONST_METHOD3(getCell,const Mantid::Geometry::SignalAggregate&(size_t dim1Increment, size_t dim2Increment, size_t dim3Increment));
    MOCK_CONST_METHOD4(getCell,const Mantid::Geometry::SignalAggregate&(size_t dim1Increment, size_t dim2Increment, size_t dim3Increment, size_t dim4Increment));

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
    MOCK_CONST_METHOD0(getNonIntegratedDimensions, Mantid::Geometry::VecIMDDimension_const_sptr());

    const Mantid::Geometry::SignalAggregate& getCell(...) const
    {
      throw std::runtime_error("Not Implemented");
    }
    double getSignalAt(size_t index1, size_t index2, size_t index3, size_t index4) const
    {
      uniqueArgumentCombination unique; //Creates a unique return value based on the inputs. Used to determine the arranagement of input arguments after remapping.
      return unique(index1, index2, index3, index4);
    }
    virtual ~MockIMDWorkspace()
    {
    }
  };

  /// Helper method. Creates a mock x Dimension by assigning a specified id to the getDimensionId return type on the mock object created.
  static Mantid::Geometry::IMDDimension* createXDimension()
  {
    using namespace testing;
    MockIMDDimension* p_xDim = new MockIMDDimension(getXDimId());
    return p_xDim;
  }

  /// Helper method. Creates a mock y Dimension by assigning a specified id to the getDimensionId return type on the mock object created.
  static Mantid::Geometry::IMDDimension* createYDimension()
  {
    using namespace testing;
    MockIMDDimension* p_yDim = new MockIMDDimension(getYDimId());
    return p_yDim;
  }

  /// Helper method. Creates a mock z Dimension by assigning a specified id to the getDimensionId return type on the mock object created.
  static Mantid::Geometry::IMDDimension* createZDimension()
  {
    using namespace testing;
    MockIMDDimension* p_zDim = new MockIMDDimension(getZDimId());
    return p_zDim;
  }

  /// Helper method. Creates a mock t Dimension by assigning a specified id to the getDimensionId return type on the mock object created.
  static Mantid::Geometry::IMDDimension* createTDimension()
  {
    using namespace testing;
    MockIMDDimension* p_tDim = new MockIMDDimension(getTDimId());
    return p_tDim;
  }

  /// Helper method. Creates a MDWorkspace with getXDimension, getYDimension ... already pre-setup.
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
    TS_ASSERT_EQUALS(getYDimId(), proxy->getZDimension()->getDimensionId());
    TS_ASSERT_EQUALS(getXDimId(), proxy->getTDimension()->getDimensionId());
  }

  void testRemapPoints_xyzt()
  {
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createXDimension());// x -> x
    Dimension_const_sptr yDim(createYDimension());// y -> y
    Dimension_const_sptr zDim(createZDimension());// z -> z
    Dimension_const_sptr tDim(createTDimension());// t -> t

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);
    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for xyzt scenario", unique(1, 2, 3, 4), proxy->getSignalAt(1, 2, 3, 4));

    //Additional test to ensure that we have characterised behaviour for getSignalNormalizedAt.
    TSM_ASSERT_EQUALS("Normalized signal should be characterised to be the same as signal", proxy->getSignalAt(1, 2, 3, 4), proxy->getSignalNormalizedAt(1, 2, 3, 4));
  }

  void testremappoints_xzyt()
  {
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createXDimension());// x -> x
    Dimension_const_sptr yDim(createZDimension());// y -> z
    Dimension_const_sptr zDim(createYDimension());// z -> y
    Dimension_const_sptr tDim(createTDimension());// t -> t

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("rebinding has not been done correctly for xzyt scenario", unique(1, 3, 2, 4),  proxy->getSignalAt(1, 2, 3, 4));
  }


  void testRemapPoints_yxzt()
  {
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createYDimension());// x -> y (so x becomes 2nd argument)
    Dimension_const_sptr yDim(createXDimension());// y -> x (so y becomes 1st argument)
    Dimension_const_sptr zDim(createZDimension());// z -> z (so z becomes 3rd argument)
    Dimension_const_sptr tDim(createTDimension());// t -> t (so t becomes 4th argument)

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for yxzt scenario", unique(2, 1, 3, 4), proxy->getSignalAt(1, 2, 3, 4));
  }

  void testRemapPoints_yzxt()
  {
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createZDimension());// x -> z (so becomes 3nd argument)
    Dimension_const_sptr yDim(createXDimension());// y -> x (so becomes 1st argument)
    Dimension_const_sptr zDim(createYDimension());// z -> y (so becomes 2nd argument)
    Dimension_const_sptr tDim(createTDimension());// t -> t (so becomes 4th argument)

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for yzxt scenario", unique(2, 3, 1, 4), proxy->getSignalAt(1, 2, 3, 4));
  }

  void testRemapPoints_zxyt()
  {
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createYDimension());
    Dimension_const_sptr yDim(createZDimension());
    Dimension_const_sptr zDim(createXDimension());
    Dimension_const_sptr tDim(createTDimension());

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for zxyt scenario", unique(3, 1, 2, 4), proxy->getSignalAt(1, 2, 3, 4));
  }

  void testRemapPoints_txyz()
  {
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createYDimension());
    Dimension_const_sptr yDim(createZDimension());
    Dimension_const_sptr zDim(createTDimension());
    Dimension_const_sptr tDim(createXDimension());

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for txyz scenario", unique(4, 1, 2, 3), proxy->getSignalAt(1, 2, 3, 4));
  }

  void testRemapPoints_txzy()
  {
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createYDimension());
    Dimension_const_sptr yDim(createTDimension());
    Dimension_const_sptr zDim(createZDimension());
    Dimension_const_sptr tDim(createXDimension());

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for txzy scenario", unique(4, 1, 3, 2), proxy->getSignalAt(1, 2, 3, 4));
  }


  void testRemapPoints_tyxz()
  {
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createZDimension());
    Dimension_const_sptr yDim(createYDimension());
    Dimension_const_sptr zDim(createTDimension());
    Dimension_const_sptr tDim(createXDimension());

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for tyxz scenario", unique(4, 2, 1, 3), proxy->getSignalAt(1, 2, 3, 4));
  }

  void testRemapPoints_tyzx()
  {

    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createTDimension());
    Dimension_const_sptr yDim(createYDimension());
    Dimension_const_sptr zDim(createZDimension());
    Dimension_const_sptr tDim(createXDimension());

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for tyzx scenario", unique(4, 2, 3, 1), proxy->getSignalAt(1, 2, 3, 4));
  }

  void testRemapPoints_tzxy()
  {
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createZDimension());
    Dimension_const_sptr yDim(createTDimension());
    Dimension_const_sptr zDim(createYDimension());
    Dimension_const_sptr tDim(createXDimension());

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for tzxy scenario", unique(4, 3, 1, 2), proxy->getSignalAt(1, 2, 3, 4));
  }

  void testRemapPoints_tzyx()
  {
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createTDimension());
    Dimension_const_sptr yDim(createZDimension());
    Dimension_const_sptr zDim(createYDimension());
    Dimension_const_sptr tDim(createXDimension());

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for tzyx scenario", unique(4, 3, 2, 1), proxy->getSignalAt(1, 2, 3, 4));
  }

  void testRemapPoints_xtyz()
  {
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createXDimension());
    Dimension_const_sptr yDim(createZDimension());
    Dimension_const_sptr zDim(createTDimension());
    Dimension_const_sptr tDim(createYDimension());

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for xtyz scenario", unique(1, 4, 2, 3), proxy->getSignalAt(1, 2, 3, 4));
  }

  void testRemapPoints_xtzy()
  {
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createXDimension());
    Dimension_const_sptr yDim(createTDimension());
    Dimension_const_sptr zDim(createZDimension());
    Dimension_const_sptr tDim(createYDimension());

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for xtzy scenario", unique(1, 4, 3, 2), proxy->getSignalAt(1, 2, 3, 4));
  }

  void testRemapPoints_ytxz()
  {
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createZDimension());
    Dimension_const_sptr yDim(createXDimension());
    Dimension_const_sptr zDim(createTDimension());
    Dimension_const_sptr tDim(createYDimension());

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for ytxz scenario", unique(2, 4, 1, 3), proxy->getSignalAt(1, 2, 3, 4));
  }

  void testRemapPoints_ytzx()
  {
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createTDimension());
    Dimension_const_sptr yDim(createXDimension());
    Dimension_const_sptr zDim(createZDimension());
    Dimension_const_sptr tDim(createYDimension());

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for ytzx scenario", unique(2, 4, 3, 1), proxy->getSignalAt(1, 2, 3, 4));
  }

    void testRemapPoints_ztxy()
  {

    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createZDimension());
    Dimension_const_sptr yDim(createTDimension());
    Dimension_const_sptr zDim(createXDimension());
    Dimension_const_sptr tDim(createYDimension());

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for ztxy scenario", unique(3, 4, 1, 2), proxy->getSignalAt(1, 2, 3, 4));
  }

  void testRemapPoints_ztyx()
  {
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createTDimension());
    Dimension_const_sptr yDim(createZDimension());
    Dimension_const_sptr zDim(createXDimension());
    Dimension_const_sptr tDim(createYDimension());

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for ztyx  scenario", unique(3, 4, 2, 1), proxy->getSignalAt(1, 2, 3, 4));
  }

  void testRemapPoints_xytz()
  {
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createXDimension());
    Dimension_const_sptr yDim(createYDimension());
    Dimension_const_sptr zDim(createTDimension());
    Dimension_const_sptr tDim(createZDimension());

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for xytz scenario", unique(1, 2, 4, 3), proxy->getSignalAt(1, 2, 3, 4));
  }

    void testRemapPoints_xzty()
  {
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createXDimension());
    Dimension_const_sptr yDim(createTDimension());
    Dimension_const_sptr zDim(createYDimension());
    Dimension_const_sptr tDim(createZDimension());

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for xzty scenario", unique(1, 3, 4, 2), proxy->getSignalAt(1, 2, 3, 4));
  }

   void testRemapPoints_yxtz()
  {
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createYDimension());
    Dimension_const_sptr yDim(createXDimension());
    Dimension_const_sptr zDim(createTDimension());
    Dimension_const_sptr tDim(createZDimension());

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for yxtz scenario", unique(2, 1, 4, 3), proxy->getSignalAt(1, 2, 3, 4));
  }

  void testRemapPoints_yztx()
  {
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createTDimension());
    Dimension_const_sptr yDim(createXDimension());
    Dimension_const_sptr zDim(createYDimension());
    Dimension_const_sptr tDim(createZDimension());

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for yztx scenario", unique(2, 3, 4, 1), proxy->getSignalAt(1, 2, 3, 4));
  }

  void testRemapPoints_zxty()
  {
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createYDimension());
    Dimension_const_sptr yDim(createTDimension());
    Dimension_const_sptr zDim(createXDimension());
    Dimension_const_sptr tDim(createZDimension());

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for zxty scenario", unique(3, 1, 4, 2), proxy->getSignalAt(1, 2, 3, 4));
  }

  void testRemapPoints_zytx()
  {
    using Mantid::API::IMDWorkspace_sptr;
    using Mantid::VATES::IMDWorkspaceProxy;
    using Mantid::VATES::Dimension_const_sptr;

    IMDWorkspace_sptr mock_sptr( createMockIMDWorkspace());

    Dimension_const_sptr xDim(createTDimension());
    Dimension_const_sptr yDim(createYDimension());
    Dimension_const_sptr zDim(createXDimension());
    Dimension_const_sptr tDim(createZDimension());

    IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(mock_sptr, xDim, yDim, zDim, tDim);

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for zytx scenario", unique(3, 2, 4, 1), proxy->getSignalAt(1, 2, 3, 4));
  }

};
#endif
