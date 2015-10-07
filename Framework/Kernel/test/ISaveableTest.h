#ifndef MANTID_MDEVENTS_ISAVEABLETEST_H_
#define MANTID_MDEVENTS_ISAVEABLETEST_H_

#include "MantidKernel/ISaveable.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;

class ISaveableTest : public CxxTest::TestSuite {
public:
  void test_Something() {
    // Bare interface, so nothing to test.
  }
};

#endif /* MANTID_MDEVENTS_ISAVEABLETEST_H_ */
