#ifndef MANTID_CRYSTAL_COMPOSITECLUSTERTEST_H_
#define MANTID_CRYSTAL_COMPOSITECLUSTERTEST_H_

#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidCrystal/CompositeCluster.h"
#include "MockObjects.h"
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace testing;

namespace {
struct null_deleter {
  void operator()(void const *) const {}
};
} // namespace

class CompositeClusterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CompositeClusterTest *createSuite() {
    return new CompositeClusterTest();
  }
  static void destroySuite(CompositeClusterTest *suite) { delete suite; }

  void test_dont_add_if_child_empty() {
    MockICluster *pMockCluster = new MockICluster();
    EXPECT_CALL(*pMockCluster, size())
        .WillRepeatedly(Return(0)); // Size==0, empty child cluster.
    boost::shared_ptr<ICluster> mockCluster(pMockCluster);

    CompositeCluster composite;
    composite.add(mockCluster);
    TSM_ASSERT_EQUALS("Should not have added cluster", 0, composite.size());

    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockCluster));
  }

  void testAdd() {

    MockICluster *pMockCluster = new MockICluster();
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

  void test_addIndex_throws() {
    CompositeCluster cluster;
    TS_ASSERT_THROWS(cluster.addIndex(1), std::runtime_error &);
  }

  void test_initialSize() {
    CompositeCluster cluster;
    TS_ASSERT_EQUALS(0, cluster.size());
  }

  void test_getLabel_when_empty_throws() {
    CompositeCluster cluster;
    TS_ASSERT_THROWS(cluster.getLabel(), std::runtime_error &);
  }

  void test_writeTo() {
    MockICluster *pMockCluster = new MockICluster();
    EXPECT_CALL(*pMockCluster, writeTo(_)).Times(2);
    EXPECT_CALL(*pMockCluster, size())
        .WillRepeatedly(Return(1)); // Fake the size as non-zero otherwise will
                                    // not be added to composite cluster
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

  void test_integrate() {
    MockICluster *pMockCluster = new MockICluster();
    EXPECT_CALL(*pMockCluster, size())
        .WillRepeatedly(Return(1)); // Fake the size as non-zero otherwise will
                                    // not be added to composite cluster
    EXPECT_CALL(*pMockCluster, integrate(_))
        .WillRepeatedly(Return(ICluster::ClusterIntegratedValues(1, 2)));
    boost::shared_ptr<ICluster> mockCluster1(pMockCluster, null_deleter());
    boost::shared_ptr<ICluster> mockCluster2(pMockCluster, null_deleter());

    CompositeCluster composite;
    composite.add(mockCluster1);
    composite.add(mockCluster2);

    IMDHistoWorkspace_sptr inWS;
    ICluster::ClusterIntegratedValues result = composite.integrate(inWS);
    TS_ASSERT_EQUALS(result.get<0>(), 2 * 1);
    TS_ASSERT_EQUALS(result.get<1>(), 2 * 2);
    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockCluster));
    delete pMockCluster;
  }

  void test_to_uniform_min() {
    std::vector<DisjointElement> disjointSet;

    MockICluster *pMockClusterA = new MockICluster();
    MockICluster *pMockClusterB = new MockICluster();
    boost::shared_ptr<ICluster> mockClusterA(pMockClusterA);
    boost::shared_ptr<ICluster> mockClusterB(pMockClusterB);

    EXPECT_CALL(*pMockClusterA, size())
        .WillRepeatedly(Return(1)); // Fake the size as non-zero otherwise will
                                    // not be added to composite cluster
    EXPECT_CALL(*pMockClusterB, size())
        .WillRepeatedly(Return(1)); // Fake the size as non-zero otherwise will
                                    // not be added to composite cluster
    EXPECT_CALL(*pMockClusterA, getLabel())
        .WillRepeatedly(Return(1)); // Max label
    EXPECT_CALL(*pMockClusterB, getLabel())
        .WillRepeatedly(Return(0)); // Min label
    EXPECT_CALL(*pMockClusterA, setRootCluster(pMockClusterB))
        .Times(1); // Use minimum as root
    EXPECT_CALL(*pMockClusterB, setRootCluster(pMockClusterB))
        .Times(1); // Use minimum as root
    EXPECT_CALL(*pMockClusterA, toUniformMinimum(_))
        .Times(1); // Use minimum as root
    EXPECT_CALL(*pMockClusterB, toUniformMinimum(_))
        .Times(1); // Use minimum as root

    CompositeCluster composite;
    composite.add(mockClusterA);
    composite.add(mockClusterB);

    composite.toUniformMinimum(disjointSet);
    TSM_ASSERT_EQUALS("Label should be minimum of subjects", 0,
                      composite.getLabel());

    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockClusterA));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockClusterB));
  }

  void test_isLabelInSet() {
    MockICluster *pMockClusterA = new MockICluster();
    MockICluster *pMockClusterB = new MockICluster();
    boost::shared_ptr<ICluster> mockClusterA(pMockClusterA);
    boost::shared_ptr<ICluster> mockClusterB(pMockClusterB);

    EXPECT_CALL(*pMockClusterA, size())
        .WillRepeatedly(Return(1)); // Fake the size as non-zero otherwise will
                                    // not be added to composite cluster
    EXPECT_CALL(*pMockClusterB, size())
        .WillRepeatedly(Return(1)); // Fake the size as non-zero otherwise will
                                    // not be added to composite cluster
    EXPECT_CALL(*pMockClusterA, getLabel())
        .WillRepeatedly(Return(1)); // Label 1 in set
    EXPECT_CALL(*pMockClusterB, getLabel())
        .WillRepeatedly(Return(2)); // Label 2 in set

    CompositeCluster composite;
    composite.add(mockClusterA);
    composite.add(mockClusterB);

    TS_ASSERT(!composite.containsLabel(3));
    TS_ASSERT(composite.containsLabel(1));
    TS_ASSERT(composite.containsLabel(2));
  }
};

#endif /* MANTID_CRYSTAL_COMPOSITECLUSTERTEST_H_ */
