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

  void test_get_integrated_signal_without_integrating_throws()
  {
    Cluster cluster(1);
    TS_ASSERT_THROWS(cluster.getSignalInt(), std::runtime_error&);
    
  }

  void test_get_integrated_errorSQ_without_integrating_throws()
  {
    Cluster cluster(1);
    TS_ASSERT_THROWS(cluster.getErrorSQInt(), std::runtime_error&);
  }

  void test_do_integration()
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1, 6); // Makes a 1 by 6 md ws with identical signal values.
    Cluster cluster(1);
    for(size_t i = 0; i < inWS->getNPoints(); ++i)
    {
      cluster.addIndex(i); // Register all indexes from the workspace to the cluster.
    }
    cluster.integrate(inWS);
    TS_ASSERT_EQUALS(6*1, cluster.getSignalInt());
    TS_ASSERT_EQUALS(6*1, cluster.getErrorSQInt());
  }

  void test_size()
  {
    Cluster cluster(1);
    TS_ASSERT_EQUALS(cluster.size(), 0);
    cluster.addIndex(0);
    TS_ASSERT_EQUALS(cluster.size(), 1);
  }

};


#endif /* MANTID_CRYSTAL_CLUSTERTEST_H_ */