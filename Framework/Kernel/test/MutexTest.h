// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MUTEXTEST_H
#define MUTEXTEST_H

#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/FunctionTask.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/ThreadPool.h"
#include <Poco/RWLock.h>
#include <cxxtest/TestSuite.h>
using namespace Mantid::Kernel;

#define DATA_SIZE 10000000
std::vector<double> shared_data;

Poco::RWLock _access;

// Poco::ScopedReadRWLock getReadLock()
//{
//  return Poco::ScopedReadRWLock(_access);
//}

void reader() {
  //  std::cout << "Read started\n";
  Poco::ScopedReadRWLock lock(_access);
  //  std::cout << "Read launching\n";
  // do work here, without anyone having exclusive access
  for (double val : shared_data) {
    UNUSED_ARG(val)
  }
  //  std::cout << "Read finished\n";
}

void unconditional_writer() {
  //  std::cout << "Write started\n";
  Poco::ScopedWriteRWLock lock(_access);
  //  std::cout << "Write launching\n";
  // do work here, with exclusive access
  shared_data.resize(shared_data.size() + 1, 2.345);
  // Dumb thing to slow down the writer
  for (double &i : shared_data)
    i = 4.567;
  //  std::cout << "Write finished\n";
}

class MutexTest : public CxxTest::TestSuite {
public:
  void setUp() override { shared_data.resize(DATA_SIZE, 1.000); }

  void tearDown() override {}

  void test_nothing() {}

  /** Launch a bunch of reading threads */
  void test_simultaneous_read() {
    ThreadPool pool;
    CPUTimer tim;
    size_t numTasks = 50;
    for (size_t i = 0; i < numTasks; i++)
      pool.schedule(std::make_shared<FunctionTask>(reader));
    pool.joinAll();
    std::cout << tim << " to execute all " << numTasks << " tasks\n";
  }

  /** Launch a bunch of writing threads */
  void test_simultaneous_write() {
    ThreadPool pool;
    CPUTimer tim;
    size_t numTasks = 10;
    for (size_t i = 0; i < numTasks; i++)
      pool.schedule(std::make_shared<FunctionTask>(unconditional_writer));
    pool.joinAll();
    std::cout << tim << " to execute all " << numTasks << " tasks\n";
    TSM_ASSERT_EQUALS("The writers were all called", shared_data.size(),
                      DATA_SIZE + numTasks)
  }

  /** Mix 1 writing thread for 9 reading threads */
  void test_write_blocks_readers() {
    ThreadPool pool;
    CPUTimer tim;
    size_t numTasks = 50;
    for (size_t i = 0; i < numTasks; i++) {
      if (i % 10 == 0)
        pool.schedule(std::make_shared<FunctionTask>(unconditional_writer));
      else
        pool.schedule(std::make_shared<FunctionTask>(reader));
    }
    pool.joinAll();
    std::cout << tim << " to execute all " << numTasks << " tasks\n";
    TSM_ASSERT_EQUALS("The writers were all called", shared_data.size(),
                      DATA_SIZE + numTasks / 10)
  }
};

#endif /* MUTEXTEST_H */
