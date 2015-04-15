#ifndef MDGRIDBOXTEST_H
#define MDGRIDBOXTEST_H

#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/Memory.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/ProgressText.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/Utils.h"
#include "MantidAPI/BoxController.h"
#include "MantidDataObjects/CoordTransformDistance.h"
#include "MantidDataObjects/MDBox.h"
#include "MantidDataObjects/MDLeanEvent.h"
#include "MantidDataObjects/MDGridBox.h"
#include <nexus/NeXusFile.hpp>
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MDBoxTest.h"
#include <boost/random/linear_congruential.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <map>
#include <memory>
#include <Poco/File.h>
#include <vector>
#include <gmock/gmock.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace testing;

class MDGridBoxTest :    public CxxTest::TestSuite
{
private:

  ///Mock type to help determine if masking is being determined correctly
    class MockMDBox : public MDBox<MDLeanEvent<1>, 1>
  {
      API::BoxController *const pBC;
  public:
      MockMDBox():
          MDBox<MDLeanEvent<1>, 1>(new API::BoxController(1)),
          pBC(MDBox<MDLeanEvent<1>, 1>::getBoxController())
      {}
      MOCK_CONST_METHOD0(getIsMasked, bool());
      MOCK_METHOD0(mask, void());
      MOCK_METHOD0(unmask, void());
      ~MockMDBox()
      {delete pBC;}
  };

  // the sp to a box controller used as general reference to all tested classes/operations including MockMDBox
   BoxController_sptr gbc;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDGridBoxTest *createSuite() { return new MDGridBoxTest(); }
  static void destroySuite( MDGridBoxTest *suite ) { delete suite; }

  bool DODEBUG;
  MDGridBoxTest()
  {
    gbc =  BoxController_sptr(new BoxController(1));
    DODEBUG = false;
  }

  //-------------------------------------------------------------------------------------
  void test_MDBoxConstructor()
  {

    MDBox<MDLeanEvent<1>,1> * b = MDEventsTestHelper::makeMDBox1(10);
    TS_ASSERT_EQUALS( b->getNumDims(), 1);
    TS_ASSERT_EQUALS( b->getNPoints(), 0);
    TS_ASSERT_DELTA( b->getExtents(0).getMin(), 0.0,  1e-5);
    TS_ASSERT_DELTA( b->getExtents(0).getMax(), 10.0, 1e-5);
    TS_ASSERT_DELTA( b->getVolume(), 10.0, 1e-5);
    // Start at ID 0.
    TS_ASSERT_EQUALS( b->getID(), 0);

    BoxController *const bcc = b->getBoxController();
    delete b;
    if(DODEBUG)
    {
        std::cout << sizeof( MDLeanEvent<3>) << " bytes per MDLeanEvent(3)" << std::endl;
        std::cout << sizeof( MDLeanEvent<4>) << " bytes per MDLeanEvent(4)" << std::endl;
        std::cout << sizeof( Mantid::Kernel::Mutex ) << " bytes per Mutex" << std::endl;
        std::cout << sizeof( MDDimensionExtents<coord_t>) << " bytes per MDDimensionExtents" << std::endl;
        std::cout << sizeof( MDBox<MDLeanEvent<3>,3>) << " bytes per MDBox(3)" << std::endl;
        std::cout << sizeof( MDBox<MDLeanEvent<4>,4> ) << " bytes per MDBox(4)" << std::endl;
        std::cout << sizeof( MDGridBox<MDLeanEvent<3>,3>) << " bytes per MDGridBox(3)" << std::endl;
        std::cout << sizeof( MDGridBox<MDLeanEvent<4>,4> ) << " bytes per MDGridBox(4)" << std::endl;

        MemoryStats mem;
        size_t start = mem.availMem();
        std::cout << start << " KB before" << std::endl;
        CPUTimer tim;
        for (size_t i=0; i<1000000; i++)
        {
          MDBox<MDLeanEvent<3>,3> * box = new MDBox<MDLeanEvent<3>,3>(bcc);
          (void) box;
        }
        std::cout << tim << " to allocate a million boxes" << std::endl;
        mem.update();
        size_t stop = mem.availMem();
        std::cout << stop << " KB after " << std::endl;
        std::cout << start-stop << " KB change " << std::endl;
        std::cout << (start-stop)*1024 / sizeof( MDBox<MDLeanEvent<3>,3>) << " times the sizeof MDBox3" << std::endl;
        delete bcc;
    }
    else
       delete bcc;
  }


  void check_MDGridBox(MDGridBox<MDLeanEvent<1>,1> * g)
  {
    // The grid box stole the ID of the box it replaces.
    TS_ASSERT_EQUALS( g->getID(), 0);

    // Look overall; it has 10 points
    TS_ASSERT_EQUALS(g->getNumDims(), 1);
    TS_ASSERT_EQUALS(g->getNPoints(), 10);
    // Its depth level should be 0 (same as parent)
    TS_ASSERT_EQUALS(g->getDepth(), 0);
    // It was split into 10 MDBoxes.
    TS_ASSERT_EQUALS(g->getNumMDBoxes(), 10);
    // Same result for non-recursive children
    TS_ASSERT_EQUALS(g->getNumChildren(), 10);
    // The volume was set correctly
    TS_ASSERT_DELTA(g->getVolume(), 10.0, 1e-5);

    // It has a BoxController
    TS_ASSERT( g->getBoxController() );

    // Check the boxes
    std::vector<MDBoxBase<MDLeanEvent<1>,1> *> &boxes = g->getBoxes();
    TS_ASSERT_EQUALS( boxes.size(), 10);
    for (size_t i=0; i<boxes.size(); i++)
    {
      // The get child method is equivalent
      TS_ASSERT_EQUALS( boxes[i], g->getChild(i));
      MDBox<MDLeanEvent<1>,1> * box = dynamic_cast<MDBox<MDLeanEvent<1>,1> *>(boxes[i]);

      // Sequential ID, starting at 1 since 0 was used by the parent.
      TS_ASSERT_EQUALS( box->getID(), i+1);
      // At the right place?
      TS_ASSERT_DELTA(box->getExtents(0).getMin(), double(i)*1.0, 1e-6);
      TS_ASSERT_DELTA(box->getExtents(0).getMax(), double(i+1)*1.0, 1e-6);
      // Look at the single event in there
      TS_ASSERT_EQUALS(box->getNPoints(), 1);
      MDLeanEvent<1> ev = box->getEvents()[0];
      TS_ASSERT_DELTA(ev.getCenter(0), double(i)*1.0 + 0.5, 1e-5);
      // Its depth level should be 1 (deeper than parent)
      TS_ASSERT_EQUALS(box->getDepth(), 1);
      // The volume was set correctly
      TS_ASSERT_DELTA(box->getVolume(), 1.0, 1e-5);
      // The parent of the MDBox is the grid box
      TS_ASSERT_EQUALS(box->getParent(), g);
    }

  }



  //-------------------------------------------------------------------------------------
  void test_MDGridBox_constructor_from_MDBox()
  {
    MDBox<MDLeanEvent<1>,1> * b = MDEventsTestHelper::makeMDBox1();
    TS_ASSERT(b->getBoxController());
    // Start at ID 0.
    TS_ASSERT_EQUALS( b->getID(), 0);
    // Give it 10 events
    const std::vector<MDLeanEvent<1> > events = MDEventsTestHelper::makeMDEvents1(10);
    b->addEvents( events );
    TS_ASSERT_EQUALS( b->getNPoints(), 10 );
    TS_ASSERT_DELTA(b->getVolume(), 10.0, 1e-5);

    // Build the grid box out of it
    MDGridBox<MDLeanEvent<1>,1> * g = new MDGridBox<MDLeanEvent<1>,1>(b);

    // Perform a detailed check
    check_MDGridBox(g);

    // Now we add 10 more events
    //auto events = MDEventsTestHelper::makeMDEvents1(10);
    TSM_ASSERT_EQUALS("No bad events ",0,g->addEvents( events  ));

    // And now there should be 2 events per box
    std::vector<MDBoxBase<MDLeanEvent<1>,1> *> boxes = g->getBoxes();
    for (size_t i=0; i<10; i++)
    {
      MDBox<MDLeanEvent<1>,1> * box = dynamic_cast<MDBox<MDLeanEvent<1>,1> *>(boxes[i]);
      TS_ASSERT_EQUALS(box->getNPoints(), 2);
    }

    std::vector<signal_t> sigErr(20);
    std::vector<coord_t> coord(10);
    std::vector<uint16_t> runIndex;
    std::vector<uint32_t> detID;

    for(size_t i=0;i<10;i++)
    {
        sigErr[2*i]=events[i].getSignal();
        sigErr[2*i+1]=events[i].getErrorSquared();
        coord[i] = events[i].getCenter(0);
    }

    g->buildAndAddEvents(sigErr,coord,runIndex,detID);

    for (size_t i=0; i<10; i++)
    {
      MDBox<MDLeanEvent<1>,1> * box = dynamic_cast<MDBox<MDLeanEvent<1>,1> *>(boxes[i]);
      TS_ASSERT_EQUALS(box->getNPoints(), 3);
    }



    BoxController *const bcc = b->getBoxController();
    delete b;
    delete bcc;
    delete g;
  }


