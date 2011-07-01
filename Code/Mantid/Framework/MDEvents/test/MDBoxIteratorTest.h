#ifndef MANTID_MDEVENTS_MDBOXITERATORTEST_H_
#define MANTID_MDEVENTS_MDBOXITERATORTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/MDBoxIterator.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDGridBox.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid;
using namespace Mantid::Kernel;

class MDBoxIteratorTest : public CxxTest::TestSuite
{
public:
  typedef MDGridBox<MDEvent<1>,1> gbox_t;
  typedef IMDBox<MDEvent<1>,1> ibox_t;

  //--------------------------------------------------------------------------------------
  /** Make a gridded box with this structure:
               A
               |
      B0 -- B1 -------- B2 ------------ B3
      |                 |
    C00-3        C20 C21 C22 C23
                      |
                  D210 D211 D212 D213

   */
  gbox_t *A;
  ibox_t *B1, *B3;
  gbox_t *B0, *B2;
  ibox_t *C00, *C01, *C02, *C03;
  ibox_t *C20,       *C22, *C23;
  gbox_t *C21;
  ibox_t *D210, *D211, *D212, *D213;
  MDBoxIterator<MDEvent<1>,1> * it;
  void setUp()
  {
    // Top level grid box
    A = MDEventsTestHelper::makeMDGridBox<1>(4, 1, 0.0, 64.0);
    A->splitContents(0); // Split B0 into C00 to C03
    A->splitContents(2); // Split B2 into C20 to C23
    B0 = dynamic_cast<gbox_t *>(A->getChild(0));
    B2 = dynamic_cast<gbox_t *>(A->getChild(2));
    B2->splitContents(1); // Split C21 into D210 to D212

    B1 = A->getChild(1);
    B3 = A->getChild(3);
    TS_ASSERT_EQUALS( B1, A->getChild(1));
    C00 = B0->getChild(0);
    C01 = B0->getChild(1);
    C02 = B0->getChild(2);
    C03 = B0->getChild(3);
    C20 = B2->getChild(0);
    C21 = dynamic_cast<gbox_t *>(B2->getChild(1));
    C22 = B2->getChild(2);
    C23 = B2->getChild(3);
    D210 = C21->getChild(0);
    D211 = C21->getChild(1);
    D212 = C21->getChild(2);
    D213 = C21->getChild(3);
  }

  //--------------------------------------------------------------------------------------
  void test_ctor_with_null_box_fails()
  {
    typedef MDBoxIterator<MDEvent<1>,1> boxit_t;
    boxit_t * it;
    TS_ASSERT_THROWS_ANYTHING( it = new boxit_t(NULL, 10, false); );
  }

  //--------------------------------------------------------------------------------------
  /** Increment the iterator and return true if the next box is the expected one*/
  bool nextIs(MDBoxIterator<MDEvent<1>,1> * it, ibox_t * expected)
  {
    // std::cout << it->getBox()->getExtentsStr() << std::endl;
    if (!it->next()) return false;
    if (it->getBox() != expected) return false;
    return true;
  }

