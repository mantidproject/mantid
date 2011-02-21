#ifndef MDGRIDBOXTEST_H
#define MDGRIDBOXTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDGridBox.h"
#include "MantidMDEvents/BoxSplitController.h"
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
    BoxSplitController_sptr splitter(new BoxSplitController(5));
    // Splits into 10 boxes
    splitter->setSplitInto(10);
    // Set the size
    MDBox<MDEvent<1>,1> * out = new MDBox<MDEvent<1>,1>(splitter);
    out->setExtents(0, 0.0, 10.0);
    return out;
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

    // Check the boxes
    std::vector<IMDBox<MDEvent<1>,1> *> boxes = g->getBoxes();
    TS_ASSERT_EQUALS( boxes.size(), 10.0);
    for (size_t i=0; i<10; i++)
    {
      MDBox<MDEvent<1>,1> * box = dynamic_cast<MDBox<MDEvent<1>,1> *>(boxes[i]);
      TS_ASSERT_DELTA(box->getExtents(0).min, i*1.0, 1e-6);
      TS_ASSERT_DELTA(box->getExtents(0).max, (i+1)*1.0, 1e-6);
      // Look at the single event in there
      TS_ASSERT_EQUALS(box->getNPoints(), 1);
      MDEvent<1> ev = box->getEvents()[0];
      TS_ASSERT_DELTA(ev.getCenter(0), i*1.0 + 0.5, 1e-5);
    }

  }

};


#endif

