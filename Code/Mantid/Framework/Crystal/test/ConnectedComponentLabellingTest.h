#ifndef MANTID_CRYSTAL_CONNECTEDCOMPONENTLABELLINGTEST_H_
#define MANTID_CRYSTAL_CONNECTEDCOMPONENTLABELLINGTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <set>
#include <algorithm>
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidCrystal/ConnectedComponentLabelling.h"
#include "MantidCrystal/BackgroundStrategy.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::MDEvents;
using namespace testing;

class ConnectedComponentLabellingTest: public CxxTest::TestSuite
{
private:

  // Mock Background strategy
  class MockBackgroundStrategy: public BackgroundStrategy
  {
  public:
    MOCK_CONST_METHOD1(configureIterator, void(Mantid::API::IMDIterator* const));
    MOCK_CONST_METHOD1(isBackground, bool(Mantid::API::IMDIterator* const));
    virtual ~MockBackgroundStrategy()
    {}
  };

  // Helper function for determining if a set contains a specific value.
  bool does_set_contain(const std::set<size_t>& container, const size_t& value)
  {
    return std::find(container.begin(), container.end(), value) != container.end();
  }

  // Helper function for converting a IMDHistoWorkspace of labels into a set of unique labels.
  std::set<size_t> connection_workspace_to_set_of_labels(IMDHistoWorkspace const * const ws)
  {
    std::set<size_t> unique_values;
    for(size_t i = 0; i < ws->getNPoints(); ++i)
    {
      unique_values.insert(static_cast<size_t>(ws->getSignalAt(i)));
    }
    return unique_values;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConnectedComponentLabellingTest *createSuite()
  { return new ConnectedComponentLabellingTest();}
  static void destroySuite( ConnectedComponentLabellingTest *suite )
  { delete suite;}

  ConnectedComponentLabellingTest()
  {
    FrameworkManager::Instance();
  }

  void test_single_1d_blob()
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1, 10); // Makes a 1 by 10 md ws with identical signal values.

    MockBackgroundStrategy mockStrategy;
    EXPECT_CALL(mockStrategy, isBackground(_)).Times(inWS->getNPoints()).WillRepeatedly(Return(false));// A filter that passes everything.

    ConnectedComponentLabelling ccl;
    size_t labellingId = 2;
    ccl.startLabellingId(labellingId);
    auto outWS = ccl.execute(inWS, &mockStrategy);

    /*
     * Because all the signal values are identical, and none are below any threshold. We assume that there will only be a single component. All
     * signal values in the output workspace should bear the first component identifier label. i.e one big blob.
     */
    auto uniqueValues = connection_workspace_to_set_of_labels(outWS.get());
    TS_ASSERT_EQUALS(1, uniqueValues.size());
    TS_ASSERT(does_set_contain(uniqueValues, labellingId));

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockStrategy));
  }

  void test_double_1d_blob()
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1, 6); // Makes a 1 by 5 md ws with identical signal values.

    MockBackgroundStrategy mockStrategy;
    EXPECT_CALL(mockStrategy, isBackground(_))
    .WillOnce(Return(false))
    .WillOnce(Return(false))
    .WillOnce(Return(true))
    .WillRepeatedly(Return(false));

    ConnectedComponentLabelling ccl;
    size_t labellingId = 0;
    ccl.startLabellingId(labellingId);
    auto outWS = ccl.execute(inWS, &mockStrategy);

    /*
     * Because all the signal values are identical, and none are below any threshold. We assume that there will only be a single component. All
     * signal values in the output workspace should bear the first component identifier label. i.e one big blob.
     */
    std::set<size_t> uniqueEntries = connection_workspace_to_set_of_labels(outWS.get());
    TSM_ASSERT_EQUALS("2 blobs so should have 3 unique entries", 3, uniqueEntries.size());
    TS_ASSERT(does_set_contain(uniqueEntries, labellingId));
    TS_ASSERT(does_set_contain(uniqueEntries, -1)); // Background entries.
    TS_ASSERT(does_set_contain(uniqueEntries, labellingId+1));

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockStrategy));
  }

};

#endif /* MANTID_CRYSTAL_CONNECTEDCOMPONENTLABELLINGTEST_H_ */
