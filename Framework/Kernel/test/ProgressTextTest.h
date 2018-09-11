#ifndef MANTID_KERNEL_PROGRESSTEXTTEST_H_
#define MANTID_KERNEL_PROGRESSTEXTTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cstdlib>
#include <cxxtest/TestSuite.h>

#include "MantidKernel/ProgressText.h"

using namespace Mantid::Kernel;

class ProgressTextTest : public CxxTest::TestSuite {
public:
  void test_setNumSteps() {
    ProgressText p(0.5, 1.0, 10);
    TS_ASSERT_THROWS_NOTHING(p.setNumSteps(100));
  }

  void test_constructors() {
    // No steps?
    TS_ASSERT_THROWS_NOTHING(ProgressText(0.0, 1.0, 0););
    // Max is < min
    TS_ASSERT_THROWS_NOTHING(ProgressText(0.0, 1.0, 2, true););
  }

  void test_with_stdout() {
    ProgressText p(0.5, 1.0, 10);
    // 4 outputs
    p.report();
    p.report("I have an optional message");
    p.report();
    p.report();

    p.setNumSteps(100);
    // These should output only 2 lines. The % will go backwards though
    p.report();
    p.report();
    p.report();
    p.report();

    p.setNumSteps(5);
    p.report();
  }

  void test_on_one_line() {
    ProgressText p(0.0, 1.0, 100, false);
    for (int i = 0; i < 100; i++) {
      std::string msg = "";
      for (int i = 0; i < std::rand() % 10; i++)
        msg += "bla";
      p.report(msg);
    }
  }
};

#endif
