#ifndef MANTID_CRYSTAL_CONNECTEDCOMPONENTLABELINGTEST_H_
#define MANTID_CRYSTAL_CONNECTEDCOMPONENTLABELINGTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <set>
#include <algorithm>
#include <boost/assign/list_of.hpp>
#include <boost/scoped_ptr.hpp>

#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Progress.h"
#include "MantidCrystal/ConnectedComponentLabeling.h"
#include "MantidCrystal/BackgroundStrategy.h"
#include "MantidCrystal/HardThresholdBackground.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::MDEvents;
using namespace testing;

namespace
{
  // Helper function for determining if a set contains a specific value.
  template<typename T>
  bool does_set_contain(const std::set<T>& container, const T& value)
  {
    return std::find(container.begin(), container.end(), value) != container.end();
  }

  // Helper function for determining if a set contains a specific value.
  bool does_vector_contain(const std::vector<size_t>& container, const size_t& value)
  {
    return std::find(container.begin(), container.end(), value) != container.end();
  }

  // Helper function for converting a IMDHistoWorkspace of labels into a set of unique labels.
  std::set<size_t> connection_workspace_to_set_of_labels(IMDHistoWorkspace const * const ws)
  {
    std::set<size_t> unique_values;
    for (size_t i = 0; i < ws->getNPoints(); ++i)
    {
      unique_values.insert(static_cast<size_t>(ws->getSignalAt(i)));
    }
    return unique_values;
  }

}

//=====================================================================================
// Functional Tests
//=====================================================================================
class ConnectedComponentLabelingTest: public CxxTest::TestSuite
{
private:

  // Mock Background strategy
  class MockBackgroundStrategy: public BackgroundStrategy
  {
  public:
    MOCK_CONST_METHOD1(configureIterator, void(Mantid::API::IMDIterator* const));
    MOCK_CONST_METHOD1(isBackground, bool(Mantid::API::IMDIterator* const));
    MockBackgroundStrategy* clone() const
    {
      throw std::runtime_error("Cannot clone the mock object");
    }
    virtual ~MockBackgroundStrategy()
    {}
  };

  const size_t m_emptyLabel;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConnectedComponentLabelingTest *createSuite()
  { return new ConnectedComponentLabelingTest();}
  static void destroySuite( ConnectedComponentLabelingTest *suite )
  { delete suite;}

  ConnectedComponentLabelingTest() : m_emptyLabel(0)
  {
    FrameworkManager::Instance();
  }

  void test_default_start_label_id()
  {
    ConnectedComponentLabeling ccl;
    TSM_ASSERT_EQUALS("Start Label Id should be 1 by default", 1, ccl.getStartLabelId());
  }

  void test_set_get_start_label_id()
  {
    ConnectedComponentLabeling ccl;
    const size_t startLabelId = 10;
    ccl.startLabelingId(startLabelId);
    TS_ASSERT_EQUALS(startLabelId, ccl.getStartLabelId())
  }