  //-------------------------------------------------------------------------------------
  void test_MDGridBox_copy_constructor()
  {
    MDBox<MDLeanEvent<1>,1> * b = MDEventsTestHelper::makeMDBox1(10);
    TS_ASSERT_EQUALS( b->getID(), 0);
    const std::vector<MDLeanEvent<1> > events = MDEventsTestHelper::makeMDEvents1(10);
    b->addEvents( events );
    TS_ASSERT_EQUALS( b->getNPoints(), 10 );
    TS_ASSERT_DELTA(b->getVolume(), 10.0, 1e-5);

    // Build the grid box out of it
    MDGridBox<MDLeanEvent<1>,1> * g1 = new MDGridBox<MDLeanEvent<1>,1>(b);
    MDGridBox<MDLeanEvent<1>,1> * g2 = new MDGridBox<MDLeanEvent<1>,1>(*g1,g1->getBoxController());

    // Perform a detailed check
    check_MDGridBox(g2);

    BoxController *const bcc = b->getBoxController();
    delete bcc;
    delete g2;
    delete g1;
    delete b;
  }

  void test_setBoxController()
  {
    MDGridBox<MDLeanEvent<1>,1> * box = MDEventsTestHelper::makeMDGridBox<1>(10,10,0.0, 10.0);
    BoxController * originalBoxController = box->getBoxController();
    BoxController*const newBoxController = originalBoxController->clone();

    TS_ASSERT_DIFFERS(originalBoxController,newBoxController);

    MDGridBox<MDLeanEvent<1>,1> * box1 = new MDGridBox<MDLeanEvent<1>,1>(*box,newBoxController);

    auto boxes = box1->getBoxes();
    for(size_t i = 0; i < boxes.size(); ++i)
    {
      TSM_ASSERT_EQUALS("All child boxes should have the same box controller as the parent.", newBoxController, boxes[i]->getBoxController());
    }
    delete newBoxController;
    delete box1;
    delete originalBoxController;
    delete box;

  }


  //-----------------------------------------------------------------------------------------
  /** Manually setting children of a grid box (for NXS file loading) */
  void test_setChildren()
  {
    // Build the grid box
    MDGridBox<MDLeanEvent<1>,1> * g = MDEventsTestHelper::makeMDGridBox<1>(10,10,0.0, 10.0);
    // Clear the initial children
    for (size_t i = 0; i < g->getNumChildren(); i++) delete g->getChild(i);

    BoxController *const bcc = g->getBoxController();
    std::vector<API::IMDNode *> boxes;
    for (size_t i=0; i<15; i++)
      boxes.push_back( MDEventsTestHelper::makeMDBox1(10,bcc) );
    TS_ASSERT_THROWS_NOTHING( g->setChildren(boxes, 2, 12) );

    TS_ASSERT_EQUALS( g->getNumChildren(), 10);
    for (size_t i=2; i<12; i++)
    {
      TS_ASSERT_EQUALS( g->getChild(i-2), boxes[i]);
      // Parent was set correctly in child
      TS_ASSERT_EQUALS( g->getChild(i-2)->getParent(), g);
    }
    // MDGridBox will delete the children that it pulled in but the rest need to be
    // taken care of manually
    size_t indices[5] = {0, 1, 12, 13, 14};
    for(size_t i = 0; i < 5; ++i) delete boxes[indices[i]];
    delete g;
    delete bcc;
  }

  void test_getChildIndexFromID()
  {
    // Build the grid box
    MDGridBox<MDLeanEvent<1>,1> * g = MDEventsTestHelper::makeMDGridBox<1>(10,10,0.0, 10.0);
    TS_ASSERT_EQUALS(g->getChildIndexFromID( g->getChild(0)->getID() ), 0);
    TS_ASSERT_EQUALS(g->getChildIndexFromID( g->getChild(5)->getID() ), 5);
    TS_ASSERT_EQUALS(g->getChildIndexFromID(0), UNDEF_SIZET );
    TS_ASSERT_EQUALS(g->getChildIndexFromID(11), UNDEF_SIZET );
    BoxController *const bcc = g->getBoxController();
    delete g;
    delete bcc;

  }



  //-------------------------------------------------------------------------------------
  /** Build a 3D MDGridBox and check that the boxes created within are where you expect */
  void test_MDGridBox3()
  {
    MDBox<MDLeanEvent<3>,3> * b = MDEventsTestHelper::makeMDBox3();
    // Build the grid box out of it
    MDGridBox<MDLeanEvent<3>,3> * g = new MDGridBox<MDLeanEvent<3>,3>(b);
    TS_ASSERT_EQUALS(g->getNumDims(), 3);

    // Check the boxes
    std::vector<MDBoxBase<MDLeanEvent<3>,3> *> boxes = g->getBoxes();
    TS_ASSERT_EQUALS( boxes.size(), 10*5*2);
    for (size_t i=0; i<boxes.size(); i++)
    {
      MDBox<MDLeanEvent<3>,3> * box = dynamic_cast<MDBox<MDLeanEvent<3>,3> *>(boxes[i]);
      TS_ASSERT( box );
    }
    MDBox<MDLeanEvent<3>,3> * box;
    box = dynamic_cast<MDBox<MDLeanEvent<3>,3> *>(boxes[1]);
    MDEventsTestHelper::extents_match(box, 0, 1.0, 2.0);
    MDEventsTestHelper::extents_match(box, 1, 0.0, 2.0);
    MDEventsTestHelper::extents_match(box, 2, 0.0, 5.0);
    box = dynamic_cast<MDBox<MDLeanEvent<3>,3> *>(boxes[10]);
    MDEventsTestHelper::extents_match(box, 0, 0.0, 1.0);
    MDEventsTestHelper::extents_match(box, 1, 2.0, 4.0);
    MDEventsTestHelper::extents_match(box, 2, 0.0, 5.0);
    box = dynamic_cast<MDBox<MDLeanEvent<3>,3> *>(boxes[53]);
    MDEventsTestHelper::extents_match(box, 0, 3.0, 4.0);
    MDEventsTestHelper::extents_match(box, 1, 0.0, 2.0);
    MDEventsTestHelper::extents_match(box, 2, 5.0, 10.0);

    BoxController *const bcc =b->getBoxController();
    delete b;
    delete bcc;
    delete g;
  }


  //-------------------------------------------------------------------------------------
  /** Start with a grid box, split some of its contents into sub-gridded boxes. */
  void test_splitContents()
  {
    MDGridBox<MDLeanEvent<2>,2> * superbox = MDEventsTestHelper::makeMDGridBox<2>();
    MDGridBox<MDLeanEvent<2>,2> * gb;
    MDBox<MDLeanEvent<2>,2> * b;

    std::vector<MDBoxBase<MDLeanEvent<2>,2>*> boxes;

    // Start with 100 boxes
    TS_ASSERT_EQUALS( superbox->getNumMDBoxes(), 100);
    // And ID 0
    TS_ASSERT_EQUALS( superbox->getID(), 0 );

    // The box is a MDBox at first
    boxes = superbox->getBoxes();
    b = dynamic_cast<MDBox<MDLeanEvent<2>,2> *>(boxes[0]);
    TS_ASSERT( b );
    TS_ASSERT_DELTA( b->getVolume(), 1.0, 1e-5 );

    // It is the first child, so ID is 1
    TS_ASSERT_EQUALS( b->getID(), 1 );
    // There were 101 assigned IDs
    TS_ASSERT_EQUALS( b->getBoxController()->getMaxId(), 100+1);

    TS_ASSERT_THROWS_NOTHING(superbox->splitContents(0));

    // Now, it has turned into a GridBox
    boxes = superbox->getBoxes();
    gb = dynamic_cast<MDGridBox<MDLeanEvent<2>,2> *>(boxes[0]);
    TS_ASSERT( gb );
    TS_ASSERT_DELTA( gb->getVolume(), 1.0, 1e-5 );

    // ID of first child remains unchanged at 1
    TS_ASSERT_EQUALS( gb->getID(), 1 );
    // There were 101 assigned IDs
    TS_ASSERT_EQUALS( gb->getBoxController()->getMaxId(), 200+1);
    // The first child of the sub-divided box got 101 as its id
    TS_ASSERT_EQUALS( gb->getBoxes()[0]->getID(), 101 );

    // There are now 199 MDBoxes; the 99 at level 1, and 100 at level 2
    TS_ASSERT_EQUALS( superbox->getNumMDBoxes(), 199);

    // You can split it again and it does nothing
    TS_ASSERT_THROWS_NOTHING(superbox->splitContents(0));

    // Still a grid box
    boxes = superbox->getBoxes();
    gb = dynamic_cast<MDGridBox<MDLeanEvent<2>,2> *>(boxes[0]);
    TS_ASSERT( gb );

    BoxController *const bcc =superbox->getBoxController();
    delete superbox;
    delete bcc;

  }

