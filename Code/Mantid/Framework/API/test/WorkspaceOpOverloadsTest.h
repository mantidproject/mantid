#ifndef WORKSPACEOPOVERLOADSTEST_H_
#define WORKSPACEOPOVERLOADSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid::API;

class WorkspaceOpOverloadsTest : public CxxTest::TestSuite
{
public:
  //----------------------------------------------------------------------
  // WorkspaceHelpers tests
  //----------------------------------------------------------------------

  void test_commmonBoundaries_negative_sum() // Added in response to bug #7391
  {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->init(2,2,1);
    ws->getSpectrum(0)->dataX()[0] = -2.0;
    ws->getSpectrum(0)->dataX()[1] = -1.0;
    ws->getSpectrum(1)->dataX()[0] = -2.5;
    ws->getSpectrum(1)->dataX()[1] = -1.5;

    TS_ASSERT( ! WorkspaceHelpers::commonBoundaries(ws) );
  }

  void test_matchingBins_negative_sum() // Added in response to bug #7391
  {
    auto ws1 = boost::make_shared<WorkspaceTester>();
    ws1->init(2,2,1);
    ws1->getSpectrum(1)->dataX()[0] = -2.5;
    ws1->getSpectrum(1)->dataX()[1] = -1.5;

    auto ws2 = boost::make_shared<WorkspaceTester>();
    ws2->init(2,2,1);
    ws2->getSpectrum(1)->dataX()[0] = -2.7;
    ws2->getSpectrum(1)->dataX()[1] = -1.7;

    TS_ASSERT( WorkspaceHelpers::matchingBins(ws1,ws2,true) );
    TS_ASSERT( ! WorkspaceHelpers::matchingBins(ws1,ws2) );

    ws1->getSpectrum(0)->dataX()[0] = -2.0;
    ws1->getSpectrum(0)->dataX()[1] = -1.0;
    ws2->getSpectrum(0)->dataX()[0] = -3.0;
    ws2->getSpectrum(0)->dataX()[1] = -4.0;

    TS_ASSERT( ! WorkspaceHelpers::matchingBins(ws1,ws2,true) );
  }
};

#endif
