#ifndef MDGRIDBOXTEST_H
#define MDGRIDBOXTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ProgressText.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDGridBox.h"
#include "MantidMDEvents/BoxController.h"
#include <memory>
#include <iomanip>
#include <map>
#include <vector>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

class MDGridBoxTest :    public CxxTest::TestSuite
{
public:
  bool DODEBUG;
  MDGridBoxTest()
  {
    DODEBUG = false;
  }

protected:

  //=====================================================================================
  //===================================== HELPER METHODS ================================
  //=====================================================================================

  //-------------------------------------------------------------------------------------
  /** Generate an empty MDBox */
  MDBox<MDEvent<1>,1> * makeMDBox1()
  {
    // Split at 5 events
    BoxController_sptr splitter(new BoxController(1));
    splitter->setSplitThreshold(5);
    // Splits into 10 boxes
    splitter->setSplitInto(10);
    // Set the size
    MDBox<MDEvent<1>,1> * out = new MDBox<MDEvent<1>,1>(splitter);
    out->setExtents(0, 0.0, 10.0);
    return out;
  }

  //-------------------------------------------------------------------------------------
  /** Generate an empty MDBox with 3 dimensions, split 10x5x2 */
  MDBox<MDEvent<3>,3> * makeMDBox3()
  {
    // Split at 5 events
    BoxController_sptr splitter(new BoxController(3));
    splitter->setSplitThreshold(5);
    // Splits into 10x5x2 boxes
    splitter->setSplitInto(10);
    splitter->setSplitInto(1,5);
    splitter->setSplitInto(2,2);
    // Set the size to 10.0 in all directions
    MDBox<MDEvent<3>,3> * out = new MDBox<MDEvent<3>,3>(splitter);
    for (size_t d=0; d<3; d++)
      out->setExtents(d, 0.0, 10.0);
    return out;
  }


  //-------------------------------------------------------------------------------------
  /** Generate an empty MDBox with nd dimensions, splitting in 10x10 boxes
   * @param numEventsPerBox :: each sub-box will get this many events (all
   *        placed in the middle of each sub-box.
   * */
  template<size_t nd>
  MDGridBox<MDEvent<nd>,nd> * makeMDGridBox(size_t numEventsPerBox)
  {
    // Split at 5 events
    BoxController_sptr splitter(new BoxController(nd));
    splitter->setSplitThreshold(5);
    // Splits into 10x10x.. boxes
    splitter->setSplitInto(10);
    // Set the size to 10.0 in all directions
    MDBox<MDEvent<nd>,nd> * box = new MDBox<MDEvent<nd>,nd>(splitter);
    for (size_t d=0; d<nd; d++)
      box->setExtents(d, 0.0, 10.0);

    // Split
    MDGridBox<MDEvent<nd>,nd> * out = new MDGridBox<MDEvent<nd>,nd>(box);

    // Make an event in the middle of each box
    for (size_t i=0; i < numEventsPerBox; i++)
      for (double x=0.5; x < 10; x += 1.0)
        for (double y=0.5; y < 10; y += 1.0)
        {
          double centers[2] = {x,y};
          out->addEvent( MDEvent<2>(2.0, 2.0, centers) );
        }
    out->refreshCache();
    return out;
  }

  //-------------------------------------------------------------------------------------
  /** Recursively split an existing MDGridBox
   *
   * @param box :: box to split
   * @param atRecurseLevel :: This is the recursion level at which we are
   * @param recurseLimit :: this is where to spot
   */
  template<size_t nd>
  void recurseSplit(MDGridBox<MDEvent<nd>,nd> * box, size_t atRecurseLevel, size_t recurseLimit)
  {
    typedef std::vector<IMDBox<MDEvent<nd>,nd> *> boxVector;
    if (atRecurseLevel >= recurseLimit) return;

    // Split all the contents
    boxVector boxes;
    boxes = box->getBoxes();
    for (size_t i=0; i< boxes.size(); i++)
      box->splitContents(i);

    // Retrieve the contained MDGridBoxes
    boxes = box->getBoxes();

    // Go through them and split them
    for (size_t i=0; i< boxes.size(); i++)
    {
      MDGridBox<MDEvent<nd>,nd> * containedbox = dynamic_cast<MDGridBox<MDEvent<nd>,nd> *>(boxes[i]);
      if (containedbox)
        recurseSplit(containedbox, atRecurseLevel+1, recurseLimit);
    }
  }