  //-------------------------------------------------------------------------------------
  /** Adding a single event pushes it as deep as the current grid
   * hierarchy allows
   */
  void test_addEvent_with_recursive_gridding()
  {
    MDGridBox<MDLeanEvent<2>,2> * gb;
    MDBox<MDLeanEvent<2>,2> * b;
    std::vector<MDBoxBase<MDLeanEvent<2>,2>*> boxes;

    // 10x10 box, extents 0-10.0
    MDGridBox<MDLeanEvent<2>,2> * superbox = MDEventsTestHelper::makeMDGridBox<2>();
    // And the 0-th box is further split (
    TS_ASSERT_THROWS_NOTHING(superbox->splitContents(0));

    TS_ASSERT_EQUALS( superbox->getNPoints(), 0 );
    { // One event in 0th box of the 0th box.
      double centers[2] = {0.05, 0.05};
      superbox->addEvent( MDLeanEvent<2>(2.0, 2.0, centers) );
    }
    { // One event in 1st box of the 0th box.
      double centers[2] = {0.15, 0.05};
      superbox->addEvent( MDLeanEvent<2>(2.0, 2.0, centers) );
    }
    { // One event in 99th box.
      double centers[2] = {9.5, 9.5};
      superbox->addEvent( MDLeanEvent<2>(2.0, 2.0, centers) );
    }

    // You must refresh the cache after adding individual events.
    superbox->refreshCache(NULL);
    //superbox->refreshCentroid(NULL);

    TS_ASSERT_EQUALS( superbox->getNPoints(), 3 );

    std::vector<coord_t> cenroid(2,0);
    TS_ASSERT_THROWS(superbox->calculateCentroid(&cenroid[0]),std::runtime_error);

    // Check the centroid for these 3 events
    //TS_ASSERT_DELTA( cenroid[0], 3.233, 0.001);
    //TS_ASSERT_DELTA(cenroid[1] , 3.200, 0.001);

    { // One event in 0th box of the 0th box.
      std::vector<coord_t> centers(2,0.05f) ;
      superbox->buildAndAddEvent(2.,2.,centers,0,0);
    }
    { // One event in 1st box of the 0th box.
      std::vector<coord_t> centers(2,0.05f);
      centers[0]=0.15f;
      superbox->buildAndAddEvent(2.,2., centers,0,0 );
    }
    { // One event in 99th box.
      std::vector<coord_t> centers(2,9.5);
      superbox->buildAndAddEvent(2.0, 2.0, centers,0,0 );
    }
    TS_ASSERT_EQUALS( superbox->getNPoints(), 3 );

    superbox->refreshCache(NULL);
    TS_ASSERT_EQUALS( superbox->getNPoints(), 6 );

    // Retrieve the 0th grid box
    boxes = superbox->getBoxes();
    gb = dynamic_cast<MDGridBox<MDLeanEvent<2>,2> *>(boxes[0]);
    TS_ASSERT( gb );

    // It has three points
    TS_ASSERT_EQUALS( gb->getNPoints(), 4 );

    // Retrieve the MDBox at 0th and 1st indexes in THAT gridbox
    boxes = gb->getBoxes();
    b = dynamic_cast<MDBox<MDLeanEvent<2>,2> *>(boxes[0]);
    TS_ASSERT_EQUALS( b->getNPoints(), 2 );
    b = dynamic_cast<MDBox<MDLeanEvent<2>,2> *>(boxes[1]);
    TS_ASSERT_EQUALS( b->getNPoints(), 2 );

    // Get the 99th box at the first level. It is not split
    boxes = superbox->getBoxes();
    b = dynamic_cast<MDBox<MDLeanEvent<2>,2> *>(boxes[99]);
    TS_ASSERT( b ); if (!b) return;
    // And it has only the one point
    TS_ASSERT_EQUALS( b->getNPoints(), 2 );

    BoxController *const bcc =superbox->getBoxController();
    delete superbox;
    delete bcc;

  }

  //-------------------------------------------------------------------------------------
  void test_transformDimensions()
  {
    MDBox<MDLeanEvent<1>,1> * b = MDEventsTestHelper::makeMDBox1();

    // Give it 10 events
    const std::vector<MDLeanEvent<1> > events = MDEventsTestHelper::makeMDEvents1(10);
    b->addEvents( events );
    MDGridBox<MDLeanEvent<1>,1> * g = new MDGridBox<MDLeanEvent<1>,1>(b);
    TSM_ASSERT_EQUALS("MDBoxes start with 1 each.", g->getChild(9)->getNPoints(), 1);

    std::vector<double> scaling(1, 3.0);
    std::vector<double> offset(1, 1.0);
    g->transformDimensions(scaling, offset);

    TS_ASSERT_DELTA(g->getVolume(), 30.0, 1e-5);
    MDLeanEvent<1> ev;
    ev.setCenter(0, 30.9f);
    g->addEvent(ev);
    TSM_ASSERT_EQUALS("New event was added in the right spot.", g->getChild(9)->getNPoints(), 2);

    BoxController *const bcc = b->getBoxController();
    delete b;
    delete bcc;
    delete g;
  }



  //-------------------------------------------------------------------------------------
  /** Recursive getting of a list of MDBoxBase */
  void test_getBoxes()
  {
    MDGridBox<MDLeanEvent<1>,1> * parent = MDEventsTestHelper::makeRecursiveMDGridBox<1>(3,3);
    TS_ASSERT(parent);
    std::vector<API::IMDNode *> boxes;

    boxes.clear();
    parent->getBoxes(boxes, 0, false);
    TS_ASSERT_EQUALS( boxes.size(), 1);
    TS_ASSERT_EQUALS( boxes[0], parent);

    boxes.clear();
    parent->getBoxes(boxes, 1, false);
    TS_ASSERT_EQUALS( boxes.size(), 4);
    TS_ASSERT_EQUALS( boxes[0], parent);
    TS_ASSERT_EQUALS( boxes[1]->getDepth(), 1);

    boxes.clear();
    parent->getBoxes(boxes, 2, false);
    TS_ASSERT_EQUALS( boxes.size(), 4+9);
    TS_ASSERT_EQUALS( boxes[0], parent);
    TS_ASSERT_EQUALS( boxes[1]->getDepth(), 1);
    TS_ASSERT_EQUALS( boxes[2]->getDepth(), 2);

    boxes.clear();
    parent->getBoxes(boxes, 3, false);
    TS_ASSERT_EQUALS( boxes.size(), 4+9+27);

    // Leaves only = only report the deepest boxes.
    boxes.clear();
    parent->getBoxes(boxes, 3, true);
    TS_ASSERT_EQUALS( boxes.size(), 27);
    TS_ASSERT_EQUALS( boxes[0]->getDepth(), 3);

    // Leaves only, with limited depth = report the max depth if that is the effective 'leaf'
    boxes.clear();
    parent->getBoxes(boxes, 2, true);
    TS_ASSERT_EQUALS( boxes.size(), 9);
    TS_ASSERT_EQUALS( boxes[0]->getDepth(), 2);

    BoxController *const bcc = parent->getBoxController();
    delete parent;
    delete bcc;

  }


  //-------------------------------------------------------------------------------------
  /** Recursive getting of a list of MDBoxBase, with an implicit function limiting it */
  void test_getBoxes_ImplicitFunction()
  {

    MDGridBox<MDLeanEvent<1>,1> * parent = MDEventsTestHelper::makeRecursiveMDGridBox<1>(4,3);
    TS_ASSERT(parent);
    std::vector<API::IMDNode *> boxes;

    // Function of everything x > 1.51
    MDImplicitFunction * function = new MDImplicitFunction;
    coord_t normal[1] = {+1};
    coord_t origin[1] = {1.51f};
    function->addPlane( MDPlane(1, normal, origin) );

    boxes.clear();
    parent->getBoxes(boxes, 3, false, function);
    TS_ASSERT_EQUALS( boxes.size(), 54);
    // The boxes extents make sense
    for (size_t i=0; i<boxes.size(); i++)
    {
      TS_ASSERT( boxes[i]->getExtents(0).getMax() >= 1.51);
    }

    // --- Now leaf-only ---
    boxes.clear();
    parent->getBoxes(boxes, 3, true, function);
    TS_ASSERT_EQUALS( boxes.size(), 40);
    // The boxes extents make sense
    for (size_t i=0; i<boxes.size(); i++)
    {
      TS_ASSERT( boxes[i]->getExtents(0).getMax() >= 1.51);
    }

    // Limit by another plane
    coord_t normal2[1] = {-1};
    coord_t origin2[1] = {2.99f};
    function->addPlane( MDPlane(1, normal2, origin2) );
    boxes.clear();
    parent->getBoxes(boxes, 3, false, function);
    TS_ASSERT_EQUALS( boxes.size(), 33);
    for (size_t i=0; i<boxes.size(); i++)
    {
      TS_ASSERT( boxes[i]->getExtents(0).getMax() >= 1.51);
      TS_ASSERT( boxes[i]->getExtents(0).getMin() <= 2.99);
    }

    // Same, leaf only
    boxes.clear();
    parent->getBoxes(boxes, 3, true, function);
    TS_ASSERT_EQUALS( boxes.size(), 24);
    for (size_t i=0; i<boxes.size(); i++)
    {
      TS_ASSERT( boxes[i]->getExtents(0).getMax() >= 1.51);
      TS_ASSERT( boxes[i]->getExtents(0).getMin() <= 2.99);
    }

    // ----- Infinitely thin plane for an implicit function ------------
    delete function;
    function = new MDImplicitFunction;
    coord_t normal3[1] = {-1};
    coord_t origin3[1] = {1.51f};
    function->addPlane( MDPlane(1, normal, origin) );
    function->addPlane( MDPlane(1, normal3, origin3) );

    boxes.clear();
    parent->getBoxes(boxes, 3, true, function);
    TSM_ASSERT_EQUALS( "Only one box is found by an infinitely thin plane", boxes.size(), 1);

    // clean up  behind
    BoxController *const bcc = parent->getBoxController();
    delete parent;
    delete bcc;
    delete function;
  }



