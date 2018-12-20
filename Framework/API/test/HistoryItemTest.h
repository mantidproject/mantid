#ifndef HISTORYITEMTEST_H_
#define HISTORYITEMTEST_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/HistoryView.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;

class HistoryItemTest : public CxxTest::TestSuite {

public:
  void test_Minimum() {
    // not really much to test
    AlgorithmHistory algHist("AnAlg", 1);
    HistoryItem item(boost::make_shared<AlgorithmHistory>(algHist));
    item.unrolled(true);

    TS_ASSERT_EQUALS(*(item.getAlgorithmHistory()), algHist)
    TS_ASSERT(item.isUnrolled())
    TS_ASSERT_EQUALS(item.numberOfChildren(), 0)
  }
};

#endif
