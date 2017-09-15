#ifndef TIMESERIESPROPERTYTEST_H_
#define TIMESERIESPROPERTYTEST_H_

#include "MantidKernel/TimeInterval.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;

class TimeIntervalTest : public CxxTest::TestSuite {
public:
  void testConstructor() { TS_ASSERT_THROWS_NOTHING(TimeInterval()); }
};

#endif /*TIMESERIESPROPERTYTEST_H_*/
