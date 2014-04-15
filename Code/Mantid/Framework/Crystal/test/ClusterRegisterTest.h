#ifndef MANTID_CRYSTAL_CLUSTERREGISTERTEST_H_
#define MANTID_CRYSTAL_CLUSTERREGISTERTEST_H_

#include <cxxtest/TestSuite.h>
#include <boost/make_shared.hpp>
#include "MantidCrystal/ClusterRegister.h"
#include "MantidCrystal/CompositeCluster.h"
#include "MantidCrystal/Cluster.h"
#include "MockObjects.h"

using namespace Mantid::Crystal;
using namespace testing;

class ClusterRegisterTest: public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ClusterRegisterTest *createSuite()
  {
    return new ClusterRegisterTest();
  }
  static void destroySuite(ClusterRegisterTest *suite)
  {
    delete suite;
  }

  void test_addClusters()
  {
    ClusterRegister cRegister;
    cRegister.add(1, boost::make_shared<MockICluster>());
    cRegister.add(2, boost::make_shared<MockICluster>());

    auto clusters = cRegister.clusters();
    TS_ASSERT_EQUALS(2, clusters.size());
  }

  void test_try_addClusters_with_duplicate_keys()
  {
    ClusterRegister cRegister;
    cRegister.add(1, boost::make_shared<MockICluster>());
    cRegister.add(1, boost::make_shared<MockICluster>());

    auto clusters = cRegister.clusters();
    TS_ASSERT_EQUALS(1, clusters.size());
  }

  void test_simple_merge()
  {
    ClusterRegister cRegister;
    auto a = boost::make_shared<MockICluster>();
    auto b = boost::make_shared<MockICluster>();
    auto c = boost::make_shared<MockICluster>();
    cRegister.add(1, a);
    cRegister.add(2, b);
    cRegister.add(3, c);
    EXPECT_CALL(*a.get(), getLabel()).WillRepeatedly(Return(1));
    EXPECT_CALL(*b.get(), getLabel()).WillRepeatedly(Return(2));
    EXPECT_CALL(*c.get(), getLabel()).WillRepeatedly(Return(3));
    cRegister.merge(DisjointElement(2), DisjointElement(3)); // Merge clusters 2 and 3

    auto combined = cRegister.clusters();
    TS_ASSERT_EQUALS(2, combined.size());
    TS_ASSERT(combined.find(1) != combined.end());
    TS_ASSERT(combined.find(2) != combined.end());
    TS_ASSERT(boost::dynamic_pointer_cast<ICluster>(combined[1]));
    TS_ASSERT(boost::dynamic_pointer_cast<CompositeCluster>(combined[2]));
    for(auto it = combined.begin(); it != combined.end(); ++it)
    {
      std::cout << "Type is " << typeid(it->second.get()).name() << std::endl;
    }
  }

  void test_simple_merge_repeat()
  {
    ClusterRegister cRegister;
    auto a = boost::make_shared<MockICluster>();
    auto b = boost::make_shared<MockICluster>();
    auto c = boost::make_shared<MockICluster>();
    cRegister.add(1, a);
    cRegister.add(2, b);
    cRegister.add(3, c);
    EXPECT_CALL(*a.get(), getLabel()).WillRepeatedly(Return(1));
    EXPECT_CALL(*b.get(), getLabel()).WillRepeatedly(Return(2));
    EXPECT_CALL(*c.get(), getLabel()).WillRepeatedly(Return(3));
    cRegister.merge(DisjointElement(2), DisjointElement(3)); // Merge clusters 2 and 3
    cRegister.merge(DisjointElement(3), DisjointElement(2)); // This is a duplicate call that should be ignored.

    auto combined = cRegister.clusters();
    TS_ASSERT_EQUALS(2, combined.size());
    TS_ASSERT(combined.find(1) != combined.end());
    TS_ASSERT(combined.find(2) != combined.end());
    TS_ASSERT(boost::dynamic_pointer_cast<ICluster>(combined[1]));
    TS_ASSERT(boost::dynamic_pointer_cast<CompositeCluster>(combined[2]));
  }

  void test_multi_merge()
  {
    ClusterRegister cRegister;
    auto a = boost::make_shared<MockICluster>();
    auto b = boost::make_shared<MockICluster>();
    auto c = boost::make_shared<MockICluster>();
    cRegister.add(1, a);
    cRegister.add(2, b);
    cRegister.add(3, c);
    EXPECT_CALL(*a.get(), getLabel()).WillRepeatedly(Return(1));
    EXPECT_CALL(*b.get(), getLabel()).WillRepeatedly(Return(2));
    EXPECT_CALL(*c.get(), getLabel()).WillRepeatedly(Return(3));
    EXPECT_CALL(*a.get(), getOriginalLabel()).WillRepeatedly(Return(1));
    EXPECT_CALL(*b.get(), getOriginalLabel()).WillRepeatedly(Return(2));
    EXPECT_CALL(*c.get(), getOriginalLabel()).WillRepeatedly(Return(3));
    cRegister.merge(DisjointElement(2), DisjointElement(3)); // Merge clusters 2 and 3
    cRegister.merge(DisjointElement(1), DisjointElement(2)); // Merge clusters 1 and 2

    auto combined = cRegister.clusters();
    TS_ASSERT_EQUALS(1, combined.size());
    TSM_ASSERT("Combined all clusters, so should have a single Composite cluster.",
        boost::dynamic_pointer_cast<CompositeCluster>(combined[0]));
  }

};

#endif /* MANTID_CRYSTAL_CLUSTERREGISTERTEST_H_ */