  //-------------------------------------------------------------------------------------
  /** Recursive getting of a list of MDBoxBase, with an implicit function limiting it */
  void test_getBoxes_ImplicitFunction_2D()
  {

    MDGridBox<MDLeanEvent<2>,2> * parent = MDEventsTestHelper::makeRecursiveMDGridBox<2>(4,1);
    TS_ASSERT(parent);
    std::vector<API::IMDNode *> boxes;

    // Function of x,y between 2 and 3
    std::vector<coord_t> min(2, 1.99f);
    std::vector<coord_t> max(2, 3.01f);
    MDImplicitFunction * function = new MDBoxImplicitFunction(min, max);

    boxes.clear();
    parent->getBoxes(boxes, 3, false, function);
    TS_ASSERT_EQUALS( boxes.size(), 46);
    // The boxes extents make sense
    for (size_t i=0; i<boxes.size(); i++)
    {
      TS_ASSERT( boxes[i]->getExtents(0).getMax() >= 2.00);
      TS_ASSERT( boxes[i]->getExtents(0).getMin() <= 3.00);
      TS_ASSERT( boxes[i]->getExtents(1).getMax() >= 2.00);
      TS_ASSERT( boxes[i]->getExtents(1).getMin() <= 3.00);
    }

    // -- Leaf only ---
    boxes.clear();
    parent->getBoxes(boxes, 3, true, function);
    TS_ASSERT_EQUALS( boxes.size(), 16+4*4+4); //16 in the center one + 4x4 at the 4 edges + 4 at the corners
    // The boxes extents make sense
    for (size_t i=0; i<boxes.size(); i++)
    {
      TS_ASSERT( boxes[i]->getExtents(0).getMax() >= 2.00);
      TS_ASSERT( boxes[i]->getExtents(0).getMin() <= 3.00);
      TS_ASSERT( boxes[i]->getExtents(1).getMax() >= 2.00);
      TS_ASSERT( boxes[i]->getExtents(1).getMin() <= 3.00);
    }

    // clean up  behind
    BoxController *const bcc = parent->getBoxController();
    delete parent;
    delete bcc;
    delete function;
  }



  //-------------------------------------------------------------------------------------
  /** Recursive getting of a list of MDBoxBase, with an implicit function limiting it.
   * This implicit function is a box of size 0. */
  void test_getBoxes_ZeroSizeImplicitFunction_2D()
  {
    MDGridBox<MDLeanEvent<2>,2> * parent = MDEventsTestHelper::makeRecursiveMDGridBox<2>(4,1);
    TS_ASSERT(parent);
    std::vector<API::IMDNode *> boxes;

    // Function of x,y with 0 width and height
    std::vector<coord_t> min(2, 1.99f);
    std::vector<coord_t> max(2, 1.99f);
    MDImplicitFunction * function = new MDBoxImplicitFunction(min, max);

    boxes.clear();
    parent->getBoxes(boxes, 3, false, function);
    // Returns 3 boxes: the big one with everything, the size 1 under, and the size 0.25 under that
    TS_ASSERT_EQUALS( boxes.size(), 3);

    // Leaf only: returns only one box
    boxes.clear();
    parent->getBoxes(boxes, 3, true, function);
    TS_ASSERT_EQUALS( boxes.size(), 1);
    TS_ASSERT_DELTA( boxes[0]->getExtents(0).getMin(), 1.75, 1e-4);
    TS_ASSERT_DELTA( boxes[0]->getExtents(0).getMax(), 2.00, 1e-4);

    // clean up  behind
    BoxController *const bcc = parent->getBoxController();
    delete parent;
    delete bcc;
    delete function;
  }

