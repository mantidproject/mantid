#ifndef MUTEXTEST_H
#define MUTEXTEST_H

#include <cxxtest/TestSuite.h>
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/FunctionTask.h"
#include <iostream>
#include "MantidKernel/CPUTimer.h"
#include <Poco/RWLock.h>
using namespace Mantid::Kernel;

#define DATA_SIZE 10000000
std::vector<double> shared_data;


Poco::RWLock _access;
void reader()
{
//  std::cout << "Read started" << std::endl;
  Poco::ScopedReadRWLock lock(_access);
//  std::cout << "Read launching" << std::endl;
  // do work here, without anyone having exclusive access
  for (size_t i=0; i<shared_data.size(); i++)
  {
    double val = shared_data[i];
  }
//  std::cout << "Read finished" << std::endl;
}


void unconditional_writer()
{
//  std::cout << "Write started" << std::endl;
  Poco::ScopedWriteRWLock lock( _access );
//  std::cout << "Write launching" << std::endl;
  // do work here, with exclusive access
  shared_data.resize(shared_data.size()+1, 2.345);
  // Dumb thing to slow down the writer
  for (size_t i=0; i<shared_data.size(); i++)
    shared_data[i] = 4.567;
//  std::cout << "Write finished" << std::endl;
}


class MutexTest : public CxxTest::TestSuite
{
public:

  void setUp()
  {
    shared_data.resize(DATA_SIZE, 1.000);
  }

  void tearDown()
  {
  }

  void test_nothing()
  {
  }

  /** Launch a bunch of reading threads */
  void test_simultaneous_read()
  {
    ThreadPool pool;
    CPUTimer tim;
    size_t numTasks = 50;
    for (size_t i=0; i<numTasks; i++)
      pool.schedule( new FunctionTask(reader) );
    pool.joinAll();
    std::cout << tim << " to execute all " << numTasks << " tasks" << std::endl;
  }

  /** Launch a bunch of writing threads */
  void test_simultaneous_write()
  {
    ThreadPool pool;
    CPUTimer tim;
    size_t numTasks = 10;
    for (size_t i=0; i<numTasks; i++)
      pool.schedule( new FunctionTask(unconditional_writer) );
    pool.joinAll();
    std::cout << tim << " to execute all " << numTasks << " tasks" << std::endl;
    TSM_ASSERT_EQUALS( "The writers were all called", shared_data.size(), DATA_SIZE + numTasks)
  }

  /** Mix 1 writing thread for 9 reading threads */
  void test_write_blocks_readers()
  {
    ThreadPool pool;
    CPUTimer tim;
    size_t numTasks = 50;
    for (size_t i=0; i<numTasks; i++)
    {
      if (i%10 == 0)
        pool.schedule( new FunctionTask(unconditional_writer) );
      else
        pool.schedule( new FunctionTask(reader) );
    }
    pool.joinAll();
    std::cout << tim << " to execute all " << numTasks << " tasks" << std::endl;
    TSM_ASSERT_EQUALS( "The writers were all called", shared_data.size(), DATA_SIZE + numTasks/10)
  }

};


#endif /* MUTEXTEST_H */


