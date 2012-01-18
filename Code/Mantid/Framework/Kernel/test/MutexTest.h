#ifndef MUTEXTEST_H
#define MUTEXTEST_H

#include <cxxtest/TestSuite.h>
#include "MantidKernel/MultiThreaded.h"
#include <boost/thread.hpp>

using namespace Mantid::Kernel;

boost::shared_mutex _access;
void reader()
{
  boost::shared_lock< boost::shared_mutex > lock(_access);
  // do work here, without anyone having exclusive access
}

void conditional_writer()
{
  boost::upgrade_lock< boost::shared_mutex > lock(_access);
  // do work here, without anyone having exclusive access

  if (true)
  {
    boost::upgrade_to_unique_lock< boost::shared_mutex > uniqueLock(lock);
    // do work here, but now you have exclusive access
  }

  // do more work here, without anyone having exclusive access
}

void unconditional_writer()
{
  boost::unique_lock< boost::shared_mutex > lock( _access );
  // do work here, with exclusive access
}


class MutexTest : public CxxTest::TestSuite
{
public:

  void test_1()
  {

  }

};


#endif /* MUTEXTEST_H */


