#ifndef CACHETEST_H_
#define CACHETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Cache.h"

using namespace Mantid::Kernel;

class CacheTest : public CxxTest::TestSuite
{
public:

  void testConstructor()
  {
    Cache<bool,double> c;
    // Test that all the base class member variables are correctly assigned to
    TS_ASSERT_EQUALS(c.size(), 0);
    TS_ASSERT_EQUALS(c.hitCount(), 0);
    TS_ASSERT_EQUALS(c.missCount(), 0);
    TS_ASSERT_EQUALS(c.hitRatio(), 0);
  }

  void testSetCache()
  {
    Cache<int,int> c;
    c.setCache(1,1);
    TS_ASSERT_EQUALS(c.size(), 1);
    TS_ASSERT_EQUALS(c.hitCount(), 0);
    TS_ASSERT_EQUALS(c.missCount(), 0);
    TS_ASSERT_EQUALS(c.hitRatio(), 0);
  }

  void testSetCacheOverwrite()
  {
    Cache<int,int> c;
    c.setCache(1,1);
    TS_ASSERT_EQUALS(c.size(), 1);
    TS_ASSERT_EQUALS(c.hitCount(), 0);
    TS_ASSERT_EQUALS(c.missCount(), 0);
    TS_ASSERT_EQUALS(c.hitRatio(), 0);

    //overwrite
    c.setCache(1,1);
    TS_ASSERT_EQUALS(c.size(), 1);
    TS_ASSERT_EQUALS(c.hitCount(), 0);
    TS_ASSERT_EQUALS(c.missCount(), 0);
    TS_ASSERT_EQUALS(c.hitRatio(), 0);
  }

  void testClear()
  {
    Cache<int,int> c;
    c.setCache(1,1);
    c.setCache(2,1);
    TS_ASSERT_EQUALS(c.size(), 2);
    c.clear();
    TS_ASSERT_EQUALS(c.size(), 0);
    TS_ASSERT_EQUALS(c.hitCount(), 0);
    TS_ASSERT_EQUALS(c.missCount(), 0);
    TS_ASSERT_EQUALS(c.hitRatio(), 0);
  }

  void testgetCache()
  {
    //set up cache
    Cache<int,int> c;
    c.setCache(1,1);
    c.setCache(2,2);
    c.setCache(3,3);
    c.setCache(4,4);
    TS_ASSERT_EQUALS(c.size(), 4);
    TS_ASSERT_EQUALS(c.hitCount(), 0);
    TS_ASSERT_EQUALS(c.missCount(), 0);
    TS_ASSERT_EQUALS(c.hitRatio(), 0);

    //test hit
    int value;
    TS_ASSERT_EQUALS(c.getCache(1,value), true);
    TS_ASSERT_EQUALS(value, 1);
    TS_ASSERT_EQUALS(c.hitCount(), 1);
    TS_ASSERT_EQUALS(c.missCount(), 0);
    TS_ASSERT_DELTA(c.hitRatio(), 100, 1e-6);

    //test miss
    TS_ASSERT_EQUALS(c.getCache(5,value), false);
    TS_ASSERT_EQUALS(value, 1);
    TS_ASSERT_EQUALS(c.hitCount(), 1);
    TS_ASSERT_EQUALS(c.missCount(), 1);
    TS_ASSERT_DELTA(c.hitRatio(), 50, 1e-6);

    //test hit
    TS_ASSERT_EQUALS(c.getCache(4,value), true);
    TS_ASSERT_EQUALS(value, 4);
    TS_ASSERT_EQUALS(c.hitCount(), 2);
    TS_ASSERT_EQUALS(c.missCount(), 1);
    TS_ASSERT_DELTA(c.hitRatio(), 66.666666, 1e-6);
  }

};

#endif /*CACHETEST_H_*/