  void test_iterator_basic()
  {
    // Create an iterator
    it = new MDBoxIterator<MDEvent<1>,1>(A, 20, false);

    // Start with the top one
    TS_ASSERT_EQUALS( it->getBox(), A);
    TS_ASSERT( nextIs(it, B0) );
    TS_ASSERT( nextIs(it, C00) );
    TS_ASSERT( nextIs(it, C01) );
    TS_ASSERT( nextIs(it, C02) );
    TS_ASSERT( nextIs(it, C03) );
    TS_ASSERT( nextIs(it, B1) );
    TS_ASSERT( nextIs(it, B2) );
    TS_ASSERT( nextIs(it, C20) );
    TS_ASSERT( nextIs(it, C21) );
    TS_ASSERT( nextIs(it, D210) );
    TS_ASSERT( nextIs(it, D211) );
    TS_ASSERT( nextIs(it, D212) );
    TS_ASSERT( nextIs(it, D213) );
    TS_ASSERT( nextIs(it, C22) );
    TS_ASSERT( nextIs(it, C23) );
    TS_ASSERT( nextIs(it, B3) );
    // No more!
    TS_ASSERT( !it->next() );
    // Calling next again does not cause problems.
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );
  }

  void test_depth_limit_1()
  {
    // Limit depth to 1 (the B level)
    it = new MDBoxIterator<MDEvent<1>,1>(A, 1, false);
    TS_ASSERT_EQUALS( it->getBox(), A);
    TS_ASSERT( nextIs(it, B0) );
    TS_ASSERT( nextIs(it, B1) );
    TS_ASSERT( nextIs(it, B2) );
    TS_ASSERT( nextIs(it, B3) );
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );
  }

  void test_depth_limit_0()
  {
    // Limit depth to 0 (the A level)
    it = new MDBoxIterator<MDEvent<1>,1>(A, 0, false);
    TS_ASSERT_EQUALS( it->getBox(), A);
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );
  }

  void test_starting_deeper_fails_for_wrong_maxDepth()
  {
    // Typedef for the iterator (for macros)
    typedef MDBoxIterator<MDEvent<1>,1> boxit_t;
    // Start at a depth of 1 (on B0): you need to give a valid maxDepth
    TS_ASSERT_THROWS_ANYTHING( it = new boxit_t(B0, 0, false); );
  }

  void test_starting_deeper()
  {
    // Start at a depth of 1 (on B0)
    it = new MDBoxIterator<MDEvent<1>,1>(B0, 20, false);
    TS_ASSERT_EQUALS( it->getBox(), B0);
    TS_ASSERT( nextIs(it, C00) );
    TS_ASSERT( nextIs(it, C01) );
    TS_ASSERT( nextIs(it, C02) );
    TS_ASSERT( nextIs(it, C03) );
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );
  }

  void test_leaf_only()
  {
    // Leaf-only iterator = skips anything with children
    it = new MDBoxIterator<MDEvent<1>,1>(A, 20, true);

    // This is the first leaf node
    TS_ASSERT_EQUALS( it->getBox(), C00);
    TS_ASSERT( nextIs(it, C01) );
    TS_ASSERT( nextIs(it, C02) );
    TS_ASSERT( nextIs(it, C03) );
    TS_ASSERT( nextIs(it, B1) );
    TS_ASSERT( nextIs(it, C20) );
    TS_ASSERT( nextIs(it, D210) );
    TS_ASSERT( nextIs(it, D211) );
    TS_ASSERT( nextIs(it, D212) );
    TS_ASSERT( nextIs(it, D213) );
    TS_ASSERT( nextIs(it, C22) );
    TS_ASSERT( nextIs(it, C23) );
    TS_ASSERT( nextIs(it, B3) );
    // No more!
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );
  }

  void test_leaf_only_depth_2()
  {
    // A node is considered a 'leaf' if it is at maxDepth
    it = new MDBoxIterator<MDEvent<1>,1>(A, 2, true);

    // This is the first leaf node
    TS_ASSERT_EQUALS( it->getBox(), C00);
    TS_ASSERT( nextIs(it, C01) );
    TS_ASSERT( nextIs(it, C02) );
    TS_ASSERT( nextIs(it, C03) );
    TS_ASSERT( nextIs(it, B1) );
    TS_ASSERT( nextIs(it, C20) );
    TS_ASSERT( nextIs(it, C21) ); // This is now a 'leaf' due to the maxDepth
    TS_ASSERT( nextIs(it, C22) );
    TS_ASSERT( nextIs(it, C23) );
    TS_ASSERT( nextIs(it, B3) );
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );
  }

  void test_leaf_only_depth_1()
  {
    // A node is considered a 'leaf' if it is at maxDepth
    it = new MDBoxIterator<MDEvent<1>,1>(A, 1, true);
    // This is the first leaf node
    TS_ASSERT_EQUALS( it->getBox(), B0);
    TS_ASSERT( nextIs(it, B1) );
    TS_ASSERT( nextIs(it, B2) );
    TS_ASSERT( nextIs(it, B3) );
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );
  }

  void test_leaf_only_depth_0()
  {
    // A node is considered a 'leaf' if it is at maxDepth
    it = new MDBoxIterator<MDEvent<1>,1>(A, 0, true);
    // This is the ONLY leaf node
    TS_ASSERT_EQUALS( it->getBox(), A);
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );
  }

  void test_leaf_only_starting_deeper()
  {
    //Now we start at B2 and look at only leaves
    it = new MDBoxIterator<MDEvent<1>,1>(B2, 10, true);
    TS_ASSERT_EQUALS( it->getBox(), C20);
    TS_ASSERT( nextIs(it, D210) );
    TS_ASSERT( nextIs(it, D211) );
    TS_ASSERT( nextIs(it, D212) );
    TS_ASSERT( nextIs(it, D213) );
    TS_ASSERT( nextIs(it, C22) );
    TS_ASSERT( nextIs(it, C23) );
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );
  }

  void test_leaf_only_starting_deeper_depth_limited()
  {
    //Now we start at B2 and look at only leaves up to depth 2
    it = new MDBoxIterator<MDEvent<1>,1>(B2, 2, true);
    TS_ASSERT_EQUALS( it->getBox(), C20);
    TS_ASSERT( nextIs(it, C21) );
    TS_ASSERT( nextIs(it, C22) );
    TS_ASSERT( nextIs(it, C23) );
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );
  }

  //--------------------------------------------------------------------------------------
  /** If you have just a MD Box, there is only one return from the iterator,
   * and it doesn't crash or anything ;)
   */
  void test_iterator_just_one_box()
  {
    // Top level grid box
    ibox_t * A = MDEventsTestHelper::makeMDBox1();
    MDBoxIterator<MDEvent<1>,1> * it = new MDBoxIterator<MDEvent<1>,1>(A, 20, false);
    TS_ASSERT_EQUALS( it->getBox(), A);
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );
  }

};


