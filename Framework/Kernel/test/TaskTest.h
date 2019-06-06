// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef TASKTEST_H_
#define TASKTEST_H_

#include "MantidKernel/Task.h"
#include "MantidKernel/Timer.h"
#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;

namespace TaskTestNamespace {
int my_check_value = 0;
}

/** A custom implementation of Task */
class MyTask : public Task {
public:
  void run() override { TaskTestNamespace::my_check_value = 123; }
};

class TaskTest : public CxxTest::TestSuite {
public:
  void test_run() {
    MyTask t;
    TaskTestNamespace::my_check_value = 0;
    TS_ASSERT_DIFFERS(TaskTestNamespace::my_check_value, 123);
    t.run();
    TS_ASSERT_EQUALS(TaskTestNamespace::my_check_value, 123);
  }

  void test_mutex() {
    MyTask t;
    auto mut = boost::make_shared<std::mutex>();
    t.setMutex(mut);
    TS_ASSERT_EQUALS(mut, t.getMutex());
  }
};

#endif
