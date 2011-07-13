#ifndef MDEVENTWORKSPACETEST_H
#define MDEVENTWORKSPACETEST_H

#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/ProgressText.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/BoxController.h"
#include "MantidMDEvents/CoordTransformDistance.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDGridBox.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <boost/random/linear_congruential.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <cxxtest/TestSuite.h>
#include <map>
#include <memory>
#include <vector>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::Geometry;

class MDEventWorkspaceTest :    public CxxTest::TestSuite
{
public:
  bool DODEBUG;
  MDEventWorkspaceTest()
  {
    DODEBUG = false;
  }


  void test_Constructor()
  {
    MDEventWorkspace<MDEvent<3>, 3> ew3;
    TS_ASSERT_EQUALS( ew3.getNumDims(), 3);
    TS_ASSERT_EQUALS( ew3.getNPoints(), 0);
    TS_ASSERT_EQUALS( ew3.id(), "MDEventWorkspace<MDEvent,3>");
    // Box controller MUST always be present
    TS_ASSERT(ew3.getBoxController() );
    TS_ASSERT(ew3.getBox());
    TS_ASSERT(ew3.getBox()->getBoxController());
    TS_ASSERT_EQUALS(ew3.getBox()->getId(), 0);
  }

  void test_Constructor_IMDEventWorkspace()
  {
    IMDEventWorkspace * ew3 = new MDEventWorkspace<MDEvent<3>, 3>();
    TS_ASSERT_EQUALS( ew3->getNumDims(), 3);
    TS_ASSERT_EQUALS( ew3->getNPoints(), 0);
    delete ew3;
  }

  void test_initialize_throws()
  {
    IMDEventWorkspace * ew = new MDEventWorkspace<MDEvent<3>, 3>();
    TS_ASSERT_THROWS( ew->initialize(), std::runtime_error);
    for (size_t i=0; i<5; i++)
      ew->addDimension( MDHistoDimension_sptr(new MDHistoDimension("x","x","m",-1,1,0)) );
    TS_ASSERT_THROWS( ew->initialize(), std::runtime_error);
    delete ew;
  }

  void test_initialize()
  {
    IMDEventWorkspace * ew = new MDEventWorkspace<MDEvent<3>, 3>();
    TS_ASSERT_THROWS( ew->initialize(), std::runtime_error);
    for (size_t i=0; i<3; i++)
      ew->addDimension( MDHistoDimension_sptr(new MDHistoDimension("x","x","m",-1,1,0)) );
    TS_ASSERT_THROWS_NOTHING( ew->initialize() );
    delete ew;
  }


  void test_splitBox()
  {
    MDEventWorkspace3 * ew = new MDEventWorkspace3();
    BoxController_sptr bc = ew->getBoxController();
    bc->setSplitInto(4);
    TS_ASSERT( !ew->isGridBox() );
    TS_ASSERT_THROWS_NOTHING( ew->splitBox(); )
    TS_ASSERT( ew->isGridBox() );
    delete ew;
  }

  /** Adding dimension info and searching for it back */
  void test_addDimension_getDimension()
  {
    MDEventWorkspace2 * ew = new MDEventWorkspace2();
    MDHistoDimension_sptr dim(new MDHistoDimension("Qx", "Qx", "Ang", -1, +1, 0));
    TS_ASSERT_THROWS_NOTHING( ew->addDimension(dim); )
    MDHistoDimension_sptr dim2(new MDHistoDimension("Qy", "Qy", "Ang", -1, +1, 0));
    TS_ASSERT_THROWS_NOTHING( ew->addDimension(dim2); )
    TS_ASSERT_EQUALS( ew->getNumDims(), 2);
    TS_ASSERT_EQUALS( ew->getDimension(0)->getName(), "Qx");
    TS_ASSERT_EQUALS( ew->getDimension(1)->getName(), "Qy");
    TS_ASSERT_EQUALS( ew->getDimensionIndexByName("Qx"), 0);
    TS_ASSERT_EQUALS( ew->getDimensionIndexByName("Qy"), 1);
    TS_ASSERT_THROWS_ANYTHING( ew->getDimensionIndexByName("IDontExist"));
  }