  //-------------------------------------------------------------------------------------
  /** Generate a recursively gridded MDGridBox
   *
   * @param splitInto :: boxes split into this many boxes/side
   * @param levels :: levels of splitting recursion (0=just the top level is split)
   * @return
   */
  template<size_t nd>
  MDGridBox<MDEvent<nd>,nd> * makeRecursiveMDGridBox(size_t splitInto, size_t levels)
  {
    // Split at 5 events
    BoxController_sptr splitter(new BoxController(nd));
    splitter->setSplitThreshold(5);
    // Splits into splitInto x splitInto x ... boxes
    splitter->setSplitInto(splitInto);
    // Set the size to splitInto*1.0 in all directions
    MDBox<MDEvent<nd>,nd> * box = new MDBox<MDEvent<nd>,nd>(splitter);
    for (size_t d=0; d<nd; d++)
      box->setExtents(d, 0.0, splitInto*1.0);
    // Split into the gridbox.
    MDGridBox<MDEvent<nd>,nd> * gridbox = new MDGridBox<MDEvent<nd>,nd>(box);

    // Now recursively split more
    recurseSplit(gridbox, 0, levels);

    return gridbox;
  }


  //-------------------------------------------------------------------------------------
  /** Return a vector with this many MDEvents, spaced evenly from 0.5, 1.5, etc. */
  std::vector<MDEvent<1> > makeMDEvents1(size_t num)
  {
    std::vector<MDEvent<1> > out;
    for (size_t i=0; i<num; i++)
    {
      CoordType coords[1] = {i*1.0+0.5};
      out.push_back( MDEvent<1>(1.0, 1.0, coords) );
    }
    return out;
  }



  //=====================================================================================
  //===================================== TEST METHODS ==================================
  //=====================================================================================


public:

  //-------------------------------------------------------------------------------------
  void test_MDBoxConstructor()
  {
    MDBox<MDEvent<1>,1> * b = makeMDBox1();
    TS_ASSERT_EQUALS( b->getNumDims(), 1);
    TS_ASSERT_EQUALS( b->getNPoints(), 0);
    TS_ASSERT_DELTA( b->getExtents(0).min, 0.0,  1e-5);
    TS_ASSERT_DELTA( b->getExtents(0).max, 10.0, 1e-5);
    delete b;
  }


  //-------------------------------------------------------------------------------------
  void test_MDGridBox_Construction()
  {
    MDBox<MDEvent<1>,1> * b = makeMDBox1();
    // Give it 10 events
    b->addEvents( makeMDEvents1(10) );
    TS_ASSERT_EQUALS( b->getNPoints(), 10 );

    // Build the grid box out of it
    MDGridBox<MDEvent<1>,1> * g = new MDGridBox<MDEvent<1>,1>(b);

    // Look overall; it has 10 points
    TS_ASSERT_EQUALS(g->getNumDims(), 1);
    TS_ASSERT_EQUALS(g->getNPoints(), 10);
    // Its depth level should be 0 (same as parent)
    TS_ASSERT_EQUALS(g->getDepth(), 0);
    // It was split into 10 MDBoxes.
    TS_ASSERT_EQUALS( g->getNumMDBoxes(), 10);

    // It has a BoxController
    TS_ASSERT( g->getBoxController() );

    // Check the boxes
    std::vector<IMDBox<MDEvent<1>,1> *> boxes = g->getBoxes();
    TS_ASSERT_EQUALS( boxes.size(), 10);
    for (size_t i=0; i<boxes.size(); i++)
    {
      MDBox<MDEvent<1>,1> * box = dynamic_cast<MDBox<MDEvent<1>,1> *>(boxes[i]);
      TS_ASSERT_DELTA(box->getExtents(0).min, i*1.0, 1e-6);
      TS_ASSERT_DELTA(box->getExtents(0).max, (i+1)*1.0, 1e-6);
      // Look at the single event in there
      TS_ASSERT_EQUALS(box->getNPoints(), 1);
      MDEvent<1> ev = box->getEvents()[0];
      TS_ASSERT_DELTA(ev.getCenter(0), i*1.0 + 0.5, 1e-5);
      // Its depth level should be 1 (deeper than parent)
      TS_ASSERT_EQUALS(box->getDepth(), 1);
    }

    // Now we add 10 more events
    g->addEvents( makeMDEvents1(10) );

    // And now there should be 2 events per box
    for (size_t i=0; i<10; i++)
    {
      MDBox<MDEvent<1>,1> * box = dynamic_cast<MDBox<MDEvent<1>,1> *>(boxes[i]);
      TS_ASSERT_EQUALS(box->getNPoints(), 2);
    }
  }


