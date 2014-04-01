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
    auto outWS = ccl.execute(inWS, &mockStrategy);

    auto uniqueValues = connection_workspace_to_set_of_labels(outWS.get());
    TS_ASSERT_EQUALS(1, uniqueValues.size());
    TS_ASSERT(does_set_contain(uniqueValues, labelingId));

    for(auto it = uniqueValues.begin(); it != uniqueValues.end(); ++it)
    {
      std::cout << "Value: " << *it << std::endl;
    }

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockStrategy));
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
    auto outWS = ccl.execute(m_inWS, m_backgroundStrategy.get());

    // ----------- Basic cluster checks

    std::set<size_t> uniqueEntries = connection_workspace_to_set_of_labels(outWS.get());
    TSM_ASSERT_EQUALS("Should be chequered pattern", 2, uniqueEntries.size());
    TS_ASSERT(does_set_contain(uniqueEntries, size_t(0)));
    TS_ASSERT(does_set_contain(uniqueEntries, size_t(1)));
  }

};

#endif /* MANTID_CRYSTAL_CONNECTEDCOMPONENTLABELINGTEST_H_ */
