#ifndef MANTID_MDEVENTS_MDBOXITERATORTEST_H_
#define MANTID_MDEVENTS_MDBOXITERATORTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/MDBoxIterator.h"
#include "MantidMDEvents/MDGridBox.h"
#include "MantidMDEvents/MDEventFactory.h"
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
  /** Increment the iterator and return true if the next box is the expected one*/
  bool nextIs(MDBoxIterator<MDEvent<1>,1> * it, ibox_t * expected)
  {
    //std::cout << it->getBox()->getExtentsStr() << std::endl;
    if (!it->next()) return false;
    if (it->getBox() != expected) return false;
    return true;
  }


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
  void xtest_iterator()
  {
    // Top level grid box
    gbox_t * A = MDEventsTestHelper::makeMDGridBox<1>(4, 1, 0.0, 64.0);
    A->splitContents(0); // Split B0 into C00 to C03
    A->splitContents(2); // Split B2 into C20 to C23
    gbox_t * B0 = dynamic_cast<gbox_t *>(A->getChild(0));
    gbox_t * B2 = dynamic_cast<gbox_t *>(A->getChild(2));
    B2->splitContents(1); // Split C21 into D210 to D212

    ibox_t * B1 = A->getChild(1);
    ibox_t * B3 = A->getChild(3);
    TS_ASSERT_EQUALS( B1, A->getChild(1));
    ibox_t * C00 = B0->getChild(0);
    ibox_t * C01 = B0->getChild(1);
    ibox_t * C02 = B0->getChild(2);
    ibox_t * C03 = B0->getChild(3);
    ibox_t * C20 = B2->getChild(0);
    gbox_t * C21 = dynamic_cast<gbox_t *>(B2->getChild(1));
    ibox_t * C22 = B2->getChild(2);
    ibox_t * C23 = B2->getChild(3);
    ibox_t * D210 = C21->getChild(0);
    ibox_t * D211 = C21->getChild(1);
    ibox_t * D212 = C21->getChild(2);
    ibox_t * D213 = C21->getChild(3);

    // Create an iterator
    MDBoxIterator<MDEvent<1>,1> * it = new MDBoxIterator<MDEvent<1>,1>(A, 20, false);

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

    // Limit depth to 1 (the B level)
    it = new MDBoxIterator<MDEvent<1>,1>(A, 1, false);
    TS_ASSERT_EQUALS( it->getBox(), A);
    TS_ASSERT( nextIs(it, B0) );
    TS_ASSERT( nextIs(it, B1) );
    TS_ASSERT( nextIs(it, B2) );
    TS_ASSERT( nextIs(it, B3) );
    TS_ASSERT( !it->next() );

  }


};

//
//class MDBoxIteratorTestPerformace : public CxxTest::TestSuite
//{
//public:
//  MDGridBox<MDEvent<3>,3> * top;
//
//  void setUp()
//  {
//    // About 2 mi1968876
//    top = MDEventsTestHelper::makeRecursiveMDGridBox<3>(5, 2);
//  }
//
//  void tearDown()
//  {
//    delete top;
//  }
//
//  /** Make a simple iterator that will go through all the boxes */
//  void test_iterator()
//  {
//    // Count the top level box.
//    size_t counter = 1;
//    IMDBox<MDEvent<3>,3> * box;
//    MDBoxIterator<MDEvent<3>,3> it(top, 20, false);
//
//    // Count all of them
//    while (it.next())
//    {
//      box = it.getBox();
//      counter++;
//    }
//    TS_ASSERT( box );
//    TS_ASSERT_EQUALS( counter, 125*125*125 + 125*125 + 125 + 1);
//  }
//
//  // ---------------------------------------------------------------
//  /** For comparison, let's use getBoxes() that fills a vector
//   */
//  void test_getBoxes()
//  {
//    std::vector< IMDBox<MDEvent<3>,3> * > boxes;
//    top->getBoxes(boxes, 20, false);
//    TS_ASSERT_EQUALS( boxes.size(), 125*125*125 + 125*125 + 125 + 1);
//
//    // Now we still need to iterate through the vector to do anything, so this is a more fair comparison
//    size_t counter = 0;
//    IMDBox<MDEvent<3>,3> * box;
//    std::vector< IMDBox<MDEvent<3>,3> * >::iterator it;
//    std::vector< IMDBox<MDEvent<3>,3> * >::iterator it_end = boxes.end();
//    for (it = boxes.begin(); it != it_end; it++)
//    {
//      box = *it;
//      counter++;
//    }
//
//    TS_ASSERT_EQUALS( counter, 125*125*125 + 125*125 + 125 + 1);
//  }
//
//};


#endif /* MANTID_MDEVENTS_MDBOXITERATORTEST_H_ */