  //-------------------------------------------------------------------------------------
  /** Helper function compares the extents of the given box */
  template<typename MDBOX>
  void extents_match(MDBOX box, size_t dim, double min, double max)
  {
    TSM_ASSERT_DELTA(dim, box->getExtents(dim).min, min, 1e-6);
    TSM_ASSERT_DELTA(dim, box->getExtents(dim).max, max, 1e-6);
  }


  //-------------------------------------------------------------------------------------
  /** Build a 3D MDGridBox and check that the boxes created within are where you expect */
  void test_MDGridBox3()
  {
    MDBox<MDEvent<3>,3> * b = makeMDBox3();
    // Build the grid box out of it
    MDGridBox<MDEvent<3>,3> * g = new MDGridBox<MDEvent<3>,3>(b);
    TS_ASSERT_EQUALS(g->getNumDims(), 3);

    // Check the boxes
    std::vector<IMDBox<MDEvent<3>,3> *> boxes = g->getBoxes();
    TS_ASSERT_EQUALS( boxes.size(), 10*5*2);
    for (size_t i=0; i<boxes.size(); i++)
    {
      MDBox<MDEvent<3>,3> * box = dynamic_cast<MDBox<MDEvent<3>,3> *>(boxes[i]);
      TS_ASSERT( box );
    }
    MDBox<MDEvent<3>,3> * box;
    box = dynamic_cast<MDBox<MDEvent<3>,3> *>(boxes[1]);
    extents_match(box, 0, 1.0, 2.0);
    extents_match(box, 1, 0.0, 2.0);
    extents_match(box, 2, 0.0, 5.0);
    box = dynamic_cast<MDBox<MDEvent<3>,3> *>(boxes[10]);
    extents_match(box, 0, 0.0, 1.0);
    extents_match(box, 1, 2.0, 4.0);
    extents_match(box, 2, 0.0, 5.0);
    box = dynamic_cast<MDBox<MDEvent<3>,3> *>(boxes[53]);
    extents_match(box, 0, 3.0, 4.0);
    extents_match(box, 1, 0.0, 2.0);
    extents_match(box, 2, 5.0, 10.0);
  }


  //-------------------------------------------------------------------------------------
  /** Start with a grid box, split some of its contents into sub-gridded boxes. */
  void test_splitContents()
  {
    MDGridBox<MDEvent<2>,2> * superbox = makeMDGridBox<2>(0);
    MDGridBox<MDEvent<2>,2> * gb;
    MDBox<MDEvent<2>,2> * b;

    std::vector<IMDBox<MDEvent<2>,2>*> boxes;

    // Start with 100 boxes
    TS_ASSERT_EQUALS( superbox->getNumMDBoxes(), 100);

    // The box is a MDBox at first
    boxes = superbox->getBoxes();
    b = dynamic_cast<MDBox<MDEvent<2>,2> *>(boxes[0]);
    TS_ASSERT( b );

    TS_ASSERT_THROWS_NOTHING(superbox->splitContents(0));

    // Now, it has turned into a GridBox
    boxes = superbox->getBoxes();
    gb = dynamic_cast<MDGridBox<MDEvent<2>,2> *>(boxes[0]);
    TS_ASSERT( gb );

    // There are now 199 MDBoxes; the 99 at level 1, and 100 at level 2
    TS_ASSERT_EQUALS( superbox->getNumMDBoxes(), 199);


    // You can split it again and it does nothing
    TS_ASSERT_THROWS_NOTHING(superbox->splitContents(0));

    // Still a grid box
    boxes = superbox->getBoxes();
    gb = dynamic_cast<MDGridBox<MDEvent<2>,2> *>(boxes[0]);
    TS_ASSERT( gb );
  }

