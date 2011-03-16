#ifndef MDGRIDBOXTEST_H
#define MDGRIDBOXTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDGridBox.h"
#include "MantidMDEvents/BoxController.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <map>

using namespace Mantid;
using namespace Mantid::MDEvents;

class MDGridBoxTest :    public CxxTest::TestSuite
{

public:

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
  /** Generate an empty MDBox with nd dimensions, splitting in 10x10 boxes */
  template<size_t nd>
  MDGridBox<MDEvent<nd>,nd> * makeMDGridBox()
  {
    // Split at 5 events
    BoxController_sptr splitter(new BoxController(nd));
    splitter->setSplitThreshold(5);
    // Splits into 10x10x.. boxes
    splitter->setSplitInto(10);
    // Set the size to 10.0 in all directions
    MDBox<MDEvent<nd>,nd> * out = new MDBox<MDEvent<nd>,nd>(splitter);
    for (size_t d=0; d<nd; d++)
      out->setExtents(d, 0.0, 10.0);
    return new MDGridBox<MDEvent<nd>,nd>(out) ;
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


  //-------------------------------------------------------------------------------------
  void test_MDBoxConstructor()
  {
    MDBox<MDEvent<1>,1> * b = makeMDBox1();
    TS_ASSERT_EQUALS( b->getNumDims(), 1);
    TS_ASSERT_EQUALS( b->getNPoints(), 0);
    TS_ASSERT( b->willSplit(10) );
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
    MDGridBox<MDEvent<2>,2> * superbox = makeMDGridBox<2>();
    MDGridBox<MDEvent<2>,2> * gb;
    MDBox<MDEvent<2>,2> * b;

    std::vector<IMDBox<MDEvent<2>,2>*> boxes;

    // The box is a MDBox at first
    boxes = superbox->getBoxes();
    b = dynamic_cast<MDBox<MDEvent<2>,2> *>(boxes[0]);
    TS_ASSERT( b );

    TS_ASSERT_THROWS_NOTHING(superbox->splitContents(0));

    // Now, it has turned into a GridBox
    boxes = superbox->getBoxes();
    gb = dynamic_cast<MDGridBox<MDEvent<2>,2> *>(boxes[0]);
    TS_ASSERT( gb );

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
    MDGridBox<MDEvent<2>,2> * superbox = makeMDGridBox<2>();
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
  /** Fill a 10x10 gridbox with events*/
  void test_addEvents_2D()
  {
    MDGridBox<MDEvent<2>,2> * b = makeMDGridBox<2>();
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
    // All 4 points get rejected
    TS_ASSERT_THROWS_NOTHING( numbad = b->addEvents( events ); );
    TS_ASSERT_EQUALS( numbad, 4);
    // Number of points and signal is unchanged.
    TS_ASSERT_EQUALS( b->getNPoints(), 100);
    TS_ASSERT_EQUALS( b->getSignal(), 100*2.0);
    TS_ASSERT_EQUALS( b->getErrorSquared(), 100*2.0);
  }



  //-------------------------------------------------------------------------------------
  /** Fill a 10x10 gridbox with events.
   * Put enough events that they will need to recursively split
   * into 10x10 boxes inside the 10x10 boxes (total of 10000 boxes with 1 event each).
   * */
  void xtest_addEvents_2D_recursive_splitting()
  {
    MDGridBox<MDEvent<2>,2> * b = makeMDGridBox<2>();

    std::vector< MDEvent<2> > events;

    // Make an event in the middle of each box
    for (double x=0.05; x < 10; x += 0.1)
      for (double y=0.05; y < 10; y += 0.1)
      {
        double centers[2] = {x,y};
        events.push_back( MDEvent<2>(2.0, 2.0, centers) );
      }

    size_t numbad;
    TS_ASSERT_THROWS_NOTHING( numbad = b->addEvents( events ); );
    TS_ASSERT_EQUALS( numbad, 0);
    TS_ASSERT_EQUALS( b->getNPoints(), 10000);
    TS_ASSERT_EQUALS( b->getSignal(), 10000*2.0);
    TS_ASSERT_EQUALS( b->getErrorSquared(), 10000*2.0);

    // Get all the boxes contained
    std::vector<IMDBox<MDEvent<2>,2>*> boxes = b->getBoxes();
    TS_ASSERT_EQUALS( boxes.size(), 100);
    for (size_t i=0; i < boxes.size(); i++)
    {
      TS_ASSERT_EQUALS( boxes[i]->getNPoints(), 100);
      TS_ASSERT_EQUALS( boxes[i]->getSignal(), 100*2.0);
      TS_ASSERT_EQUALS( boxes[i]->getErrorSquared(), 100*2.0);

      // --- Now look recursively -----
      MDGridBox<MDEvent<2>,2> * b2 = dynamic_cast<MDGridBox<MDEvent<2>,2> *>(boxes[i]);
      TSM_ASSERT("Box has not split into MDGridBox", b2);
      if (b2)
      {
        std::vector<IMDBox<MDEvent<2>,2>*> boxes2 = b2->getBoxes();
        TS_ASSERT_EQUALS( boxes2.size(), 100);
        for (size_t i=0; i < boxes2.size(); i++)
        {
          TS_ASSERT_EQUALS( boxes[i]->getNPoints(), 1);
          TS_ASSERT_EQUALS( boxes[i]->getSignal(), 2.0);
          TS_ASSERT_EQUALS( boxes[i]->getErrorSquared(), 2.0);
        }
      }
      else
        break;
    }
  }


};


#endif

