#ifndef MUTEXTEST_H
#define MUTEXTEST_H

#include <cxxtest/TestSuite.h>
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/FunctionTask.h"
#include <iostream>
#include "MantidKernel/CPUTimer.h"

using namespace Mantid::Kernel;

#define DATA_SIZE 10000000
std::vector<double> shared_data;


//#include <boost/thread.hpp>
//boost::shared_mutex _access;
//void reader()
//{
//  boost::shared_lock< boost::shared_mutex > lock(_access);
//  // do work here, without anyone having exclusive access
//  for (size_t i=0; i<shared_data.size(); i++)
//  {
//    double val = shared_data[i];
//  }
//}
//
//void conditional_writer()
//{
//  boost::upgrade_lock< boost::shared_mutex > lock(_access);
//  // do work here, without anyone having exclusive access
//
//  if (true)
//  {
//    boost::upgrade_to_unique_lock< boost::shared_mutex > uniqueLock(lock);
//    // do work here, but now you have exclusive access
//    for (size_t i=0; i<shared_data.size(); i++)
//    {
//      shared_data[i] = 2.345;
//    }
//  }
//
//  // do more work here, without anyone having exclusive access
//}
//
//void unconditional_writer()
//{
//  boost::unique_lock< boost::shared_mutex > lock( _access );
//  // do work here, with exclusive access
//  shared_data.resize(shared_data.size()+1, 2.345);
//  // Dumb thing to slow down the writer
//  for (size_t i=0; i<shared_data.size(); i++)
//    shared_data[i] = 4.567;
//}


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

//  /** Launch a bunch of reading threads */
//  void test_simultaneous_read()
//  {
//    ThreadPool pool;
//    CPUTimer tim;
//    size_t numTasks = 100;
//    for (size_t i=0; i<numTasks; i++)
//      pool.schedule( new FunctionTask(reader) );
//    pool.joinAll();
//    std::cout << tim << " to execute all " << numTasks << " tasks" << std::endl;
//  }
//
//  /** Launch a bunch of writing threads */
//  void test_simultaneous_write()
//  {
//    ThreadPool pool;
//    CPUTimer tim;
//    size_t numTasks = 10;
//    for (size_t i=0; i<numTasks; i++)
//      pool.schedule( new FunctionTask(unconditional_writer) );
//    pool.joinAll();
//    std::cout << tim << " to execute all " << numTasks << " tasks" << std::endl;
//    TSM_ASSERT_EQUALS( "The writers were all called", shared_data.size(), DATA_SIZE + numTasks)
//  }
//
//  /** Mix 1 writing thread for 9 reading threads */
//  void test_write_blocks_readers()
//  {
//    ThreadPool pool;
//    CPUTimer tim;
//    size_t numTasks = 100;
//    for (size_t i=0; i<numTasks; i++)
//    {
//      if (i%10 == 0)
//        pool.schedule( new FunctionTask(unconditional_writer) );
//      else
//        pool.schedule( new FunctionTask(reader) );
//    }
//    pool.joinAll();
//    std::cout << tim << " to execute all " << numTasks << " tasks" << std::endl;
//    TSM_ASSERT_EQUALS( "The writers were all called", shared_data.size(), DATA_SIZE + numTasks/10)
//  }

};


#endif /* MUTEXTEST_H */