  //-------------------------------------------------------------------------------------
  /** Adding a single event pushes it as deep as the current grid
   * hierarchy allows
   */
  void test_addEvent_with_recursive_gridding()
  {
    MDGridBox<MDEvent<2>,2> * gb;
    MDBox<MDEvent<2>,2> * b;
    std::vector<IMDBox<MDEvent<2>,2>*> boxes;

    // 10x10 box, extents 0-10.0
    MDGridBox<MDEvent<2>,2> * superbox = makeMDGridBox<2>(0);
    // And the 0-th box is further split (
    TS_ASSERT_THROWS_NOTHING(superbox->splitContents(0));

    TS_ASSERT_EQUALS( superbox->getNPoints(), 0 );
    { // One event in 0th box of the 0th box.
      double centers[2] = {0.05, 0.05};
      superbox->addEvent( MDEvent<2>(2.0, 2.0, centers) );
    }
    { // One event in 1st box of the 0th box.
      double centers[2] = {0.15, 0.05};
      superbox->addEvent( MDEvent<2>(2.0, 2.0, centers) );
    }
    { // One event in 99th box.
      double centers[2] = {9.5, 9.5};
      superbox->addEvent( MDEvent<2>(2.0, 2.0, centers) );
    }

    // You must refresh the cache after adding individual events.
    superbox->refreshCache();

    TS_ASSERT_EQUALS( superbox->getNPoints(), 3 );

    // Retrieve the 0th grid box
    boxes = superbox->getBoxes();
    gb = dynamic_cast<MDGridBox<MDEvent<2>,2> *>(boxes[0]);
    TS_ASSERT( gb );

    // It has two points
    TS_ASSERT_EQUALS( gb->getNPoints(), 2 );

    // Retrieve the MDBox at 0th and 1st indexes in THAT gridbox
    boxes = gb->getBoxes();
    b = dynamic_cast<MDBox<MDEvent<2>,2> *>(boxes[0]);
    TS_ASSERT_EQUALS( b->getNPoints(), 1 );
    b = dynamic_cast<MDBox<MDEvent<2>,2> *>(boxes[1]);
    TS_ASSERT_EQUALS( b->getNPoints(), 1 );

    // Get the 99th box at the first level. It is not split
    boxes = superbox->getBoxes();
    b = dynamic_cast<MDBox<MDEvent<2>,2> *>(boxes[99]);
    TS_ASSERT( b ); if (!b) return;
    // And it has only the one point
    TS_ASSERT_EQUALS( b->getNPoints(), 1 );
  }


  //-------------------------------------------------------------------------------------
  /** Gauge how fast addEvent is with several levels of gridding
   * NOTE: DISABLED because it is slow.
   * */
  void xtest_addEvent_with_recursive_gridding_Performance()
  {
    // Make a 2D box split into 4, 4 levels deep. = 4^4^2 boxes at the bottom = 256^2 boxes.
    size_t numSplit = 4;
    for (size_t recurseLevels = 1; recurseLevels < 5; recurseLevels++)
    {
      std::cout << " --- Recursion Level " << recurseLevels << " --- " << std::endl;
      Timer tim1;
      double boxes_per_side = pow(numSplit*1.0, recurseLevels*1.0);
      double spacing = (numSplit*1.0)/boxes_per_side;
      // How many times to add the same event
      size_t num_to_repeat = size_t(1e7 / (boxes_per_side*boxes_per_side));

      MDGridBox<MDEvent<2>,2> * box = this->makeRecursiveMDGridBox<2>(numSplit, recurseLevels);
      std::cout << tim1.elapsed() << " seconds to generate the " << boxes_per_side << "^2 boxes." << std::endl;

      for (double x=0; x < numSplit; x += spacing)
        for (double y=0; y < numSplit; y += spacing)
        {
          for (size_t i=0; i<num_to_repeat; i++)
          {
            double centers[2] = {x,y};
            box->addEvent( MDEvent<2>(2.0, 2.0, centers) );
          }
        }
      // You must refresh the cache after adding individual events.
      box->refreshCache();

      double sec = tim1.elapsed();
      std::cout << sec << " seconds to add " << box->getNPoints() << " events. Each box had " << num_to_repeat << " events." << std::endl;
      std::cout << "equals " << 1e6*sec/box->getNPoints() << " seconds per million events." << std::endl;
    }

  }



