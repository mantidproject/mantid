#ifndef MANTID_CRYSTAL_DISJOINTELEMENTTEST_H_
#define MANTID_CRYSTAL_DISJOINTELEMENTTEST_H_

#include <cxxtest/TestSuite.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include "MantidCrystal/DisjointElement.h"

using Mantid::Crystal::DisjointElement;

class DisjointElementTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DisjointElementTest *createSuite() { return new DisjointElementTest(); }
  static void destroySuite( DisjointElementTest *suite ) { delete suite; }

  void test_default_constructor()
  {
    DisjointElement item;
    TSM_ASSERT("Should be empty", item.isEmpty());
  }

  void test_make_first_of_cluster()
  {
    DisjointElement item(12);
    TS_ASSERT_EQUALS(12, item.getId());
    TS_ASSERT_EQUALS(0, item.getRank());
    TS_ASSERT(!item.isEmpty());
    TS_ASSERT_EQUALS(&item, item.getParent());
  }

  void test_set_id()
  {
    DisjointElement item;
    TS_ASSERT(item.isEmpty());
    item.setId(2);
    TS_ASSERT(!item.isEmpty());
    TS_ASSERT_EQUALS(2.0, item.getId());
  }

  void test_copy()
  {
    DisjointElement item(1);
    DisjointElement copy = item;
    TS_ASSERT_EQUALS(item.getId(), copy.getId());
    TS_ASSERT_EQUALS(item.getRank(), copy.getRank());
    TS_ASSERT_DIFFERS(item.getParent(), copy.getParent());
  }

  void test_assign()
  {
    DisjointElement a(1);
    DisjointElement b = a;
    TS_ASSERT_EQUALS(a.getId(), b.getId());
    TS_ASSERT_EQUALS(a.getRank(), b.getRank());
    TS_ASSERT_DIFFERS(a.getParent(), b.getParent());
  }

  void test_increment_rank()
  {
    DisjointElement item(0);
    TS_ASSERT_EQUALS(0, item.getRank());
    item.incrementRank();
    TS_ASSERT_EQUALS(1, item.getRank());
    item.incrementRank();
    TS_ASSERT_EQUALS(2, item.getRank());
  }

  void test_union_two_singleton_sets()
  {
    DisjointElement item1(0);
    DisjointElement item2(1);

    // We now have two singletons. Diagram shows parents are selves.
    /*
     *   item1  item2
     *    |       |
     *   item1  item2
     */

    item1.unionWith(&item2);
    TS_ASSERT_EQUALS(0, item1.getRank());
    TSM_ASSERT_EQUALS("Same rank, but different parents, so item2, should take ownership", 1, item2.getRank());
    TSM_ASSERT_EQUALS("item2 should be parent", item1.getParent(), &item2);
  }

  void test_union_with_same_root()
  {
    DisjointElement child1(0);
    DisjointElement child2(1);
    DisjointElement base(2);
    child1.unionWith(&base);
    child2.unionWith(&base);
    TS_ASSERT_EQUALS(1, base.getRank());

    // We now have
    /*
     *       base
     *      /    \
     *   child1  child2
     */

    //Try to union child1 and child2. Nothing should change.

    child1.unionWith(&child2);
    TS_ASSERT_EQUALS(0, child1.getRank());
    TS_ASSERT_EQUALS(0, child2.getRank());
    TSM_ASSERT_EQUALS("base should be parent", child1.getParent(), &base);
    TSM_ASSERT_EQUALS("base should be parent", child2.getParent(), &base);
  }

  void test_union_with_different_roots()
  {
    DisjointElement a(0);
    DisjointElement b(1);
    DisjointElement c(2);
    b.unionWith(&a);
    TS_ASSERT_EQUALS(1, a.getRank());

    // We now have two trees. One is singleton.
    /*
     *     a    c
     *     |    |
     *     b    c
     */

    c.unionWith(&b);

    // We should get
    /*
     *       a
     *      / \
     *     b   c
     *
     */

    TS_ASSERT_EQUALS(0, b.getRank());
    TS_ASSERT_EQUALS(0, c.getRank());
    TSM_ASSERT_EQUALS("b should be parent of a", c.getParent(), &a);
    TSM_ASSERT_EQUALS("a should be parent of b", b.getParent(), &a);
    TSM_ASSERT_EQUALS("b and c should have a common root", b.getRoot(), c.getRoot());
  }

  void test_complex()
  {
    typedef boost::shared_ptr<DisjointElement> DisjointElement_sptr;
    typedef std::vector<DisjointElement_sptr> VecDisjointElement;

    // Create elements from 0-9
    VecDisjointElement vecElements;
    for(int i=0; i < 10; ++i)
    {
      vecElements.push_back(boost::make_shared<DisjointElement>(i));
    }

    // Merge selected sets.
    vecElements[3]->unionWith(vecElements[1].get());
    vecElements[1]->unionWith(vecElements[2].get());
    vecElements[2]->unionWith(vecElements[4].get());
    vecElements[0]->unionWith(vecElements[7].get());
    vecElements[8]->unionWith(vecElements[9].get());

    // Should get this.
    /*
     *       7     1       5    6   9
     *      /    / | \              |
     *     0    2  3  4             8
     *
     */


    TS_ASSERT_EQUALS(7, vecElements[0]->getRoot());

    TS_ASSERT_EQUALS(1, vecElements[2]->getRoot());
    TS_ASSERT_EQUALS(1, vecElements[3]->getRoot());
    TS_ASSERT_EQUALS(1, vecElements[4]->getRoot());

    TS_ASSERT_EQUALS(9, vecElements[8]->getRoot());

    TS_ASSERT_EQUALS(7, vecElements[7]->getRoot());
    TS_ASSERT_EQUALS(1, vecElements[1]->getRoot());
    TS_ASSERT_EQUALS(5, vecElements[5]->getRoot());
    TS_ASSERT_EQUALS(6, vecElements[6]->getRoot());
    TS_ASSERT_EQUALS(9, vecElements[9]->getRoot());
  }

};


#endif /* MANTID_CRYSTAL_DISJOINTELEMENTTEST_H_ */