  void test_1d_one_node()
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1, 1); // Single node. Simpliest possible test case

    MockBackgroundStrategy mockStrategy;
    EXPECT_CALL(mockStrategy, isBackground(_)).Times(static_cast<int>(inWS->getNPoints())).WillRepeatedly(Return(false));// A filter that passes everything.

    size_t labelingId = 1;
    bool multiThreaded = false;
    ConnectedComponentLabeling ccl(labelingId, multiThreaded);
    
    ccl.startLabelingId(labelingId);
    Progress prog;
    auto outWS = ccl.execute(inWS, &mockStrategy, prog);

    auto uniqueValues = connection_workspace_to_set_of_labels(outWS.get());
    TS_ASSERT_EQUALS(1, uniqueValues.size());
    TS_ASSERT(does_set_contain(uniqueValues, labelingId));

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockStrategy));
  }

  void test_1d_with_one_object()
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1, 10); // Makes a 1 by 10 md ws with identical signal values.

    MockBackgroundStrategy mockStrategy;
    EXPECT_CALL(mockStrategy, isBackground(_)).Times(static_cast<int>(inWS->getNPoints())).WillRepeatedly(Return(false));// A filter that passes everything.

    size_t labelingId = 2;
    bool multiThreaded = false;
    ConnectedComponentLabeling ccl(labelingId, multiThreaded);
    Progress prog;
    auto outWS = ccl.execute(inWS, &mockStrategy, prog);

    /*
     * Because all the signal values are identical, and none are below any threshold. We assume that there will only be a single component. All
     * signal values in the output workspace should bear the first component identifier label. i.e one big object.
     */
    auto uniqueValues = connection_workspace_to_set_of_labels(outWS.get());
    TS_ASSERT_EQUALS(1, uniqueValues.size());
    TS_ASSERT(does_set_contain(uniqueValues, labelingId));

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockStrategy));
  }

  void test_1d_with_double_object()
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1, 6); // Makes a 1 by 6 md ws with identical signal values.

    MockBackgroundStrategy mockStrategy;

    /*
     * We use the is background strategy to set up two disconected blocks for us.
     * */
    EXPECT_CALL(mockStrategy, isBackground(_))
    .WillOnce(Return(false))
    .WillOnce(Return(false))
    .WillOnce(Return(true)) // is background
    .WillRepeatedly(Return(false));

    size_t labelingId = 1;
    bool multiThreaded = false;
    Progress prog;
    ConnectedComponentLabeling ccl(labelingId, multiThreaded);
    auto outWS = ccl.execute(inWS, &mockStrategy, prog);

    std::set<size_t> uniqueEntries = connection_workspace_to_set_of_labels(outWS.get());
    TSM_ASSERT_EQUALS("2 objects so should have 3 unique entries", 3, uniqueEntries.size());
    TS_ASSERT(does_set_contain(uniqueEntries, labelingId));
    TS_ASSERT(does_set_contain(uniqueEntries, m_emptyLabel));// Background entries.
    TS_ASSERT(does_set_contain(uniqueEntries, labelingId+1));

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockStrategy));
  }

  void test_1d_with_tripple_object()
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1, 5); // Makes a 1 by 5 md ws with identical signal values.

    MockBackgroundStrategy mockStrategy;

    /*
     * We use the is background strategy to set up three disconected blocks for us.
     * */
    EXPECT_CALL(mockStrategy, isBackground(_))
    .WillOnce(Return(false))
    .WillOnce(Return(true)) // is background
    .WillOnce(Return(false))
    .WillOnce(Return(true))// is background
    .WillOnce(Return(false));

    size_t labelingId = 1;
    bool multiThreaded = false;
    ConnectedComponentLabeling ccl(labelingId, multiThreaded);
    Progress prog;
    auto outWS = ccl.execute(inWS, &mockStrategy, prog);

    std::set<size_t> uniqueEntries = connection_workspace_to_set_of_labels(outWS.get());
    TSM_ASSERT_EQUALS("3 objects so should have 4 unique entries", 4, uniqueEntries.size());
    TS_ASSERT(does_set_contain(uniqueEntries, labelingId));
    TS_ASSERT(does_set_contain(uniqueEntries, m_emptyLabel));// Background entries.
    TS_ASSERT(does_set_contain(uniqueEntries, ++labelingId));
    TS_ASSERT(does_set_contain(uniqueEntries, ++labelingId));

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockStrategy));
  }

  void test_2d_with_single_object()
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 2, 4); // Makes a 4 by 4 grid.

    MockBackgroundStrategy mockStrategy;

    EXPECT_CALL(mockStrategy, isBackground(_)).WillRepeatedly(Return(false));// Nothing is treated as background
    size_t labelingId = 1;
    bool multiThreaded = false;
    ConnectedComponentLabeling ccl(labelingId, multiThreaded);
    Progress prog;
    auto outWS = ccl.execute(inWS, &mockStrategy, prog);

    std::set<size_t> uniqueEntries = connection_workspace_to_set_of_labels(outWS.get());
    TSM_ASSERT_EQUALS("Just one object", 1, uniqueEntries.size());
    TS_ASSERT(does_set_contain(uniqueEntries, labelingId));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockStrategy));
  }

  void test_2d_chequred_pattern()
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 2, 3); // Makes a 3 by 3 grid.

    MockBackgroundStrategy mockStrategy;

    /*
     * We treat alternate cells as background, which actually should result in a single object. Think of a chequered flag.
     * */
    EXPECT_CALL(mockStrategy, isBackground(_))
    .WillOnce(Return(true))
    .WillOnce(Return(false))
    .WillOnce(Return(true))
    .WillOnce(Return(false))
    .WillOnce(Return(true))
    .WillOnce(Return(false))
    .WillOnce(Return(true))
    .WillOnce(Return(false))
    .WillOnce(Return(true));

    size_t labelingId = 1;
    bool multiThreaded = false;
    ConnectedComponentLabeling ccl(labelingId, multiThreaded);
    Progress prog;
    auto outWS = ccl.execute(inWS, &mockStrategy, prog);

    std::set<size_t> uniqueEntries = connection_workspace_to_set_of_labels(outWS.get());
    TSM_ASSERT_EQUALS("Just one object, but we have some 'empty' entries too", 2, uniqueEntries.size());
    TS_ASSERT(does_set_contain(uniqueEntries, labelingId));
    TS_ASSERT(does_set_contain(uniqueEntries, m_emptyLabel));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockStrategy));
  }

  void test_3d_chequred_pattern()
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 3, 3); // Makes a 3 by 3 by 3 grid. All populated with a single value.

    MockBackgroundStrategy mockStrategy;

    /*
     * We treat alternate cells as background, which actually should result in a single object. Think of a chequered flag.
     * */
    EXPECT_CALL(mockStrategy, isBackground(_)).Times(27)
    .WillOnce(Return(true)).WillOnce(Return(false)).WillOnce(Return(true))
    .WillOnce(Return(false)).WillOnce(Return(true)).WillOnce(Return(false))
    .WillOnce(Return(true)).WillOnce(Return(false)).WillOnce(Return(true))
    .WillOnce(Return(false)).WillOnce(Return(true)).WillOnce(Return(false))
    .WillOnce(Return(true)).WillOnce(Return(false)).WillOnce(Return(true))
    .WillOnce(Return(false)).WillOnce(Return(true)).WillOnce(Return(false))
    .WillOnce(Return(true)).WillOnce(Return(false)).WillOnce(Return(true))
    .WillOnce(Return(false)).WillOnce(Return(true)).WillOnce(Return(false))
    .WillOnce(Return(true)).WillOnce(Return(false)).WillOnce(Return(true));

    size_t labelingId = 1;
    bool multiThreaded = false;
    ConnectedComponentLabeling ccl(labelingId, multiThreaded);
    Progress prog;
    auto outWS = ccl.execute(inWS, &mockStrategy, prog);

    std::set<size_t> uniqueEntries = connection_workspace_to_set_of_labels(outWS.get());
    TSM_ASSERT_EQUALS("Just one object, but we have some 'empty' entries too", 2, uniqueEntries.size());
    TS_ASSERT(does_set_contain(uniqueEntries, labelingId));
    TS_ASSERT(does_set_contain(uniqueEntries, m_emptyLabel));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockStrategy));
  }

  void do_test_cluster_labeling(const std::vector<size_t>& clusterIndexes, IMDHistoWorkspace const * const ws, const size_t& expectedLabel)
  {
    std::set<double> valuesInCluster;
    for(size_t i = 0; i < ws->getNPoints(); ++i)
    {
      if(does_vector_contain(clusterIndexes, i))
      {
        valuesInCluster.insert(ws->getSignalAt(i));
      }
    }
    TSM_ASSERT_EQUALS("Labels within a cluster should be unique", 1, valuesInCluster.size());
    TSM_ASSERT("Label of cluster is not what was expected", does_set_contain(valuesInCluster, double(expectedLabel)));
  }

  void test_3d_with_many_objects()
  {
    // ------------- Setup

    const double raisedSignal = 1;
    const double backgroundSignal = 0;
    // Create an array initialized to background for a n by n by n grid.
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(backgroundSignal, 3, 5);// 5*5*5

    // Now add some objects
    // First cluster amongst 3 dimensions.
    std::vector<size_t> clusterOneIndexes = boost::assign::list_of(1)(1+1)(1+5-1);

    // Another cluster amongst 3 dimensions. Rough center of block.
    std::vector<size_t> clusterTwoIndexes = boost::assign::list_of(5*5*2)((5*5*2)+1)((5*5*2)+5);

    // Another cluster amongst 3 dimensions. Far side of block.
    std::vector<size_t> clusterThreeIndexes = boost::assign::list_of((5*5*5)-1)((5*5*5)-2)((5*5*5)-(5*5)-1);

    // Accumulate all cluster indexes
    std::vector<size_t> allClusterIndexes;
    allClusterIndexes.insert(allClusterIndexes.end(), clusterOneIndexes.begin(), clusterOneIndexes.end());
    allClusterIndexes.insert(allClusterIndexes.end(), clusterTwoIndexes.begin(), clusterTwoIndexes.end());
    allClusterIndexes.insert(allClusterIndexes.end(), clusterThreeIndexes.begin(), clusterThreeIndexes.end());

    // Add elevated signal to the workspace at cluster indexes.
    for(auto it = allClusterIndexes.begin(); it != allClusterIndexes.end(); ++it)
    {
      inWS->setSignalAt(*it, raisedSignal);
    }

    // ---------- Run the cluster finding
    HardThresholdBackground strategy(backgroundSignal, NoNormalization);

    size_t labelingId = 1;
    bool multiThreaded = false;
    ConnectedComponentLabeling ccl(labelingId, multiThreaded);
    Progress prog;
    auto outWS = ccl.execute(inWS, &strategy, prog);

    // ----------- Basic cluster checks

    std::set<size_t> uniqueEntries = connection_workspace_to_set_of_labels(outWS.get());
    TSM_ASSERT_EQUALS("Should have 3 clusters, but we have some 'empty' entries too", 4, uniqueEntries.size());
    TS_ASSERT(does_set_contain(uniqueEntries, labelingId));
    TS_ASSERT(does_set_contain(uniqueEntries, labelingId+1));
    TS_ASSERT(does_set_contain(uniqueEntries, labelingId+2));
    TS_ASSERT(does_set_contain(uniqueEntries, m_emptyLabel));

    // ------------ Detailed cluster checks

    // First-off. All indexes in allClusterIndexes should be represented as non-background in the output workspace.
    for(size_t i = 0; i < outWS->getNPoints(); ++i)
    {
      if(does_vector_contain(allClusterIndexes, i))
      {
        TSM_ASSERT("Should be labeled", outWS->getSignalAt(i) >= labelingId) // Background is marked as -1.
      }
      else
      {
        TSM_ASSERT_EQUALS("Should not be labeled", outWS->getSignalAt(i), m_emptyLabel);
      }
    }
    do_test_cluster_labeling(clusterOneIndexes, outWS.get(), labelingId);
    do_test_cluster_labeling(clusterTwoIndexes, outWS.get(), labelingId+1);
    do_test_cluster_labeling(clusterThreeIndexes, outWS.get(), labelingId+2);
  }

};