  //-------------------------------------------------------------------------------------
  /** Fill a 10x10 gridbox with events
   *
   * Tests that bad events are thrown out when using addEvents.
   * */
  void test_addEvents_2D()
  {
    MDGridBox<MDEvent<2>,2> * b = makeMDGridBox<2>(0);
    std::vector< MDEvent<2> > events;

    // Make an event in the middle of each box
    for (double x=0.5; x < 10; x += 1.0)
      for (double y=0.5; y < 10; y += 1.0)
      {
        double centers[2] = {x,y};
        events.push_back( MDEvent<2>(2.0, 2.0, centers) );
      }

    size_t numbad;
    TS_ASSERT_THROWS_NOTHING( numbad = b->addEvents( events ); );
    // Get the right totals again
    b->refreshCache();
    TS_ASSERT_EQUALS( numbad, 0);
    TS_ASSERT_EQUALS( b->getNPoints(), 100);
    TS_ASSERT_EQUALS( b->getSignal(), 100*2.0);
    TS_ASSERT_EQUALS( b->getErrorSquared(), 100*2.0);

    // Get all the boxes contained
    std::vector<IMDBox<MDEvent<2>,2>*> boxes = b->getBoxes();
    TS_ASSERT_EQUALS( boxes.size(), 100);
    for (size_t i=0; i < boxes.size(); i++)
    {
      TS_ASSERT_EQUALS( boxes[i]->getNPoints(), 1);
      TS_ASSERT_EQUALS( boxes[i]->getSignal(), 2.0);
      TS_ASSERT_EQUALS( boxes[i]->getErrorSquared(), 2.0);
    }

    // Now try to add bad events (outside bounds)
    events.clear();
    for (double x=-5.0; x < 20; x += 20.0)
      for (double y=-5.0; y < 20; y += 20.0)
      {
        double centers[2] = {x,y};
        events.push_back( MDEvent<2>(2.0, 2.0, centers) );
      }
    // Get the right totals again
    b->refreshCache();
    // All 4 points get rejected
    TS_ASSERT_THROWS_NOTHING( numbad = b->addEvents( events ); );
    TS_ASSERT_EQUALS( numbad, 4);
    // Number of points and signal is unchanged.
    TS_ASSERT_EQUALS( b->getNPoints(), 100);
    TS_ASSERT_EQUALS( b->getSignal(), 100*2.0);
    TS_ASSERT_EQUALS( b->getErrorSquared(), 100*2.0);
  }


  //-------------------------------------------------------------------------------------
  /** Tests add_events with limits into the vectorthat bad events are thrown out when using addEvents.
   * */
  void test_addEvents_start_stop()
  {
    MDGridBox<MDEvent<2>,2> * b = makeMDGridBox<2>(0);
    std::vector< MDEvent<2> > events;

    // Make an event in the middle of each box
    for (double x=0.5; x < 10; x += 1.0)
      for (double y=0.5; y < 10; y += 1.0)
      {
        double centers[2] = {x,y};
        events.push_back( MDEvent<2>(2.0, 2.0, centers) );
      }

    size_t numbad;
    TS_ASSERT_THROWS_NOTHING( numbad = b->addEvents( events, 50, 60 ); );
    // Get the right totals again
    b->refreshCache();
    TS_ASSERT_EQUALS( numbad, 0);
    TS_ASSERT_EQUALS( b->getNPoints(), 10);
    TS_ASSERT_EQUALS( b->getSignal(), 10*2.0);
    TS_ASSERT_EQUALS( b->getErrorSquared(), 10*2.0);
  }