  //-------------------------------------------------------------------------------------
  /** Recursive getting of a list of MDBoxBase, with an implicit function limiting it.
   * This implicit function is a box of size 0. */
  void test_getBoxes_ZeroSizeImplicitFunction_4D()
  {
    MDGridBox<MDLeanEvent<4>,4> * parent = MDEventsTestHelper::makeRecursiveMDGridBox<4>(4,1);
    TS_ASSERT(parent);
    std::vector<API::IMDNode *> boxes;

    // Function of x,y with 0 width and height
    std::vector<coord_t> min(4, 1.99f);
    std::vector<coord_t> max(4, 1.99f);
    MDImplicitFunction * function = new MDBoxImplicitFunction(min, max);

    boxes.clear();
    parent->getBoxes(boxes, 3, false, function);
    // Returns 3 boxes: the big one with everything, the size 1 under, and the size 0.25 under that
    TS_ASSERT_EQUALS( boxes.size(), 3);

    // Leaf only: returns only one box
    boxes.clear();
    parent->getBoxes(boxes, 3, true, function);
    TS_ASSERT_EQUALS( boxes.size(), 1);
    TS_ASSERT_DELTA( boxes[0]->getExtents(0).getMin(), 1.75, 1e-4);
    TS_ASSERT_DELTA( boxes[0]->getExtents(0).getMax(), 2.00, 1e-4);

    // clean up  behind
    BoxController *const bcc = parent->getBoxController();
    delete parent;
    delete bcc;
    delete function;
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
      double boxes_per_side = pow(double(numSplit), double(recurseLevels));
      double spacing = double(numSplit)/boxes_per_side;
      // How many times to add the same event
      size_t num_to_repeat = size_t(1e7 / (boxes_per_side*boxes_per_side));

      MDGridBox<MDLeanEvent<2>,2> * box = MDEventsTestHelper::makeRecursiveMDGridBox<2>(numSplit, recurseLevels);

      for (double x=0; x < numSplit; x += spacing)
        for (double y=0; y < numSplit; y += spacing)
        {
          for (size_t i=0; i<num_to_repeat; i++)
          {
            coord_t centers[2] = {static_cast<coord_t>(x),static_cast<coord_t>(y)};
            box->addEvent( MDLeanEvent<2>(2.0, 2.0, centers) );
          }
        }
      // You must refresh the cache after adding individual events.
      box->refreshCache(NULL);
    }

  }



  //-------------------------------------------------------------------------------------
  /** Fill a 10x10 gridbox with events
   *
   * Tests that bad events are thrown out when using addEvents.
   * */
  void test_addEvents_2D()
  {
    MDGridBox<MDLeanEvent<2>,2> * b = MDEventsTestHelper::makeMDGridBox<2>();
    std::vector< MDLeanEvent<2> > events;

    // Make an event in the middle of each box
    for (double x=0.5; x < 10; x += 1.0)
      for (double y=0.5; y < 10; y += 1.0)
      {
        coord_t centers[2] = {static_cast<coord_t>(x),static_cast<coord_t>(y)};
        events.push_back( MDLeanEvent<2>(2.0, 2.0, centers) );
      }

    size_t numbad = 0;
    TS_ASSERT_THROWS_NOTHING( numbad = b->addEvents( events ); );
    // Get the right totals again
    b->refreshCache(NULL);
    TS_ASSERT_EQUALS( numbad, 0);
    TS_ASSERT_EQUALS( b->getNPoints(), 100);
    TS_ASSERT_EQUALS( b->getSignal(), 100*2.0);
    TS_ASSERT_EQUALS( b->getErrorSquared(), 100*2.0);
    TS_ASSERT_DELTA( b->getSignalNormalized(), 100*2.0 / 100.0, 1e-5);
    TS_ASSERT_DELTA( b->getErrorSquaredNormalized(), 100*2.0 / 100.0, 1e-5);

    // Get all the boxes contained
    std::vector<MDBoxBase<MDLeanEvent<2>,2>*> boxes = b->getBoxes();
    TS_ASSERT_EQUALS( boxes.size(), 100);
    for (size_t i=0; i < boxes.size(); i++)
    {
      TS_ASSERT_EQUALS( boxes[i]->getNPoints(), 1);
      TS_ASSERT_EQUALS( boxes[i]->getSignal(), 2.0);
      TS_ASSERT_EQUALS( boxes[i]->getErrorSquared(), 2.0);
      TS_ASSERT_EQUALS( boxes[i]->getSignalNormalized(), 2.0);
      TS_ASSERT_EQUALS( boxes[i]->getErrorSquaredNormalized(), 2.0);
    }

    // Now try to add bad events (outside bounds)
    events.clear();
    for (double x=-5.0; x < 20; x += 20.0)
      for (double y=-5.0; y < 20; y += 20.0)
      {
        double centers[2] = {x,y};
        events.push_back( MDLeanEvent<2>(2.0, 2.0, centers) );
      }
    // Get the right totals again
    b->refreshCache(NULL);
    // All 4 points get rejected
    TS_ASSERT_THROWS_NOTHING( numbad = b->addEvents( events ); );
    TS_ASSERT_EQUALS( numbad, 4);
    // Number of points and signal is unchanged.
    TS_ASSERT_EQUALS( b->getNPoints(), 100);
    TS_ASSERT_EQUALS( b->getSignal(), 100*2.0);
    TS_ASSERT_EQUALS( b->getErrorSquared(), 100*2.0);

    // clean up  behind
    BoxController *const bcc = b->getBoxController();
    delete b;
    delete bcc;

  }


  ////-------------------------------------------------------------------------------------
  ///** Tests add_events with limits into the vectorthat bad events are thrown out when using addEvents.
  // * */
  //void xest_addEvents_start_stop()
  //{
  //  MDGridBox<MDLeanEvent<2>,2> * b = MDEventsTestHelper::makeMDGridBox<2>();
  //  std::vector< MDLeanEvent<2> > events;

  //  // Make an event in the middle of each box
  //  for (double x=0.5; x < 10; x += 1.0)
  //    for (double y=0.5; y < 10; y += 1.0)
  //    {
  //      double centers[2] = {x,y};
  //      events.push_back( MDLeanEvent<2>(2.0, 2.0, centers) );
  //    }

  //  size_t numbad = 0;
  //  TS_ASSERT_THROWS_NOTHING( numbad = b->addEventsPart( events, 50, 60 ); );
  //  // Get the right totals again
  //  b->refreshCache(NULL);
  //  TS_ASSERT_EQUALS( numbad, 0);
  //  TS_ASSERT_EQUALS( b->getNPoints(), 10);
  //  TS_ASSERT_EQUALS( b->getSignal(), 10*2.0);
  //  TS_ASSERT_EQUALS( b->getErrorSquared(), 10*2.0);
  //}

  //-------------------------------------------------------------------------------------
  /** Test that adding events (as vectors) in parallel does not cause
   * segfaults or incorrect totals.
   * */
  void do_test_addEvents_inParallel(ThreadScheduler * ts)
  {
    MDGridBox<MDLeanEvent<2>,2> * b = MDEventsTestHelper::makeMDGridBox<2>();
    int num_repeat = 1000;

    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i=0; i < num_repeat; i++)
    {
      std::vector< MDLeanEvent<2> > events;
      // Make an event in the middle of each box
      for (double x=0.5; x < 10; x += 1.0)
        for (double y=0.5; y < 10; y += 1.0)
        {
          double centers[2] = {x,y};
          events.push_back( MDLeanEvent<2>(2.0, 2.0, centers) );
        }
      //TS_ASSERT_THROWS_NOTHING( b->addEvents( events ); );
    }

    // Get the right totals again by refreshing
    // b->refreshCache(ts);
    // TS_ASSERT_EQUALS( b->getNPoints(), 100*num_repeat);
    // TS_ASSERT_EQUALS( b->getSignal(), 100*num_repeat*2.0);
    // TS_ASSERT_EQUALS( b->getErrorSquared(), 100*num_repeat*2.0);

    // clean up  behind
    BoxController *const bcc = b->getBoxController();
    delete b;
    delete bcc;

  }


  void test_addEvents_inParallel()
  {
    do_test_addEvents_inParallel(NULL);
  }

  /** Disabled because parallel RefreshCache is not implemented. Might not be ever? */
  void xtest_addEvents_inParallel_then_refreshCache_inParallel()
  {
    ThreadScheduler * ts = new ThreadSchedulerFIFO();
    do_test_addEvents_inParallel(ts);
    ThreadPool tp(ts);
    tp.joinAll();
  }


  //-------------------------------------------------------------------------------------
  /** Get a sub-box at a given coord */
  void test_getBoxAtCoord()
  {
    MDGridBox<MDLeanEvent<2>,2> * b = MDEventsTestHelper::makeMDGridBox<2>();
    coord_t coords[2] = {1.5,1.5};
    const MDBoxBase<MDLeanEvent<2>,2> * c = dynamic_cast<const MDBoxBase<MDLeanEvent<2>,2> *>(b->getBoxAtCoord(coords));
    TS_ASSERT_EQUALS(c, b->getChild(11));
    delete b->getBoxController();
    delete b;
  }


  //-------------------------------------------------------------------------------------
  /** Test the routine that auto-splits MDBoxes into MDGridBoxes recursively.
   * It tests the max_depth of splitting too, because there are numerous
   * repeated events at exactly the same position = impossible to separate further.
   * */
  void test_splitAllIfNeeded()
  {
    typedef MDGridBox<MDLeanEvent<2>,2> gbox_t;
    typedef MDBox<MDLeanEvent<2>,2> box_t;
    typedef MDBoxBase<MDLeanEvent<2>,2> ibox_t;

    gbox_t * b0 = MDEventsTestHelper::makeMDGridBox<2>();
    b0->getBoxController()->setSplitThreshold(100);
    b0->getBoxController()->setMaxDepth(4);

    // Make a 1000 events at exactly the same point
    size_t num_repeat = 1000;
    std::vector< MDLeanEvent<2> > events;
    for (size_t i=0; i < num_repeat; i++)
    {
      // Make an event in the middle of each box
      double centers[2] = {1e-10, 1e-10};
      events.push_back( MDLeanEvent<2>(2.0, 2.0, centers) );
    }
    TS_ASSERT_THROWS_NOTHING( b0->addEvents( events ); );


    // Split into sub-grid boxes
    TS_ASSERT_THROWS_NOTHING( b0->splitAllIfNeeded(NULL); )

    // Dig recursively into the gridded box hierarchies
    std::vector<ibox_t*> boxes;
    size_t expected_depth = 0;
    gbox_t *b = b0;
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

    // clean up  behind
    BoxController *const bcc = b0->getBoxController();
    delete b0;
    delete bcc;

  }


  //------------------------------------------------------------------------------------------------
  /** This test splits a large number of events, and uses a ThreadPool
   * to use all cores.
   */
  void test_splitAllIfNeeded_usingThreadPool()
  {
    typedef MDGridBox<MDLeanEvent<2>,2> gbox_t;
    typedef MDBoxBase<MDLeanEvent<2>,2> ibox_t;

    gbox_t * b = MDEventsTestHelper::makeMDGridBox<2>();
    b->getBoxController()->setSplitThreshold(100);
    b->getBoxController()->setMaxDepth(4);

    // Make a 1000 events in each sub-box
    size_t num_repeat = 1000;
    if (DODEBUG) num_repeat = 2000;

    Timer tim;
    if (DODEBUG) std::cout << "Adding " << num_repeat*100 << " events...\n";
    MDEventsTestHelper::feedMDBox<2>(b, num_repeat, 10, 0.5, 1.0);
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
      ibox_t * box = boxes[i];
      TS_ASSERT_EQUALS( box->getNPoints(), num_repeat );
      TS_ASSERT( dynamic_cast<gbox_t *>(box) );

      size_t numChildren = box->getNumChildren();
      if (numChildren > 0)
      {
        size_t lastId = box->getChild(0)->getID();
        for (size_t i = 1; i < numChildren; i++)
        {
          TSM_ASSERT_EQUALS("Children IDs need to be sequential!", box->getChild(i)->getID(), lastId+1);
          lastId = box->getChild(i)->getID();
        }
      }

    }

    // clean up  behind
    BoxController *const bcc = b->getBoxController();
    delete b;
    delete bcc;

  }






  //------------------------------------------------------------------------------------------------
  /** Helper to make a 2D MDBin */
  MDBin<MDLeanEvent<2>,2> makeMDBin2(double minX, double maxX, double minY, double maxY)
  {
    MDBin<MDLeanEvent<2>,2> bin;
    bin.m_min[0] = static_cast<coord_t>(minX);
    bin.m_max[0] = static_cast<coord_t>(maxX);
    bin.m_min[1] = static_cast<coord_t>(minY);
    bin.m_max[1] = static_cast<coord_t>(maxY);
    return bin;
  }

  //------------------------------------------------------------------------------------------------
  /** Helper to test the binning of a 2D bin */
  void doTestMDBin2(MDGridBox<MDLeanEvent<2>,2> * b,
      const std::string & message,
      double minX, double maxX, double minY, double maxY, double expectedSignal)
  {
//    std::cout << "Bins: X " << std::setw(5) << minX << " to "<< std::setw(5)  << maxX << ", Y " << std::setw(5) << minY << " to "<< std::setw(5)  << maxY      << ". " << message << std::endl;

    MDBin<MDLeanEvent<2>,2> bin;
    bin = makeMDBin2(minX, maxX, minY, maxY);
    b->centerpointBin(bin, NULL);
    TSM_ASSERT_DELTA( message, bin.m_signal, expectedSignal, 1e-5);
  }

  //------------------------------------------------------------------------------------------------
  /** Test binning in orthogonal axes */
  void test_centerpointBin()
  {
    typedef MDGridBox<MDLeanEvent<2>,2> gbox_t;

    // 10x10 bins, 2 events per bin, each weight of 1.0
    gbox_t * b = MDEventsTestHelper::makeMDGridBox<2>();
    MDEventsTestHelper::feedMDBox<2>(b, 2);
    TS_ASSERT_DELTA( b->getSignal(), 200.0, 1e-5);

    MDBin<MDLeanEvent<2>,2> bin;

    doTestMDBin2(b, "Bin that is completely off",
        10.1, 11.2, 1.9, 3.12,   0.0);

    doTestMDBin2(b, "Bin that is completely off (2)",
        2, 3, -0.6, -0.1,   0.0);

    doTestMDBin2(b, "Bin that holds one entire MDBox (bigger than it)",
        0.8, 2.2, 1.9, 3.12,     2.0);

    doTestMDBin2(b, "Bin that holds one entire MDBox (going off one edge)",
        -0.2, 1.2, 1.9, 3.12,     2.0);

    doTestMDBin2(b, "Bin that holds one entire MDBox (going off the other edge)",
        8.9, 10.2, 1.9, 3.12,     2.0);

    doTestMDBin2(b, "Bin that holds one entire MDBox (going off both edge)",
        -0.2, 1.2, -0.2, 1.2,     2.0);

    doTestMDBin2(b, "Bin that holds one entire MDBox and a fraction of at least one more with something",
        0.8, 2.7, 1.9, 3.12,     4.0);

    doTestMDBin2(b, "Bin that holds four entire MDBoxes",
        0.8, 3.1, 0.9, 3.2,      8.0);

    doTestMDBin2(b, "Bin goes off two edges in one direction",
        -0.3, 10.2, 1.9, 3.1,   10*2.0);

    doTestMDBin2(b, "Bin that fits all within a single MDBox, and contains the center",
        0.2, 0.8, 0.2, 0.8,     2.0);

    doTestMDBin2(b, "Bin that fits all within a single MDBox, and DOES NOT contain anything",
        0.2, 0.3, 0.1, 0.2,     0.0);

    doTestMDBin2(b, "Bin that fits partially in two MDBox'es, and DOES NOT contain anything",
        0.8, 1.2, 0.1, 0.2,     0.0);

    doTestMDBin2(b, "Bin that fits partially in two MDBox'es, and contains the centers",
        0.2, 1.8, 0.1, 0.9,     4.0);

    doTestMDBin2(b, "Bin that fits partially in one MDBox'es, and goes of the edge",
        -3.2, 0.8, 0.1, 0.9,     2.0);

    delete b->getBoxController();
    delete b;
  }





  //------------------------------------------------------------------------------------------------
  /** For test_integrateSphere
   *
   * @param box
   * @param radius :: radius to integrate
   * @param numExpected :: how many events should be in there
   */
  void do_check_integrateSphere(MDGridBox<MDLeanEvent<2>,2> & box, double x, double y, const double radius, double numExpected, std::string message)
  {
    // The sphere transformation
    bool dimensionsUsed[2] = {true,true};
    coord_t center[2] = {static_cast<coord_t>(x),static_cast<coord_t>(y)};
    CoordTransformDistance sphere(2, center, dimensionsUsed);

    signal_t signal = 0;
    signal_t errorSquared = 0;
    box.integrateSphere(sphere, static_cast<coord_t>(radius*radius), signal, errorSquared);
    TSM_ASSERT_DELTA( message, signal, 1.0*numExpected, 1e-5);
    TSM_ASSERT_DELTA( message, errorSquared, 1.0*numExpected, 1e-5);
  }

  /** Re-used suite of tests */
  void do_test_integrateSphere(MDGridBox<MDLeanEvent<2>,2> * box_ptr)
  {
    // Events are at 0.5, 1.5, etc.
    MDGridBox<MDLeanEvent<2>,2> & box = *box_ptr;
    TS_ASSERT_EQUALS( box.getNPoints(), 10*10);

    do_check_integrateSphere(box, 4.5,4.5,  0.5,   1.0, "Too small to contain any vertices");
    do_check_integrateSphere(box, 4.5, 4.5, 0.001, 1.0, "Tiny but still has an event.");
    do_check_integrateSphere(box, 4.51,4.5, 0.001, 0.0, "Tiny but off the event.");
    do_check_integrateSphere(box, 2.0,2.0,  0.49,  0.0, "At a corner but grabbing nothing");
    do_check_integrateSphere(box, 4.8,4.5,  0.35,  1.0, "Too small to contain any vertices");
    do_check_integrateSphere(box, 5.0,5.0,  1.0,   4.0, "At a corner, containing 4 neighbors");
    do_check_integrateSphere(box, 4.5,4.5,  0.9,   1.0, "Contains one box completely");
    do_check_integrateSphere(box, 0.5,0.5,  0.9,   1.0, "Contains one box completely, at the edges");
    do_check_integrateSphere(box, 9.5,0.5,  0.9,   1.0, "Contains one box completely, at the edges");
    do_check_integrateSphere(box, 0.5,9.5,  0.9,   1.0, "Contains one box completely, at the edges");
    do_check_integrateSphere(box, 4.5,9.5,  0.9,   1.0, "Contains one box completely, at the edges");
    do_check_integrateSphere(box, 9.5,9.5,  0.9,   1.0, "Contains one box completely, at the edges");
    do_check_integrateSphere(box, 1.5,1.5,  1.95,  9.0, "Contains 5 boxes completely, and 4 boxes with a point");
    do_check_integrateSphere(box, -1.0,0.5, 1.55,  1.0, "Off an edge but enough to get an event");

    // Now I add an event very near an edge
    double center[2] = {0.001, 0.5};
    box.addEvent(MDLeanEvent<2>(1.0, 1.0, center));
    do_check_integrateSphere(box, -1.0,0.5, 1.01,  1.0, "Off an edge but just barely enough to get an event");
    do_check_integrateSphere(box, 0.0,0.5, 0.01,  1.0, "Tiny, but just barely enough to get an event");
  }

  /** Test of sphere integration with even splitting*/
  void test_integrateSphere()
  {
    // 10x10 sized box
    MDGridBox<MDLeanEvent<2>,2> * box_ptr =  MDEventsTestHelper::makeMDGridBox<2>();
    MDEventsTestHelper::feedMDBox<2>(box_ptr, 1);
    do_test_integrateSphere(box_ptr);

    // clean up  behind
    BoxController *const bcc = box_ptr->getBoxController();
    delete box_ptr;
    delete bcc;

  }

  void test_integrateSphere_unevenSplit()
  {
    // 10x5 sized box
    MDGridBox<MDLeanEvent<2>,2> * box_ptr = MDEventsTestHelper::makeMDGridBox<2>(10,5);
    MDEventsTestHelper::feedMDBox<2>(box_ptr, 1);
    do_test_integrateSphere(box_ptr);
    // clean up  behind
    BoxController *const bcc = box_ptr->getBoxController();
    delete box_ptr;
    delete bcc;

  }

  void test_integrateSphere_unevenSplit2()
  {
    // Funnier splitting: 3x7 sized box
    MDGridBox<MDLeanEvent<2>,2> * box_ptr = MDEventsTestHelper::makeMDGridBox<2>(3,7);
    MDEventsTestHelper::feedMDBox<2>(box_ptr, 1);
    do_test_integrateSphere(box_ptr);

    // clean up  behind
    BoxController *const bcc = box_ptr->getBoxController();
    delete box_ptr;
    delete bcc;

  }


  /** Had a really-hard to find bug where the tests worked only
   * if the extents started at 0.0.
   * This test has a box from -10.0 to +10.0 to check for that
   */
  void test_integrateSphere_dimensionsDontStartAtZero()
  {
    MDGridBox<MDLeanEvent<2>,2> * box_ptr = MDEventsTestHelper::makeMDGridBox<2>(10,10, -10.0);
    // One event at center of each box
    MDEventsTestHelper::feedMDBox<2>(box_ptr, 1, 10, -9.0, 2.0);
    MDGridBox<MDLeanEvent<2>,2> & box = *box_ptr;
    TS_ASSERT_EQUALS( box.getNPoints(), 10*10);

    do_check_integrateSphere(box, 1.0,1.0,  1.45,  1.0, "Contains one box completely");
    do_check_integrateSphere(box, 9.0,9.0,  1.45,  1.0, "Contains one box completely, at the edges");

    // clean up  behind
    BoxController *const bcc = box_ptr->getBoxController();
    delete box_ptr;
    delete bcc;

  }


  //------------------------------------------------------------------------------------------------
  /** For test_integrateSphere3d
   *
   * @param box
   * @param radius :: radius to integrate
   * @param numExpected :: how many events should be in there
   */
  void do_check_integrateSphere3d(MDGridBox<MDLeanEvent<3>,3> & box, double x, double y, double z,
      const double radius, double numExpected, std::string message)
  {
    // The sphere transformation
    bool dimensionsUsed[3] = {true,true, true};
    coord_t center[3] = {static_cast<coord_t>(x),static_cast<coord_t>(y),static_cast<coord_t>(z)};
    CoordTransformDistance sphere(3, center, dimensionsUsed);

    signal_t signal = 0;
    signal_t errorSquared = 0;
    box.integrateSphere(sphere, static_cast<coord_t>(radius*radius), signal, errorSquared);
    TSM_ASSERT_DELTA( message, signal, 1.0*numExpected, 1e-5);
    TSM_ASSERT_DELTA( message, errorSquared, 1.0*numExpected, 1e-5);
  }

  //------------------------------------------------------------------------------------------------
  void test_integrateSphere3d()
  {
    MDGridBox<MDLeanEvent<3>,3> * box_ptr = MDEventsTestHelper::makeMDGridBox<3>();
    MDEventsTestHelper::feedMDBox<3>(box_ptr, 1);
    MDGridBox<MDLeanEvent<3>,3> & box = *box_ptr;
    TS_ASSERT_EQUALS( box.getNPoints(), 10*10*10);

    do_check_integrateSphere3d(box, 0.5,0.5,0.5,  0.9,   1.0, "Contains one box completely, at a corner");
    do_check_integrateSphere3d(box, 9.5,9.5,9.5,  0.9,   1.0, "Contains one box completely, at a corner");
    do_check_integrateSphere3d(box, 9.5,9.5,9.5,  0.85,  1.0, "Does NOT contain one box completely, at a corner");
    do_check_integrateSphere3d(box, 9.0,9.0,9.0,  1.75, 20.0, "Contains 8 boxes completely, at a corner");
    do_check_integrateSphere3d(box, 9.0,9.0,9.0,  1.70, 20.0, "Does NOT contains one box completely, at a corner");

    // Now I add an event very near an edge
    double center[3] = {0.001, 0.5, 0.5};
    box.addEvent(MDLeanEvent<3>(2.0, 2.0, center));
//    do_check_integrateSphere(box, -1.0,0.5, 1.01,  1.0, "Off an edge but just barely enough to get an event");
//    do_check_integrateSphere(box, 0.0,0.5, 0.01,  1.0, "Tiny, but just barely enough to get an event");
    delete box_ptr->getBoxController();
    delete box_ptr;
  }





  //------------------------------------------------------------------------------------------------
  /** For test_integrateSphere
   *
   * @param box
   * @param radius :: radius to integrate
   * @param xExpected :: expected centroid
   * @param yExpected :: expected centroid
   */
  void do_check_centroidSphere(MDGridBox<MDLeanEvent<2>,2> & box, double x, double y,
      const double radius,
      double numExpected, double xExpected, double yExpected,
      std::string message)
  {
    // The sphere transformation
    bool dimensionsUsed[2] = {true,true};
    coord_t center[2] = {static_cast<coord_t>(x),static_cast<coord_t>(y)};
    CoordTransformDistance sphere(2, center, dimensionsUsed);

    signal_t signal = 0;
    coord_t centroid[2] = {0., 0.};
    box.centroidSphere(sphere, static_cast<coord_t>(radius*radius), centroid, signal);
    // Normalized
    if (signal != 0.0)
    {
      for (size_t d=0; d<2; d++)
        centroid[d] /= static_cast<coord_t>(signal);
    }

    TSM_ASSERT_DELTA( message, signal, 1.0*numExpected, 1e-5);
    TSM_ASSERT_DELTA( message, centroid[0], xExpected, 1e-5);
    TSM_ASSERT_DELTA( message, centroid[1], yExpected, 1e-5);
  }

  /** Re-used suite of tests */
  void test_centroidSphere()
  {
    // 10x10 sized box
    MDGridBox<MDLeanEvent<2>,2> * box_ptr = MDEventsTestHelper::makeMDGridBox<2>();
    MDEventsTestHelper::feedMDBox<2>(box_ptr, 1);
    // Events are at 0.5, 1.5, etc.
    MDGridBox<MDLeanEvent<2>,2> & box = *box_ptr;
    TS_ASSERT_EQUALS( box.getNPoints(), 10*10);

    do_check_centroidSphere(box, 4.5,4.5,  0.5,   1.0, 4.5, 4.5, "Too small to contain any vertices");
    do_check_centroidSphere(box, 4.5, 4.5, 0.001, 1.0, 4.5, 4.5, "Tiny but still has an event.");
    do_check_centroidSphere(box, 4.51,4.5, 0.001, 0.0, 0.0, 0.0, "Tiny but off the event.");
    do_check_centroidSphere(box, 2.0,2.0,  0.49,  0.0, 0.0, 0.0, "At a corner but grabbing nothing");
    do_check_centroidSphere(box, 4.8,4.5,  0.35,  1.0, 4.5, 4.5, "Too small to contain any vertices");
    do_check_centroidSphere(box, 5.0,5.0,  1.0,   4.0, 5.0, 5.0, "At a corner, containing 4 neighbors");
    do_check_centroidSphere(box, 4.5,4.5,  0.9,   1.0, 4.5, 4.5, "Contains one box completely");
    do_check_centroidSphere(box, 0.5,0.5,  0.9,   1.0, 0.5, 0.5, "Contains one box completely, at the edges");
    do_check_centroidSphere(box, 9.5,0.5,  0.9,   1.0, 9.5, 0.5, "Contains one box completely, at the edges");
    do_check_centroidSphere(box, 0.5,9.5,  0.9,   1.0, 0.5, 9.5, "Contains one box completely, at the edges");
    do_check_centroidSphere(box, 4.5,9.5,  0.9,   1.0, 4.5, 9.5, "Contains one box completely, at the edges");
    do_check_centroidSphere(box, 9.5,9.5,  0.9,   1.0, 9.5, 9.5, "Contains one box completely, at the edges");
    do_check_centroidSphere(box, 1.5,1.5,  1.95,  9.0, 1.5, 1.5, "Contains 5 boxes completely, and 4 boxes with a point");
    do_check_centroidSphere(box, -1.0,0.5, 1.55,  1.0, 0.5, 0.5, "Off an edge but enough to get an event");

    // Now I add an event very near an edge
    double center[2] = {0.001, 0.5};
    box.addEvent(MDLeanEvent<2>(1.0, 1.0, center));
    do_check_integrateSphere(box, -1.0,0.5, 1.01,  1.0, "Off an edge but just barely enough to get an event");
    do_check_integrateSphere(box, 0.0,0.5, 0.01,  1.0, "Tiny, but just barely enough to get an event");

    delete box_ptr->getBoxController();
    delete box_ptr;
  }

  void test_getIsMasked_WhenNoMasking()
  {
      std::vector<API::IMDNode *> boxes;

    MockMDBox* a = new MockMDBox;
    MockMDBox* b = new MockMDBox;

    EXPECT_CALL(*a, getIsMasked()).Times(1).WillOnce(Return(false)); //Not masked
    EXPECT_CALL(*b, getIsMasked()).Times(1).WillOnce(Return(false)); //Not masked

    boxes.push_back(a);
    boxes.push_back(b);

    auto bc = boost::shared_ptr<BoxController>(new BoxController(1));
    std::vector<Mantid::Geometry::MDDimensionExtents<coord_t> > extentsVector(1);
    MDGridBox<MDLeanEvent<1>,1> g(bc,0,extentsVector);
    g.setChildren(boxes, 0, 2);

    TSM_ASSERT("No inner boxes were masked so the MDGridBox should not report that it is masked", !g.getIsMasked());
    TS_ASSERT(Mock::VerifyAndClearExpectations(a));
    TS_ASSERT(Mock::VerifyAndClearExpectations(b));
  }

  void test_getIsMasked_WhenFirstMasked()
  {
    std::vector<API::IMDNode *> boxes;

    MockMDBox* a = new MockMDBox;
    MockMDBox* b = new MockMDBox;

    EXPECT_CALL(*a, getIsMasked()).Times(1).WillOnce(Return(true)); //MASKED
    EXPECT_CALL(*b, getIsMasked()).Times(0); //Not masked, but will never be called.

    boxes.push_back(a);
    boxes.push_back(b);

    auto bc = boost::shared_ptr<BoxController>(new BoxController(1));
    std::vector<Mantid::Geometry::MDDimensionExtents<coord_t> > extentsVector(1);
    MDGridBox<MDLeanEvent<1>,1> g(bc,0,extentsVector);
    g.setChildren(boxes, 0, 2);

    TSM_ASSERT("First inner box masked, so should return masked", g.getIsMasked());
    TS_ASSERT(Mock::VerifyAndClearExpectations(a));
    TS_ASSERT(Mock::VerifyAndClearExpectations(b));
  }

  void test_getIsMasked_WhenLastMasked()
  {
      std::vector<API::IMDNode *> boxes;

    MockMDBox* a = new MockMDBox;
    MockMDBox* b = new MockMDBox;

    EXPECT_CALL(*a, getIsMasked()).Times(1).WillOnce(Return(false)); //NOT MASKED
    EXPECT_CALL(*b, getIsMasked()).Times(1).WillOnce(Return(true)); //MASKED

    boxes.push_back(a);
    boxes.push_back(b);

    auto bc = boost::shared_ptr<BoxController>(new BoxController(1));
    std::vector<Mantid::Geometry::MDDimensionExtents<coord_t> > extentsVector(1);
    MDGridBox<MDLeanEvent<1>,1> g(bc,0,extentsVector);

    g.setChildren(boxes, 0, 2);

    TSM_ASSERT("Second inner box masked, so should return masked", g.getIsMasked());
    TS_ASSERT(Mock::VerifyAndClearExpectations(a));
    TS_ASSERT(Mock::VerifyAndClearExpectations(b));
  }

  void test_mask()
  {
    std::vector<API::IMDNode *> boxes;

    MockMDBox* a = new MockMDBox;
    MockMDBox* b = new MockMDBox;

    EXPECT_CALL(*a, mask()).Times(1);
    EXPECT_CALL(*b, mask()).Times(1);

    boxes.push_back(a);
    boxes.push_back(b);

    auto bc = boost::shared_ptr<BoxController>(new BoxController(1));
    std::vector<Mantid::Geometry::MDDimensionExtents<coord_t> > extentsVector(1);
    MDGridBox<MDLeanEvent<1>,1> griddedBox(bc,0,extentsVector);

    griddedBox.setChildren(boxes, 0, 2);

    TS_ASSERT_THROWS_NOTHING(griddedBox.mask());//Mask the gridded box

    TS_ASSERT(Mock::VerifyAndClearExpectations(a));
    TS_ASSERT(Mock::VerifyAndClearExpectations(b));
  }

  void test_unmask()
  {
    std::vector<API::IMDNode *> boxes;

    MockMDBox* a = new MockMDBox;
    MockMDBox* b = new MockMDBox;

    EXPECT_CALL(*a, unmask()).Times(1);
    EXPECT_CALL(*b, unmask()).Times(1);

    boxes.push_back(a);
    boxes.push_back(b);

    auto bc = boost::shared_ptr<BoxController>(new BoxController(1));
    std::vector<Mantid::Geometry::MDDimensionExtents<coord_t> > extentsVector(1);
    MDGridBox<MDLeanEvent<1>,1> griddedBox(bc,0,extentsVector);

    griddedBox.setChildren(boxes, 0, 2);

    TS_ASSERT_THROWS_NOTHING(griddedBox.unmask());//Un-Mask the gridded box

    TS_ASSERT(Mock::VerifyAndClearExpectations(a));
    TS_ASSERT(Mock::VerifyAndClearExpectations(b));
  }

