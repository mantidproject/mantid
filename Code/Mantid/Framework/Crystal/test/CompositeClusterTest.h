#ifndef MANTID_CRYSTAL_COMPOSITECLUSTERTEST_H_
#define MANTID_CRYSTAL_COMPOSITECLUSTERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidCrystal/CompositeCluster.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include <gmock/gmock.h>
#include <boost/shared_ptr.hpp>

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace testing;

namespace
{
  struct null_deleter
  {
    void operator()(void const *) const
    {
    }
  };
}

class CompositeClusterTest: public CxxTest::TestSuite
{
private:

  class MockICluster: public ICluster
  {
  public:
    MOCK_CONST_METHOD1(integrate,
        ClusterIntegratedValues(boost::shared_ptr<const Mantid::API::IMDHistoWorkspace> ws));
    MOCK_CONST_METHOD1(writeTo,
        void(boost::shared_ptr<Mantid::API::IMDHistoWorkspace> ws));
    MOCK_CONST_METHOD0(getLabel,
        size_t());
    MOCK_CONST_METHOD0(getOriginalLabel,
        size_t());
    MOCK_CONST_METHOD0(size,
        size_t());
    MOCK_METHOD1(addIndex,
        void(const size_t& index));
    MOCK_METHOD1(toUniformMinimum,
        void(std::vector<DisjointElement>& disjointSet));
  };

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CompositeClusterTest *createSuite()
  { return new CompositeClusterTest();}
  static void destroySuite( CompositeClusterTest *suite )
  { delete suite;}

  void testAdd()
  {

    MockICluster* pMockCluster = new MockICluster();
    EXPECT_CALL(*pMockCluster, size()).WillRepeatedly(Return(1));
    boost::shared_ptr<ICluster> mockCluster1(pMockCluster, null_deleter());
    boost::shared_ptr<ICluster> mockCluster2(pMockCluster, null_deleter());

    CompositeCluster composite;
    composite.add(mockCluster1);
    composite.add(mockCluster2);
    TS_ASSERT_EQUALS(2, composite.size());

    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockCluster));
    delete pMockCluster;
  }

  void test_addIndex_throws()
  {
    CompositeCluster cluster;
    TS_ASSERT_THROWS(cluster.addIndex(1), std::runtime_error&);
  }

  void test_initialSize()
  {
    CompositeCluster cluster;
    TS_ASSERT_EQUALS(0, cluster.size());
  }

  void test_getLabel_when_empty_throws()
  {
    CompositeCluster cluster;
    TS_ASSERT_THROWS(cluster.getLabel(), std::runtime_error&);
  }

  void test_writeTo()
  {
    MockICluster* pMockCluster = new MockICluster();
    EXPECT_CALL(*pMockCluster, writeTo(_)).Times(2);
    boost::shared_ptr<ICluster> mockCluster1(pMockCluster, null_deleter());
    boost::shared_ptr<ICluster> mockCluster2(pMockCluster, null_deleter());

    CompositeCluster composite;
    composite.add(mockCluster1);
    composite.add(mockCluster2);

    IMDHistoWorkspace_sptr inWS;
    composite.writeTo(inWS);

    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockCluster));
    delete pMockCluster;
  }

  void test_integrate()
  {
    MockICluster* pMockCluster = new MockICluster();
    EXPECT_CALL(*pMockCluster, integrate(_)).WillRepeatedly(Return(ICluster::ClusterIntegratedValues(1,2)));
    boost::shared_ptr<ICluster> mockCluster1(pMockCluster, null_deleter());
    boost::shared_ptr<ICluster> mockCluster2(pMockCluster, null_deleter());

    CompositeCluster composite;
    composite.add(mockCluster1);
    composite.add(mockCluster2);

    IMDHistoWorkspace_sptr inWS;
    ICluster::ClusterIntegratedValues result = composite.integrate(inWS);
    TS_ASSERT_EQUALS(result.get<0>(), 2*1);
    TS_ASSERT_EQUALS(result.get<1>(), 2*2);
    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockCluster));
    delete pMockCluster;
  }
};

#endif /* MANTID_CRYSTAL_COMPOSITECLUSTERTEST_H_ */
