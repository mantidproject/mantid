#ifndef BOXPLITCONTROLLER_TEST_H
#define BOXPLITCONTROLLER_TEST_H

#include "MantidKernel/DiskBuffer.h"
#include "MantidAPI/BoxController.h"
#include <cxxtest/TestSuite.h>
#include <map>
#include <memory>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class BoxControllerTest :    public CxxTest::TestSuite
{
public:

  void test_Constructor()
  {
    BoxController sc(2);
    TS_ASSERT_EQUALS( sc.getNDims(), 2);
  }

  void test_willSplit()
  {
    BoxController sc(2);
    sc.setMaxDepth(4);
    sc.setSplitThreshold(10);
    TS_ASSERT( sc.willSplit(100,3) );
    TS_ASSERT( !sc.willSplit(100,4) );
    TS_ASSERT( !sc.willSplit(2,3) );
    TS_ASSERT( !sc.willSplit(100,5) );
  }

  void test_getSplitInto()
  {
    BoxController sc(3);
    sc.setSplitInto(10);
    TS_ASSERT_EQUALS( sc.getNumSplit(), 1000);
    TS_ASSERT_EQUALS( sc.getSplitInto(0), 10);
    TS_ASSERT_EQUALS( sc.getSplitInto(1), 10);
    TS_ASSERT_EQUALS( sc.getSplitInto(2), 10);
    sc.setSplitInto(1,5);
    TS_ASSERT_EQUALS( sc.getNumSplit(), 500);
    TS_ASSERT_EQUALS( sc.getSplitInto(0), 10);
    TS_ASSERT_EQUALS( sc.getSplitInto(1), 5);
    TS_ASSERT_EQUALS( sc.getSplitInto(2), 10);

  }

  void test_maxDepth()
  {
    BoxController sc(3);
    sc.setSplitInto(10);
    sc.setMaxDepth(6);
    TS_ASSERT_EQUALS( sc.getMaxDepth(), 6);
  }

  void test_IDs()
  {
    BoxController sc(3);
    // No IDs given out yet
    TS_ASSERT_EQUALS( sc.getMaxId(), 0 );
    // Start handing some out
    TS_ASSERT_EQUALS( sc.getNextId(), 0 );
    TS_ASSERT_EQUALS( sc.getNextId(), 1 );
    TS_ASSERT_EQUALS( sc.getNextId(), 2 );
    // You've got 3 given.
    TS_ASSERT_EQUALS( sc.getMaxId(), 3 );

    sc.setSplitInto(10);
    sc.setMaxDepth(6);
    TS_ASSERT_EQUALS( sc.getMaxDepth(), 6);
  }

  void test_maxNumBoxes()
  {
    BoxController sc(3);
    sc.setSplitInto(10);
    TS_ASSERT_EQUALS( sc.getNumSplit(), 1000);
    sc.setMaxDepth(6);
    {
      const std::vector<double> & max = sc.getMaxNumMDBoxes();
      TS_ASSERT_DELTA( max[0], 1.0, 1e-2);
      TS_ASSERT_DELTA( max[1], 1e3, 1e-2);
      TS_ASSERT_DELTA( max[2], 1e6, 1e-2);
      TS_ASSERT_DELTA( max[3], 1e9, 1e-2);
    }

    {
      // If you split into a different number, the values get reset too.
      sc.setSplitInto(5);
      TS_ASSERT_EQUALS( sc.getNumSplit(), 125);
      const std::vector<double> & max = sc.getMaxNumMDBoxes();
      TS_ASSERT_DELTA( max[0], 1.0, 1e-2);
      TS_ASSERT_DELTA( max[1], 125.0, 1e-2);
      TS_ASSERT_DELTA( max[2], 125*125.0, 1e-2);
    }
  }



  void doTest_numBoxes(BoxController & bc, size_t expectedNumEntries)
  {
    const std::vector<size_t> & num = bc.getNumMDBoxes();
    TS_ASSERT_EQUALS( num.size(), expectedNumEntries);
    TS_ASSERT_EQUALS( num[0], 1);
    TS_ASSERT_EQUALS( num[1], 0);

    // Average depth is 0 = all at level 0.
    TS_ASSERT_DELTA( bc.getAverageDepth(), 0.0, 1e-5 );

    bc.trackNumBoxes(0);
    TS_ASSERT_EQUALS( num[0], 0);
    TS_ASSERT_EQUALS( num[1], 100);

    // All at depth 1.0
    TS_ASSERT_DELTA( bc.getAverageDepth(), 1.0, 1e-5 );

    bc.trackNumBoxes(1);
    bc.trackNumBoxes(1);
    TS_ASSERT_EQUALS( num[0], 0);
    TS_ASSERT_EQUALS( num[1], 98);
    TS_ASSERT_EQUALS( num[2], 200);

    // Mostly at depth 1.0
    TS_ASSERT_DELTA( bc.getAverageDepth(), 1.02, 1e-5 );
  }

  /* Try setting these values in different orders */
  void test_trackNumBoxes1()
  {
    BoxController bc(2);
    bc.setSplitInto(10);
    bc.setMaxDepth(4);
    doTest_numBoxes(bc, 5);
  }

  /* This used to give wrong values */
  void test_trackNumBoxes2()
  {
    BoxController bc(2);
    bc.setMaxDepth(4);
    bc.setSplitInto(10);
    bc.setMaxDepth(10);
    doTest_numBoxes(bc, 11);
  }

  /// Compare two box controllers and assert each part of them.
  void compareBoxControllers(BoxController & a, BoxController & b)
  {
    TS_ASSERT_EQUALS( a.getNDims(), b.getNDims());
    TS_ASSERT_EQUALS( a.getMaxDepth(), b.getMaxDepth());
    TS_ASSERT_EQUALS( a.getMaxId(), b.getMaxId());
    TS_ASSERT_EQUALS( a.getSplitThreshold(), b.getSplitThreshold());
    TS_ASSERT_EQUALS( a.getNumMDBoxes(), b.getNumMDBoxes());
    TS_ASSERT_EQUALS( a.getNumSplit(), b.getNumSplit());
    TS_ASSERT_EQUALS( a.getMaxNumMDBoxes(), b.getMaxNumMDBoxes());
    for (size_t d=0; d< a.getNDims(); d++)
    {
      TS_ASSERT_EQUALS( a.getSplitInto(d), b.getSplitInto(d));
    }
  }

  /// Generate XML and read it back
  void test_xml()
  {
    BoxController a(2);
    a.setMaxDepth(4);
    a.setSplitInto(10);
    a.setMaxDepth(10);
    a.setMaxId(123456);

    std::string xml = a.toXMLString();
    TS_ASSERT(!xml.empty());

    // Read it back
    BoxController b(1);
    b.fromXMLString(xml);
    // Check that it is the same
    compareBoxControllers(a, b);
  }

  void test_copy_constructor()
  {
    BoxController a(2);
    a.setMaxDepth(4);
    a.setSplitInto(10);
    a.setMaxDepth(10);
    a.setMaxId(123456);
    BoxController b(a);
    // Check that it is the same
    compareBoxControllers(a, b);
  }

  void test_MRU_access()
  {
    BoxController a(2);
    DiskBuffer & dbuf = a.getDiskBuffer();
    // Set the cache parameters

    // Can't have 0-sized events
    TS_ASSERT_THROWS_ANYTHING( a.setCacheParameters(0, 4560) );
    a.setCacheParameters(40, 123);

    TS_ASSERT_EQUALS( dbuf.getWriteBufferSize(), 123);
  }

  void test_construction_defaults()
  {
    // Check the constructor defaults.
    BoxController box_controller(2);
    TS_ASSERT_EQUALS(2, box_controller.getNDims());
    TS_ASSERT_EQUALS(1, box_controller.getNumSplit());
    TS_ASSERT_EQUALS(0, box_controller.getMaxId());
    TS_ASSERT_EQUALS(true, box_controller.useWriteBuffer());
  }


};

#endif