private:
  std::string message;
};










//=====================================================================================
//===================================== Performance Test ==============================
//=====================================================================================
class MDGridBoxTestPerformance : public CxxTest::TestSuite
{
public:

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDGridBoxTestPerformance *createSuite() { return new MDGridBoxTestPerformance(); }
  static void destroySuite( MDGridBoxTestPerformance *suite ) { delete suite; }

  MDGridBox<MDLeanEvent<3>,3> * box3;
  MDGridBox<MDLeanEvent<3>,3> * box3b;
  std::vector<MDLeanEvent<3> > events;
  MDGridBox<MDLeanEvent<1>,1> * recursiveParent;
  MDGridBox<MDLeanEvent<1>,1> * recursiveParent2;

  MDGridBoxTestPerformance()
  {
    // Split 5x5x5, 2 deep.
    box3b = MDEventsTestHelper::makeRecursiveMDGridBox<3>(5,1);

    // Make the list of fake events, random dist.
    size_t num = 1000000;
    events.clear();

    boost::mt19937 rng;
    boost::uniform_real<double> u(0, 5.0); // Range
    boost::variate_generator<boost::mt19937&, boost::uniform_real<double> > gen(rng, u);
    for (size_t i=0; i<num; ++i)
    {
      double centers[3];
      for (size_t d=0; d<3; d++)
        centers[d] = gen();
      // Create and add the event.
      events.push_back( MDLeanEvent<3>( 1.0, 1.0, centers) );
    }

    box3b->addEvents(events);
    box3b->refreshCache();
    // Recursively gridded box with 1,111,111 boxes total.
    recursiveParent = MDEventsTestHelper::makeRecursiveMDGridBox<1>(10,6);
    // Recursively gridded box with 111,111 boxes total.
    recursiveParent2 = MDEventsTestHelper::makeRecursiveMDGridBox<1>(10,5);

  }

