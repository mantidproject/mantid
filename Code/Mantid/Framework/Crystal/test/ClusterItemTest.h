#ifndef MANTID_CRYSTAL_CLUSTERITEMTEST_H_
#define MANTID_CRYSTAL_CLUSTERITEMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCrystal/ClusterItem.h"

using Mantid::Crystal::ClusterItem;

class ClusterItemTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ClusterItemTest *createSuite() { return new ClusterItemTest(); }
  static void destroySuite( ClusterItemTest *suite ) { delete suite; }


  void test_make_first_of_cluster()
  {
    ClusterItem item(12);
    TS_ASSERT_EQUALS(12, item.getId());
    TS_ASSERT_EQUALS(0, item.getDepth());
    TS_ASSERT_EQUALS(&item, item.getParent());
  }

  void test_make_with_parent()
  {
    ClusterItem parent(0);
    ClusterItem item(1, &parent);
    TS_ASSERT_EQUALS(1, item.getId());
    TS_ASSERT_EQUALS(1, item.getDepth());
    TS_ASSERT_EQUALS(0, item.getRank());
    TSM_ASSERT_EQUALS("Parent rank should be incremented", 2, parent.getRank())
    TS_ASSERT_EQUALS(&parent, item.getParent());
  }

  void test_copy()
  {
    ClusterItem a(1);
    ClusterItem b = a;
    TS_ASSERT_EQUALS(a.getId(), b.getId());
    TS_ASSERT_EQUALS(a.getRoot(), b.getRoot());
    TS_ASSERT_EQUALS(a.getDepth(), b.getDepth());
    TS_ASSERT_EQUALS(a.getRank(), b.getRank());
  }

  void test_assign()
  {
    ClusterItem a(1);
    ClusterItem b(2);

    b=a;
    TS_ASSERT_DIFFERS(a.getId(), b.getId());
  }

  void test_increment_rank()
  {
    ClusterItem item(0);
    TS_ASSERT_EQUALS(1, item.getRank());
    item.incrementRank();
    TS_ASSERT_EQUALS(2, item.getRank());
    item.incrementRank();
    TS_ASSERT_EQUALS(3, item.getRank());
  }

  void test_decrement_rank()
  {
    ClusterItem item(0);
    item.incrementRank();
    item.incrementRank();

    TS_ASSERT_EQUALS(3, item.getRank());
    item.decrementRank();
    TS_ASSERT_EQUALS(2, item.getRank());
  }

  void test_decrement_parent_rank_on_death()
  {
    ClusterItem parent(0);
    TS_ASSERT_EQUALS(1, parent.getRank());
    {
      ClusterItem child(1, &parent);
      TS_ASSERT_EQUALS(2, parent.getRank());
    }
    TSM_ASSERT_EQUALS("Parent rank should be reduced as child item destroyed", 1, parent.getRank());
  }

  void test_find_root()
  {
    ClusterItem a(0);
    ClusterItem b(1, &a);
    ClusterItem c(2, &b);
    ClusterItem d(3, &c);

    //All have a common root.
    TS_ASSERT_EQUALS(a.getId(), a.getRoot());
    TS_ASSERT_EQUALS(a.getId(), b.getRoot());
    TS_ASSERT_EQUALS(a.getId(), c.getRoot());
    TS_ASSERT_EQUALS(a.getId(), d.getRoot());
  }

  void test_make_compressed()
  {
    ClusterItem a(0);
    ClusterItem b(1, &a);
    ClusterItem c(2, &b);

    TSM_ASSERT_EQUALS("Relationship prior to compression", a.getId(), c.getRoot());

    ClusterItem compressed = c;
    compressed.compress();

    TSM_ASSERT_EQUALS("Relationship after compression", a.getId(), c.getRoot());
    TSM_ASSERT_EQUALS("Depth should be collapsed", 1, compressed.getDepth());
    TSM_ASSERT_EQUALS("Id should be the same", c.getId(), compressed.getId());

  }

  void test_union_with_simple_shared_root()
  {
    ClusterItem a(0);
    ClusterItem b(1, &a);
    ClusterItem c(2, &a);

    //Everything should be the same before and after.
    c.unionWith(b);

    TS_ASSERT_EQUALS( c.getRoot(), a.getId() );
    TS_ASSERT_EQUALS( b.getRoot(), a.getId() );
    TS_ASSERT_EQUALS( 1, c.getDepth());
    TS_ASSERT_EQUALS( 1, c.getDepth());
  }

  void test_union_with_complex_shared_root()
  {
    ClusterItem a(0);
    ClusterItem b(1, &a);
    ClusterItem c(2, &a);
    ClusterItem d(3, &c);

    //Everything should be the same before and after.
    c.unionWith(d);

    TS_ASSERT_EQUALS( c.getRoot(), a.getId() );
    TS_ASSERT_EQUALS( b.getRoot(), a.getId() );
    TS_ASSERT_EQUALS( 1, c.getDepth());
    TSM_ASSERT_EQUALS("Depth should have been reduced", 1, d.getDepth());
  }



};


#endif /* MANTID_CRYSTAL_CLUSTERITEMTEST_H_ */