class MDBoxIteratorTestPerformace : public CxxTest::TestSuite
{
public:
  MDGridBox<MDEvent<3>,3> * top;

  void setUp()
  {
    // About 2 mi1968876
    top = MDEventsTestHelper::makeRecursiveMDGridBox<3>(5, 2);
  }

  void tearDown()
  {
    delete top;
  }

  // ---------------------------------------------------------------
  /** Make a simple iterator that will go through all the boxes */
  void do_test_iterator(bool leafOnly)
  {
    // Count the top level box.
    size_t counter = 1;
    IMDBox<MDEvent<3>,3> * box = NULL;
    MDBoxIterator<MDEvent<3>,3> it(top, 20, leafOnly);

    // Count all of them
    while (it.next())
    {
      box = it.getBox();
      counter++;
    }
    TS_ASSERT( box );
    size_t expected = 125*125*125 + 125*125 + 125 + 1;
    if (leafOnly)
      expected = 125*125*125;
    TS_ASSERT_EQUALS( counter, expected );
  }

  void test_iterator()
  {
    do_test_iterator(false);
  }

  void test_iterator_leafOnly()
  {
    do_test_iterator(true);
  }

  // ---------------------------------------------------------------
  /** This iterator will also take the boxes and fill
   * a vector with them; this is a full speed comparison to getBoxes()
   * which directly returns that vector, if that happens to be what you need.*/
  void do_test_iterator_that_fills_a_vector(bool leafOnly)
  {
    IMDBox<MDEvent<3>,3> * box;
    MDBoxIterator<MDEvent<3>,3> it(top, 20, leafOnly);
    std::vector< IMDBox<MDEvent<3>,3> * > boxes;

    // Iterate and fill the vector as you go.
    boxes.push_back(it.getBox());
    while (it.next())
    {
      box = it.getBox();
      boxes.push_back(box);
    }
    TS_ASSERT( box );
    size_t expected = 125*125*125 + 125*125 + 125 + 1;
    if (leafOnly)
      expected = 125*125*125;
    TS_ASSERT_EQUALS( boxes.size(), expected );
  }

  void test_iterator_that_fills_a_vector()
  {
    do_test_iterator_that_fills_a_vector(false);
  }

  void test_iterator_that_fills_a_vector_leafOnly()
  {
    do_test_iterator_that_fills_a_vector(true);
  }



  // ---------------------------------------------------------------
  /** For comparison, let's use getBoxes() that fills a vector directly.
   * After that, we iterate through them to compare how long the whole operation takes.
   */
  void do_test_getBoxes(bool leafOnly)
  {
    size_t expected = 125*125*125 + 125*125 + 125 + 1;
    if (leafOnly)
      expected = 125*125*125;

    std::vector< IMDBox<MDEvent<3>,3> * > boxes;
    top->getBoxes(boxes, 20, leafOnly);
    TS_ASSERT_EQUALS( boxes.size(), expected);

    // Now we still need to iterate through the vector to do anything, so this is a more fair comparison
    size_t counter = 0;
    IMDBox<MDEvent<3>,3> * box;
    std::vector< IMDBox<MDEvent<3>,3> * >::iterator it;
    std::vector< IMDBox<MDEvent<3>,3> * >::iterator it_end = boxes.end();
    for (it = boxes.begin(); it != it_end; it++)
    {
      box = *it;
      counter++;
    }

    TS_ASSERT_EQUALS( counter, expected );
  }

  void test_getBoxes()
  {
    do_test_getBoxes(false);
  }

  void test_getBoxes_leafOnly()
  {
    do_test_getBoxes(true);
  }

};


#endif /* MANTID_MDEVENTS_MDBOXITERATORTEST_H_ */

