#ifndef MANTID_MDEVENTS_MDSPLITBOXTEST_H_
#define MANTID_MDEVENTS_MDSPLITBOXTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/MDSplitBox.h"
#include "MantidMDEvents/MDBox.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

class MDSplitBoxTest : public CxxTest::TestSuite
{
public:

  // =============================================================================================
  // ======================================== HELPER FUNCTIONS ===================================
  // =============================================================================================

  //-------------------------------------------------------------------------------------
  /** Generate an empty MDBox , 10x10*/
  MDBox<MDEvent<2>,2> * makeMDBox2()
  {
    // Split at 5 events
    BoxController_sptr splitter(new BoxController(2));
    splitter->setSplitThreshold(5);
    // Splits into 10 boxes
    splitter->setSplitInto(10);
    // Set the size
    MDBox<MDEvent<2>,2> * out = new MDBox<MDEvent<2>,2>(splitter);
    out->setExtents(0, 0.0, 10.0);
    out->setExtents(1, 0.0, 10.0);
    out->calcVolume();
    return out;
  }


  //-------------------------------------------------------------------------------------
  /** Helper function compares the extents of the given box */
  template<typename MDBOX>
  void extents_match(MDBOX box, size_t dim, double min, double max)
  {
    TSM_ASSERT_DELTA(dim, box->getExtents(dim).min, min, 1e-6);
    TSM_ASSERT_DELTA(dim, box->getExtents(dim).max, max, 1e-6);
  }

  // =============================================================================================
  // ======================================== TEST FUNCTIONS =====================================
  // =============================================================================================
  void test_constructor()
  {
    // Start with an empty MDBox2
    MDBox<MDEvent<2>,2> * mdbox = makeMDBox2();

    // Fill events that are more spread in dimension 1.
    for (double x=40; x<60; x++) //20
      for (double y=20; y<80; y++) //60
      {
        double centers[2] = {x*0.1,y*0.1 + 0.05};
        mdbox->addEvent( MDEvent<2>(2.0, 2.0, centers) );
      }
    TS_ASSERT_EQUALS(mdbox->getNPoints(), 20*60);

    // Build the splitbox
    typedef MDSplitBox<MDEvent<2>,2> MDSplitBox2;
    typedef IMDBox<MDEvent<2>,2> IMDBox2;
    MDSplitBox2 * box = NULL;
    TS_ASSERT_THROWS_NOTHING( box = new MDSplitBox2(mdbox) );

    TS_ASSERT_EQUALS(box->getNPoints(), 20*60);
    TS_ASSERT_DELTA(box->getSignal(), double(box->getNPoints())*2.0, 1e-5);
    TS_ASSERT_DELTA(box->getErrorSquared(), double(box->getNPoints())*2.0, 1e-5);

    // Where did it split?
    TS_ASSERT_EQUALS(box->getSplitDimension(), 1);
    TS_ASSERT_DELTA(box->getSplitPoint(), 5.00, 1e-3);

    IMDBox2 * left;
    IMDBox2 * right;
    TS_ASSERT_THROWS_NOTHING(left = box->getLeft());
    TS_ASSERT_THROWS_NOTHING(right = box->getRight());
    TS_ASSERT(left);
    TS_ASSERT(right);
    // Dimensions make sense
    extents_match(left, 0, 0.0, 10.0);
    extents_match(right, 0, 0.0, 10.0);
    extents_match(left, 1, 0.0, 5.0);
    extents_match(right, 1, 5.0, 10.0);

    // Points were split evenly
    TS_ASSERT_EQUALS(left->getNPoints(), 600);
    TS_ASSERT_EQUALS(right->getNPoints(), 600);

    // Signals etc. are okay
    TS_ASSERT_EQUALS(left->getSignal(), double(left->getNPoints())*2.0);
    TS_ASSERT_EQUALS(left->getErrorSquared(), double(left->getNPoints())*2.0);

    // Depths is deeper?
    TS_ASSERT_EQUALS(left->getDepth(), 1);
    TS_ASSERT_EQUALS(right->getDepth(), 1);
    TS_ASSERT_EQUALS(left->getBoxController(), box->getBoxController());
    TS_ASSERT_EQUALS(right->getBoxController(), box->getBoxController());


    // Only 2 MDBoxes contained.
    TS_ASSERT_EQUALS(box->getNumMDBoxes(), 2);

  }


  /** Test the constructor that forces a particular split */
  void test_manual_constructor()
  {
    // Start with an empty MDBox2
    MDBox<MDEvent<2>,2> * mdbox = makeMDBox2();

    typedef MDSplitBox<MDEvent<2>,2> MDSplitBox2;
    typedef IMDBox<MDEvent<2>,2> IMDBox2;
    MDSplitBox2 * box = NULL;

    //Manually create it
    TS_ASSERT_THROWS_NOTHING( box = new MDSplitBox2(mdbox, 1, 5.0) );

    // Where did it split?
    TS_ASSERT_EQUALS(box->getSplitDimension(), 1);
    TS_ASSERT_DELTA(box->getSplitPoint(), 5.00, 1e-3);

    IMDBox2 * left;
    IMDBox2 * right;
    TS_ASSERT_THROWS_NOTHING(left = box->getLeft());
    TS_ASSERT_THROWS_NOTHING(right = box->getRight());
    TS_ASSERT(left);
    TS_ASSERT(right);
    // Dimensions make sense
    extents_match(left, 0, 0.0, 10.0);
    extents_match(right, 0, 0.0, 10.0);
    extents_match(left, 1, 0.0, 5.0);
    extents_match(right, 1, 5.0, 10.0);

  }
};

#endif /* MANTID_MDEVENTS_MDSPLITBOXTEST_H_ */