//=====================================================================================
// Performance Tests
//=====================================================================================
class ConnectedComponentLabelingTestPerformance: public CxxTest::TestSuite
{
private:

  IMDHistoWorkspace_sptr m_inWS;
  const double m_backgroundSignal;
  boost::scoped_ptr<BackgroundStrategy> m_backgroundStrategy;
  
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConnectedComponentLabelingTestPerformance *createSuite()
  {
    return new ConnectedComponentLabelingTestPerformance();
  }
  static void destroySuite(ConnectedComponentLabelingTestPerformance *suite)
  {
    delete suite;
  }


  ConnectedComponentLabelingTestPerformance() : m_backgroundSignal(0), m_backgroundStrategy(new HardThresholdBackground(0, NoNormalization))
  {
    FrameworkManager::Instance();

    const double raisedSignal = 1;
    // Create an array initialized to background for a n by n by n grid.
    m_inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(m_backgroundSignal, 2,
        1000); // 1000 by 1000 grid

    // All cluster indexes
    std::vector<size_t> allClusterIndexes(m_inWS->getNPoints(), 0);

    size_t count = 0;
    for (auto it = allClusterIndexes.begin(); it != allClusterIndexes.end(); ++it, ++count)
    {
      if (count % 2 == 0)
      {
        m_inWS->setSignalAt(*it, raisedSignal);
      }
    }

  }

  void testPerformance()
  {
    ConnectedComponentLabeling ccl;
    size_t labelingId = 1;
    ccl.startLabelingId(labelingId);
    Progress prog;
    auto outWS = ccl.execute(m_inWS, m_backgroundStrategy.get(), prog);

    // ----------- Basic cluster checks

    std::set<size_t> uniqueEntries = connection_workspace_to_set_of_labels(outWS.get());
    TSM_ASSERT_EQUALS("Should be chequered pattern", 2, uniqueEntries.size());
    TS_ASSERT(does_set_contain(uniqueEntries, size_t(0)));
    TS_ASSERT(does_set_contain(uniqueEntries, size_t(1)));
  }

};

#endif /* MANTID_CRYSTAL_CONNECTEDCOMPONENTLABELINGTEST_H_ */