  ~MDGridBoxTestPerformance()
  {
    delete box3b;
  }

  void setUp()
  {
    // Make a fresh box.
    box3 = MDEventsTestHelper::makeRecursiveMDGridBox<3>(5,1);
  }

  void tearDown()
  {
    delete box3;
  }


  void test_refreshCache()
  {
    box3b->refreshCache();
  }

  /** Performance test that adds lots of events to a recursively split box.
   * SINGLE-THREADED!
   */
  void test_addEvents_lots()
  {
    // We built this many MDBoxes
    TS_ASSERT_EQUALS( box3->getBoxController()->getTotalNumMDBoxes(), 125*125+1); // +1 might be a test issue
    TS_ASSERT_EQUALS( events.size(), 1e6);

    // Add them!
    for(size_t i=0; i<5; ++i)
    {
      box3->addEvents(events);
    }
  }

  //-----------------------------------------------------------------------------
  /** Do a sphere integration
   *
   * @param center :: coordinate of the center
   * @param radius :: radius
   */
  void do_test_sphereIntegrate(coord_t * center, coord_t radius, double expectSignal, double tol)
  {
    // The sphere transformation
    bool dimensionsUsed[3] = {true,true,true};
    CoordTransformDistance sphere(3, center, dimensionsUsed);

    // Repeat the integration a lot
    signal_t signal, errorSquared;
    for (size_t i=0; i < 1000; i++)
    {
      signal = 0;
      errorSquared = 0;
      box3b->integrateSphere(sphere, radius*radius, signal, errorSquared);
    }

    TS_ASSERT_DELTA(signal, expectSignal, tol);
    TS_ASSERT_DELTA(signal, errorSquared, 1e-3);
  }