  //-------------------------------------------------------------------------------------
  /** Test that adding events (as vectors) in parallel does not cause
   * segfaults or incorrect totals.
   * */
  void test_addEvents_inParallel()
  {
    MDGridBox<MDEvent<2>,2> * b = makeMDGridBox<2>(0);
    int num_repeat = 1000;

    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i=0; i < num_repeat; i++)
    {
      std::vector< MDEvent<2> > events;
      // Make an event in the middle of each box
      for (double x=0.5; x < 10; x += 1.0)
        for (double y=0.5; y < 10; y += 1.0)
        {
          double centers[2] = {x,y};
          events.push_back( MDEvent<2>(2.0, 2.0, centers) );
        }
      TS_ASSERT_THROWS_NOTHING( b->addEvents( events ); );
    }
    // Get the right totals again
    b->refreshCache();
    TS_ASSERT_EQUALS( b->getNPoints(), 100*num_repeat);
    TS_ASSERT_EQUALS( b->getSignal(), 100*num_repeat*2.0);
    TS_ASSERT_EQUALS( b->getErrorSquared(), 100*num_repeat*2.0);
  }


  //-------------------------------------------------------------------------------------
  /** Test the routine that auto-splits MDBoxes into MDGridBoxes recursively.
   * It tests the max_depth of splitting too, because there are numerous
   * repeated events at exactly the same position = impossible to separate further.
   * */
  void test_splitAllIfNeeded()
  {
    typedef MDGridBox<MDEvent<2>,2> gbox_t;
    typedef MDBox<MDEvent<2>,2> box_t;
    typedef IMDBox<MDEvent<2>,2> ibox_t;

    gbox_t * b = makeMDGridBox<2>(0);
    b->getBoxController()->m_SplitThreshold = 100;
    b->getBoxController()->m_maxDepth = 4;

    // Make a 1000 events at exactly the same point
    size_t num_repeat = 1000;
    std::vector< MDEvent<2> > events;
    for (size_t i=0; i < num_repeat; i++)
    {
      // Make an event in the middle of each box
      double centers[2] = {1e-10, 1e-10};
      events.push_back( MDEvent<2>(2.0, 2.0, centers) );
    }
    TS_ASSERT_THROWS_NOTHING( b->addEvents( events ); );


    // Split into sub-grid boxes
    TS_ASSERT_THROWS_NOTHING( b->splitAllIfNeeded(NULL); )

    // Dig recursively into the gridded box hierarchies
    std::vector<ibox_t*> boxes;
    size_t expected_depth = 0;
    while (b)
    {
      expected_depth++;
      boxes = b->getBoxes();

      // Get the 0th box
      b = dynamic_cast<gbox_t*>(boxes[0]);

      // The 0-th box is a MDGridBox (it was split)
      // (though it is normal for b to be a MDBox when you reach the max depth)
      if (expected_depth < 4)
      { TS_ASSERT( b ) }

      // The 0-th box has all the points
      TS_ASSERT_EQUALS( boxes[0]->getNPoints(), num_repeat);
      // The 0-th box is at the expected_depth
      TS_ASSERT_EQUALS( boxes[0]->getDepth(), expected_depth);

      // The other boxes have nothing
      TS_ASSERT_EQUALS( boxes[1]->getNPoints(), 0);
      // The other box is a MDBox (it was not split)
      TS_ASSERT( dynamic_cast<box_t*>(boxes[1]) )
    }

    // We went this many levels (and no further) because recursion depth is limited
    TS_ASSERT_EQUALS(boxes[0]->getDepth(), 4);
  }


  //------------------------------------------------------------------------------------------------
  /** This test splits a large number of events, and uses a ThreadPool
   * to use all cores.
   */
  void test_splitAllIfNeeded_usingThreadPool()
  {
    typedef MDGridBox<MDEvent<2>,2> gbox_t;
    typedef MDBox<MDEvent<2>,2> box_t;
    typedef IMDBox<MDEvent<2>,2> ibox_t;

    gbox_t * b = makeMDGridBox<2>(0);
    b->getBoxController()->m_SplitThreshold = 100;
    b->getBoxController()->m_maxDepth = 4;

    // Make a 1000 events in each sub-box
    size_t num_repeat = 1000;
    if (DODEBUG) num_repeat = 2000;

    Timer tim;
    if (DODEBUG) std::cout << "Adding " << num_repeat*100 << " events...\n";

    std::vector< MDEvent<2> > events;
    for (double x=0.5; x < 10; x += 1.0)
      for (double y=0.5; y < 10; y += 1.0)
      {
        double centers[2] = {x,y};
        for (size_t i=0; i < num_repeat; i++)
        {
          // Make an event in the middle of each box
          events.push_back( MDEvent<2>(2.0, 2.0, centers) );
        }
      }
    TS_ASSERT_THROWS_NOTHING( b->addEvents( events ); );
    if (DODEBUG) std::cout << "Adding events done in " << tim.elapsed() << "!\n";

    // Split those boxes in parallel.
    ThreadSchedulerFIFO * ts = new ThreadSchedulerFIFO();
    ThreadPool tp(ts);
    b->splitAllIfNeeded(ts);
    tp.joinAll();

    if (DODEBUG) std::cout << "Splitting events done in " << tim.elapsed() << " sec.\n";

    // Now check the results. Each sub-box should be MDGridBox and have that many events
    std::vector<ibox_t*> boxes = b->getBoxes();
    TS_ASSERT_EQUALS(boxes.size(), 100);
    for (size_t i=0; i<boxes.size(); i++)
    {
      TS_ASSERT_EQUALS( boxes[i]->getNPoints(), num_repeat );
      TS_ASSERT( dynamic_cast<gbox_t *>(boxes[i]) );
    }

  }


  //------------------------------------------------------------------------------------------------
  /** Helper to make a 2D MDBin */
  MDBin<MDEvent<2>,2> makeMDBin2(double minX, double maxX, double minY, double maxY)
  {
    MDBin<MDEvent<2>,2> bin;
    bin.m_min[0] = minX;
    bin.m_max[0] = maxX;
    bin.m_min[1] = minY;
    bin.m_max[1] = maxY;
    return bin;
  }

  /** Helper to test the binning of a 2D bin */
  void doTestMDBin2(MDGridBox<MDEvent<2>,2> * b,
      const std::string & message,
      double minX, double maxX, double minY, double maxY, double expectedSignal)
  {
//    std::cout << "Bins: X " << std::setw(5) << minX << " to "<< std::setw(5)  << maxX << ", Y " << std::setw(5) << minY << " to "<< std::setw(5)  << maxY      << ". " << message << std::endl;

    MDBin<MDEvent<2>,2> bin;
    bin = makeMDBin2(minX, maxX, minY, maxY);
    b->centerpointBin(bin);
    TSM_ASSERT_DELTA( message, bin.m_signal, expectedSignal, 1e-5);
  }

  //------------------------------------------------------------------------------------------------
  /** Test binning in orthogonal axes */
  void test_centerpointBin()
  {
    typedef MDGridBox<MDEvent<2>,2> gbox_t;
    typedef MDBox<MDEvent<2>,2> box_t;
    typedef IMDBox<MDEvent<2>,2> ibox_t;

    // 10x10 bins, 2 events per bin, each weight of 2.0
    gbox_t * b = makeMDGridBox<2>(2);
    TS_ASSERT_DELTA( b->getSignal(), 400.0, 1e-5);

    MDBin<MDEvent<2>,2> bin;

    doTestMDBin2(b, "Bin that is completely off",
        10.1, 11.2, 1.9, 3.12,   0.0);

    doTestMDBin2(b, "Bin that is completely off (2)",
        2, 3, -0.6, -0.1,   0.0);

    doTestMDBin2(b, "Bin that holds one entire MDBox (bigger than it)",
        0.8, 2.2, 1.9, 3.12,     4.0);

    doTestMDBin2(b, "Bin that holds one entire MDBox (going off one edge)",
        -0.2, 1.2, 1.9, 3.12,     4.0);

    doTestMDBin2(b, "Bin that holds one entire MDBox (going off the other edge)",
        8.9, 10.2, 1.9, 3.12,     4.0);

    doTestMDBin2(b, "Bin that holds one entire MDBox (going off both edge)",
        -0.2, 1.2, -0.2, 1.2,     4.0);

    doTestMDBin2(b, "Bin that holds one entire MDBox and a fraction of at least one more with something",
        0.8, 2.7, 1.9, 3.12,     8.0);

    doTestMDBin2(b, "Bin that holds four entire MDBoxes",
        0.8, 3.1, 0.9, 3.2,     16.0);

    doTestMDBin2(b, "Bin goes off two edges in one direction",
        -0.3, 10.2, 1.9, 3.1,   10*4.0);

    doTestMDBin2(b, "Bin that fits all within a single MDBox, and contains the center",
        0.2, 0.8, 0.2, 0.8,     4.0);

    doTestMDBin2(b, "Bin that fits all within a single MDBox, and DOES NOT contain anything",
        0.2, 0.3, 0.1, 0.2,     0.0);

    doTestMDBin2(b, "Bin that fits partially in two MDBox'es, and DOES NOT contain anything",
        0.8, 1.2, 0.1, 0.2,     0.0);

    doTestMDBin2(b, "Bin that fits partially in two MDBox'es, and contains the centers",
        0.2, 1.8, 0.1, 0.9,     8.0);

    doTestMDBin2(b, "Bin that fits partially in one MDBox'es, and goes of the edge",
        -3.2, 0.8, 0.1, 0.9,     4.0);

  }

private:
  std::string message;
};


#endif