  //-------------------------------------------------------------------------------------
  /** Fill a 10x10 gridbox with events
   *
   * Tests that bad events are thrown out when using addEvents.
   * */
  void test_addManyEvents()
  {
    ProgressText * prog = NULL;
    if (DODEBUG) prog = new ProgressText(0.0, 1.0, 10, false);

    typedef MDGridBox<MDEvent<2>,2> box_t;
    MDEventWorkspace2::sptr b = MDEventsTestHelper::makeMDEW<2>(10, 0.0, 10.0);
    box_t * subbox;

    // Manually set some of the tasking parameters
    b->getBoxController()->setAddingEvents_eventsPerTask(1000);
    b->getBoxController()->setAddingEvents_numTasksPerBlock(20);
    b->getBoxController()->setSplitThreshold(100);
    b->getBoxController()->setMaxDepth(4);

    std::vector< MDEvent<2> > events;
    size_t num_repeat = 1000;
    // Make an event in the middle of each box
    for (double x=0.0005; x < 10; x += 1.0)
      for (double y=0.0005; y < 10; y += 1.0)
      {
        for (size_t i=0; i < num_repeat; i++)
        {
          coord_t centers[2] = {x, y};
          events.push_back( MDEvent<2>(2.0, 2.0, centers) );
        }
      }
    TS_ASSERT_EQUALS( events.size(), 100*num_repeat);

    TS_ASSERT_THROWS_NOTHING( b->addManyEvents( events, prog ); );
    TS_ASSERT_EQUALS( b->getNPoints(), 100*num_repeat);
    TS_ASSERT_EQUALS( b->getBox()->getSignal(), 100*double(num_repeat)*2.0);
    TS_ASSERT_EQUALS( b->getBox()->getErrorSquared(), 100*double(num_repeat)*2.0);

    box_t * gridBox = dynamic_cast<box_t *>(b->getBox());
    std::vector<IMDBox<MDEvent<2>,2>*> boxes = gridBox->getBoxes();
    TS_ASSERT_EQUALS( boxes[0]->getNPoints(), num_repeat);
    // The box should have been split itself into a gridbox, because 1000 events > the split threshold.
    subbox = dynamic_cast<box_t *>(boxes[0]);
    TS_ASSERT( subbox ); if (!subbox) return;
    // The sub box is at a depth of 1.
    TS_ASSERT_EQUALS( subbox->getDepth(), 1);

    // And you can keep recursing into the box.
    boxes = subbox->getBoxes();
    subbox = dynamic_cast<box_t *>(boxes[0]);
    TS_ASSERT( subbox ); if (!subbox) return;
    TS_ASSERT_EQUALS( subbox->getDepth(), 2);

    // And so on (this type of recursion was checked in test_splitAllIfNeeded()
    if (prog) delete prog;
  }


  void checkExtents( std::vector<Mantid::Geometry::MDDimensionExtents> & ext, coord_t xmin, coord_t xmax, coord_t ymin, coord_t ymax)
  {
    TS_ASSERT_DELTA( ext[0].min, xmin, 1e-4);
    TS_ASSERT_DELTA( ext[0].max, xmax, 1e-4);
    TS_ASSERT_DELTA( ext[1].min, ymin, 1e-4);
    TS_ASSERT_DELTA( ext[1].max, ymax, 1e-4);
  }

  void addEvent(MDEventWorkspace2::sptr b, coord_t x, coord_t y)
  {
    coord_t centers[2] = {x, y};
    b->addEvent(MDEvent<2>(2.0, 2.0, centers));
  }

