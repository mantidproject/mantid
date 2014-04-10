#ifndef MANTID_CRYSTAL_CLUSTERTEST_H_
#define MANTID_CRYSTAL_CLUSTERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidCrystal/Cluster.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

using Mantid::Crystal::Cluster;
using namespace Mantid::API;
using namespace Mantid::MDEvents;

class ClusterTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ClusterTest *createSuite() { return new ClusterTest(); }
  static void destroySuite( ClusterTest *suite ) { delete suite; }


  void test_construction()
  {
    const size_t label = 1;
    Cluster cluster(label);
    TS_ASSERT_EQUALS(cluster.getLabel(), label);
  }

  void test_do_integration()
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1, 6); // Makes a 1 by 6 md ws with identical signal values.
    Cluster cluster(1);
    for(size_t i = 0; i < inWS->getNPoints(); ++i)
    {
      cluster.addIndex(i); // Register all indexes from the workspace to the cluster.
    }
    auto result = cluster.integrate(inWS);
    TS_ASSERT_EQUALS(6*1, result.get<0>());
    TS_ASSERT_EQUALS(6*1, result.get<1>());
  }

  void test_size()
  {
    Cluster cluster(1);
    TS_ASSERT_EQUALS(cluster.size(), 0);
    cluster.addIndex(0);
    TS_ASSERT_EQUALS(cluster.size(), 1);
  }

  void test_writeTo()
  {
    const double nolabel = 0;
    const double label = 2;
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(nolabel, 1, 6); // Makes a 1 by 6 md ws with identical signal values.
    const size_t labelId = static_cast<size_t>(label);
    Cluster cluster(labelId);
    cluster.addIndex(1);
    cluster.addIndex(2);
    cluster.writeTo(inWS);
    TS_ASSERT_EQUALS(nolabel, inWS->getSignalAt(0));
    TS_ASSERT_EQUALS(label, inWS->getSignalAt(1));
    TS_ASSERT_EQUALS(label, inWS->getSignalAt(2));
    TS_ASSERT_EQUALS(nolabel, inWS->getSignalAt(3));
  }

  void test_size_with_children()
  {
    Cluster clusterOne(1);
    clusterOne.addIndex(0);
    TS_ASSERT_EQUALS(clusterOne.size(), 1);
    Cluster* clusterTwo = new Cluster(2);
    clusterTwo->addIndex(1);
    boost::shared_ptr<const Cluster> clusterTwo_sptr(clusterTwo);
    clusterOne.attachCluster(clusterTwo_sptr);
    TSM_ASSERT_EQUALS("Size should include children",clusterOne.size(), 2);
  }

  void test_append_and_integrate()
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1, 6); // Makes a 1 by 6 md ws with identical signal values.
    Cluster clusterA(1);
    Cluster* clusterB = new Cluster(2);
    for(size_t i = 0; i < inWS->getNPoints(); ++i)
    {
      clusterA.addIndex(i); // Register all indexes from the workspace to the cluster.
      clusterB->addIndex(i); // Register all indexes from the workspace to the cluster.
    }
    boost::shared_ptr<const Cluster> clusterB_sptr(clusterB);
    clusterA.attachCluster(clusterB_sptr);
    auto resultB = clusterB->integrate(inWS);
    TS_ASSERT_EQUALS(6*1, resultB.get<0>());
    TS_ASSERT_EQUALS(6*1, resultB.get<1>());
    auto resultA = clusterA.integrate(inWS);
    TSM_ASSERT_EQUALS("Cluster should integrate children", 6*1*2, resultA.get<0>());
    TSM_ASSERT_EQUALS("Cluster should integrate children", 6*1*2, resultA.get<1>());
  }

  void test_append_and_writeTo()
  {
    const double nolabel = 0;
    const double label = 1;

    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(nolabel, 1, 6); // Makes a 1 by 6 md ws with identical signal values.
    const size_t labelId = static_cast<size_t>(label);
    Cluster clusterOne(labelId);
    Cluster* clusterTwo = new Cluster(labelId);
    clusterOne.addIndex(1);
    clusterOne.addIndex(2);
    clusterTwo->addIndex(3);
    boost::shared_ptr<const Cluster> clusterTwo_sptr(clusterTwo);
    clusterOne.attachCluster(clusterTwo_sptr);
    clusterOne.writeTo(inWS);
    TS_ASSERT_EQUALS(nolabel, inWS->getSignalAt(0));
    TS_ASSERT_EQUALS(label, inWS->getSignalAt(1));
    TS_ASSERT_EQUALS(label, inWS->getSignalAt(2));
    TS_ASSERT_EQUALS(label, inWS->getSignalAt(3));
    TS_ASSERT_EQUALS(nolabel, inWS->getSignalAt(4));
  }

};


#endif /* MANTID_CRYSTAL_CLUSTERTEST_H_ */
