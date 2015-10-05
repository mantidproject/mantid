#ifndef CACHETEST_H_
#define CACHETEST_H_

#include <cxxtest/TestSuite.h>

// The test requires the hit/miss stats. Note that this only works because the
// Cache class
// is header only
#define USE_CACHE_STATS
#include "MantidKernel/Cache.h"

using namespace Mantid::Kernel;

class CacheTest : public CxxTest::TestSuite {
public:
  void testConstructor() {
    Cache<bool, double> c;
    // Test that all the base class member variables are correctly assigned to
    TS_ASSERT_EQUALS(c.size(), 0);
    TS_ASSERT_EQUALS(c.hitCount(), 0);
    TS_ASSERT_EQUALS(c.missCount(), 0);
    TS_ASSERT_EQUALS(c.hitRatio(), 0);
  }

  void testSetCache() {
    Cache<int, int> c;
    c.setCache(1, 1);
    TS_ASSERT_EQUALS(c.size(), 1);
    TS_ASSERT_EQUALS(c.hitCount(), 0);
    TS_ASSERT_EQUALS(c.missCount(), 0);
    TS_ASSERT_EQUALS(c.hitRatio(), 0);
  }

  void testSetCacheOverwrite() {
    Cache<int, int> c;
    c.setCache(1, 1);
    TS_ASSERT_EQUALS(c.size(), 1);
    TS_ASSERT_EQUALS(c.hitCount(), 0);
    TS_ASSERT_EQUALS(c.missCount(), 0);
    TS_ASSERT_EQUALS(c.hitRatio(), 0);

    // overwrite
    c.setCache(1, 1);
    TS_ASSERT_EQUALS(c.size(), 1);
    TS_ASSERT_EQUALS(c.hitCount(), 0);
    TS_ASSERT_EQUALS(c.missCount(), 0);
    TS_ASSERT_EQUALS(c.hitRatio(), 0);
  }

  void testClear() {
    Cache<int, int> c;
    c.setCache(1, 1);
    c.setCache(2, 1);
    TS_ASSERT_EQUALS(c.size(), 2);
    c.clear();
    TS_ASSERT_EQUALS(c.size(), 0);
    TS_ASSERT_EQUALS(c.hitCount(), 0);
    TS_ASSERT_EQUALS(c.missCount(), 0);
    TS_ASSERT_EQUALS(c.hitRatio(), 0);
  }

  void testgetCache() {
    // set up cache
    Cache<int, int> c;
    c.setCache(1, 1);
    c.setCache(2, 2);
    c.setCache(3, 3);
    c.setCache(4, 4);
    TS_ASSERT_EQUALS(c.size(), 4);
    TS_ASSERT_EQUALS(c.hitCount(), 0);
    TS_ASSERT_EQUALS(c.missCount(), 0);
    TS_ASSERT_EQUALS(c.hitRatio(), 0);

    // test hit
    int value = 0;
    TS_ASSERT_EQUALS(c.getCache(1, value), true);
    TS_ASSERT_EQUALS(value, 1);
    TS_ASSERT_EQUALS(c.hitCount(), 1);
    TS_ASSERT_EQUALS(c.missCount(), 0);
    TS_ASSERT_DELTA(c.hitRatio(), 100, 1e-6);

    // test miss
    TS_ASSERT_EQUALS(c.getCache(5, value), false);
    TS_ASSERT_EQUALS(value, 1);
    TS_ASSERT_EQUALS(c.hitCount(), 1);
    TS_ASSERT_EQUALS(c.missCount(), 1);
    TS_ASSERT_DELTA(c.hitRatio(), 50, 1e-6);

    // test hit
    TS_ASSERT_EQUALS(c.getCache(4, value), true);
    TS_ASSERT_EQUALS(value, 4);
    TS_ASSERT_EQUALS(c.hitCount(), 2);
    TS_ASSERT_EQUALS(c.missCount(), 1);
    TS_ASSERT_DELTA(c.hitRatio(), 66.666666, 1e-6);
  }
};

// Remove the define here
#undef USE_CACHE_STATS

class CacheTestPerformance : public CxxTest::TestSuite {
public:
  void setUp() {
    m_nelements = 250000;
    for (int i = 0; i < m_nelements; ++i) {
      m_cacheGet.setCache(i, 1.5);
    }
  }

  void test_get_performance() {
    double value(0.0);
    for (int i = 0; i < m_nelements; ++i) {
      m_cacheGet.getCache(i, value);
    }
  }

  void test_set_performance() {
    for (int i = 0; i < m_nelements; ++i) {
      m_cacheSet.setCache(i, 1.5);
    }
  }

  int m_nelements;
  Cache<int, double> m_cacheGet;
  Cache<int, double> m_cacheSet;
};

#endif /*CACHETEST_H_*/
