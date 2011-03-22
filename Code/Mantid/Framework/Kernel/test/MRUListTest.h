#ifndef MRULISTTEST_H_
#define MRULISTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/MRUList.h"

using namespace Mantid::Kernel;


/** Dummy class needed by MRUList */
class MyTestClass
{
public:
  int hash;
  int value;

  /// Constructor
  MyTestClass(int hash, int value) : hash(hash), value(value)
  {
  }

  // Function returning the has index value
  int hashIndexFunction() const
  {
    return hash;
  }

};


class MRUListTest : public CxxTest::TestSuite
{
public:

  void testMRU_Everything()
  {
    // MRUList with 3 spots
    MRUList<MyTestClass> m(3);
    TS_ASSERT_EQUALS( m.size(), 0 );
    m.insert( new MyTestClass(10, 20) );
    TS_ASSERT_EQUALS( m.size(), 1 );

    // Retrieve an element
    TS_ASSERT( m.find(10) );
    TS_ASSERT_EQUALS( m.find(10)->value, 20 );

    m.insert( new MyTestClass(20, 40) );
    TS_ASSERT_EQUALS( m.size(), 2 );
    m.insert( new MyTestClass(30, 60) );
    TS_ASSERT_EQUALS( m.size(), 3 );

    // This will drop one from the list
    MyTestClass * being_dropped = m.insert( new MyTestClass(40, 80) );
    TS_ASSERT_EQUALS( m.size(), 3 );
    // # 10 was dropped off; the calling class takes care of whatever this means
    TS_ASSERT_EQUALS( being_dropped->hash, 10 );

    // Can't find the one that dropped off
    TS_ASSERT( !m.find(10) );

    // But we can find the one that was last in line
    TS_ASSERT( m.find(20) );
    // We can add it again, pushing it to the top
    being_dropped = m.insert( new MyTestClass(20, 40) );
    TS_ASSERT_EQUALS( m.size(), 3 );
    // Nothing needs to be dropped.
    TS_ASSERT( !being_dropped);

    // And we can add 2 new ones
    m.insert( new MyTestClass(50, 100) );
    TS_ASSERT_EQUALS( m.size(), 3 );
    m.insert( new MyTestClass(60, 120) );
    TS_ASSERT_EQUALS( m.size(), 3 );

    // And now the ones left are 20 (since it was moved to the top of the MRU list) and the 2 new ones.
    TS_ASSERT( m.find(20) );
    TS_ASSERT( m.find(50) );
    TS_ASSERT( m.find(60) );

    TS_ASSERT_THROWS_NOTHING( m.deleteIndex(50) );
    TS_ASSERT_EQUALS( m.size(), 2 );

    // Test out the clear method.
    m.clear();
    TS_ASSERT_EQUALS( m.size(), 0 );

  }

};


#endif /* MRULISTTEST_H_ */