  void test_getMinimumExtents()
  {
    MDEventWorkspace2::sptr ws = MDEventsTestHelper::makeMDEW<2>(10, 0.0, 10.0);
    std::vector<Mantid::Geometry::MDDimensionExtents> ext;

    // If nothing in the workspace, the extents given are invalid.
    ext = ws->getMinimumExtents(2);
    TS_ASSERT( ext[0].min > ext[0].max )

    std::vector< MDEvent<2> > events;
    // Make an event in the middle of each box
    for (double x=4.0005; x < 7; x += 1.0)
      for (double y=4.0005; y < 7; y += 1.0)
      {
        coord_t centers[2] = {x, y};
        events.push_back( MDEvent<2>(2.0, 2.0, centers) );
      }
    // So it doesn't split
    ws->getBoxController()->setSplitThreshold(1000);
    ws->addManyEvents( events, NULL );
    ws->refreshCache();

    // Base extents
    ext = ws->getMinimumExtents(2);
    checkExtents(ext, 4, 7,  4, 7);

    // Start adding events to make the extents bigger
    addEvent(ws, 3.5, 5.0);
    ext = ws->getMinimumExtents(2);
    checkExtents(ext, 3, 7,  4, 7);

    addEvent(ws, 8.5, 7.9);
    ext = ws->getMinimumExtents(2);
    checkExtents(ext, 3, 9,  4, 8);

    addEvent(ws, 0.5, 0.9);
    ext = ws->getMinimumExtents(2);
    checkExtents(ext, 0, 9,  0, 8);

  }

//
//  //-------------------------------------------------------------------------------------
//  /** Tests that bad events are thrown out when using addEvents.
//   * */
//  void test_addManyEvents_Performance()
//  {
//    // This test is too slow for unit tests, so it is disabled except in debug mode.
//    if (!DODEBUG) return;
//
//    ProgressText * prog = new ProgressText(0.0, 1.0, 10, true);
//    prog->setNotifyStep(0.5); //Notify more often
//
//    typedef MDGridBox<MDEvent<2>,2> box_t;
//    box_t * b = makeMDEW<2>(10, 0.0, 10.0);
//
//    // Manually set some of the tasking parameters
//    b->getBoxController()->m_addingEvents_eventsPerTask = 50000;
//    b->getBoxController()->m_addingEvents_numTasksPerBlock = 50;
//    b->getBoxController()->m_SplitThreshold = 1000;
//    b->getBoxController()->m_maxDepth = 6;
//
//    Timer tim;
//    std::vector< MDEvent<2> > events;
//    double step_size = 1e-3;
//    size_t numPoints = (10.0/step_size)*(10.0/step_size);
//    std::cout << "Starting to write out " << numPoints << " events\n";
//    if (true)
//    {
//      // ------ Make an event in the middle of each box ------
//      for (double x=step_size; x < 10; x += step_size)
//        for (double y=step_size; y < 10; y += step_size)
//        {
//          double centers[2] = {x, y};
//          events.push_back( MDEvent<2>(2.0, 3.0, centers) );
//        }
//    }
//    else
//    {
//      // ------- Randomize event distribution ----------
//      boost::mt19937 rng;
//      boost::uniform_real<float> u(0.0, 10.0); // Range
//      boost::variate_generator<boost::mt19937&, boost::uniform_real<float> > gen(rng, u);
//
//      for (size_t i=0; i < numPoints; i++)
//      {
//        double centers[2] = {gen(), gen()};
//        events.push_back( MDEvent<2>(2.0, 3.0, centers) );
//      }
//    }
//    TS_ASSERT_EQUALS( events.size(), numPoints);
//    std::cout << "..." << numPoints << " events were filled in " << tim.elapsed() << " secs.\n";
//
//    size_t numbad = 0;
//    TS_ASSERT_THROWS_NOTHING( numbad = b->addManyEvents( events, prog); );
//    TS_ASSERT_EQUALS( numbad, 0);
//    TS_ASSERT_EQUALS( b->getNPoints(), numPoints);
//    TS_ASSERT_EQUALS( b->getSignal(), numPoints*2.0);
//    TS_ASSERT_EQUALS( b->getErrorSquared(), numPoints*3.0);
//
//    std::cout << "addManyEvents() ran in " << tim.elapsed() << " secs.\n";
//  }


  void test_integrateSphere()
  {
    // 10x10x10 eventWorkspace
    MDEventWorkspace3::sptr ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 1 /*event per box*/);
    TS_ASSERT_EQUALS( ws->getNPoints(), 1000);

    // The sphere transformation
    coord_t center[3] = {0,0,0};
    bool dimensionsUsed[3] = {true,true,true};
    CoordTransformDistance sphere(3, center, dimensionsUsed);

    signal_t signal = 0;
    signal_t errorSquared = 0;
    ws->getBox()->integrateSphere(sphere, 1.0, signal, errorSquared);

    //TODO:
//    TS_ASSERT_DELTA( signal, 1.0, 1e-5);
//    TS_ASSERT_DELTA( errorSquared, 1.0, 1e-5);


  }

};

#endif