  /** Smallish sphere in the middle goes partially through lots of boxes */
  void test_sphereIntegrate_inTheMiddle()
  {
    coord_t center[3] = {2.5, 2.5, 2.5};
    do_test_sphereIntegrate(center, 1.0, (1e6/125)*(4.0*M_PI/3.0), 2000.0);
  }

  /** Huge sphere containing all within */
  void test_sphereIntegrate_inTheMiddle_largeSphere()
  {
    coord_t center[3] = {2.5, 2.5, 2.5};
    do_test_sphereIntegrate(center, 5.0, 1e6, 1e-3);
  }

  /** Peak that is off the box entirely */
  void test_sphereIntegrate_OffTheBox()
  {
    coord_t center[3] = {11., 5., 5.};
    do_test_sphereIntegrate(center, 1.0, 0.0, 1e-3);
  }




  //-----------------------------------------------------------------------------
  /** Do a sphere centroiding
   *
   * @param center :: coordinate of the center
   * @param radius :: radius
   */
  void do_test_sphereCentroid(coord_t * center, coord_t radius, double expectSignal, double tol)
  {
    // The sphere transformation
    bool dimensionsUsed[3] = {true,true,true};
    CoordTransformDistance sphere(3, center, dimensionsUsed);

    // Repeat the integration a lot
    signal_t signal;
    coord_t centroid[3];
    for (size_t i=0; i < 100; i++)
    {
      signal = 0;
      for (size_t d=0; d<3; d++)
        centroid[d] = 0.0;
      box3b->centroidSphere(sphere, radius*radius, centroid, signal) ;
      if (signal != 0.0)
      {
        for (size_t d=0; d<3; d++)
          centroid[d] /= static_cast<coord_t>(signal);
      }
    }

    // The expected number of events, given a sphere of radius "radius"
    TS_ASSERT_DELTA(signal, expectSignal, tol);

    if (expectSignal > 0.0)
    {
      // And the centroid should be close to the sphere center
      for (size_t d=0; d<3; d++)
        TS_ASSERT_DELTA(centroid[d], center[d], 1e-2);
    }
  }

  /** Smallish sphere in the middle goes partially through lots of boxes */
  void test_sphereCentroid_inTheMiddle()
  {
    coord_t center[3] = {2.5, 2.5, 2.5};
    do_test_sphereCentroid(center, 1.0, (1e6/125)*(4.0*M_PI/3.0), 2000);
  }

  /** Huge sphere containing all within */
  void test_sphereCentroid_inTheMiddle_largeSphere()
  {
    coord_t center[3] = {2.5, 2.5, 2.5};
    do_test_sphereCentroid(center, 5.0, 1e6, 1e-3);
  }

  /** Peak that is off the box entirely */
  void test_sphereCentroid_OffTheBox()
  {
    coord_t center[3] = {11., 5., 5.};
    do_test_sphereCentroid(center, 1.0, 0.0, 1e-3);
  }


  /** Recursive getting of a list of MDBoxBase.
   * Gets about 11 million boxes */
  void test_getBoxes()
  {
    std::vector<API::IMDNode *> boxes;
    for (size_t i=0; i<10; i++)
    {
      boxes.clear();
      boxes.reserve(1111111);
      recursiveParent->getBoxes(boxes, 6, false);
      TS_ASSERT_EQUALS( boxes.size(), 1111111);
      TS_ASSERT_EQUALS( boxes[0], recursiveParent);
    }
  }


};

#endif
