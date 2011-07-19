#ifndef MANTID_MDEVENTS_IMDBOXTEST_H_
#define MANTID_MDEVENTS_IMDBOXTEST_H_

#include "MantidAPI/ImplicitFunction.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidNexus/NeXusFile.hpp"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include <Poco/File.h>

using namespace Mantid;
using namespace Mantid::MDEvents;
using Mantid::Kernel::ConfigService;

/** Tester class that implements the minimum IMDBox to
 * allow testing
 */
TMDE_CLASS
class IMDBoxTester : public IMDBox<MDE,nd>
{

  /// Clear all contained data
  virtual void clear()
  {}

  /// Get total number of points
  virtual size_t getNPoints() const
  {return 0;}

  /// Get number of dimensions
  virtual size_t getNumDims() const
  {return nd;}

  /// Get the total # of unsplit MDBoxes contained.
  virtual size_t getNumMDBoxes() const
  {return 0;}

  virtual size_t getNumChildren() const
  {return 0;}

  IMDBox<MDE,nd> * getChild(size_t /*index*/)
  { throw std::runtime_error("MDBox does not have children."); }

  /// Sets the children from a vector of children
  void setChildren(const std::vector<IMDBox<MDE,nd> *> & /*boxes*/, const size_t /*indexStart*/, const size_t /*indexEnd*/)
  { throw std::runtime_error("MDBox cannot have children."); }

  /// Return a copy of contained events
  virtual std::vector< MDE > * getEventsCopy()
  {return NULL;}

  /// Add a single event
  virtual void addEvent(const MDE & /*point*/)
  {}

  /// Add several events
  virtual size_t addEvents(const std::vector<MDE> & /*events*/)
  {return 0;}

  /** Perform centerpoint binning of events
   * @param bin :: MDBin object giving the limits of events to accept.
   */
  virtual void centerpointBin(MDBin<MDE,nd> & /*bin*/, bool * ) const
  {}

  virtual void integrateSphere(CoordTransform & /*radiusTransform*/, const coord_t /*radiusSquared*/, signal_t & /*signal*/, signal_t & /*errorSquared*/) const {};
  virtual void centroidSphere(CoordTransform & /*radiusTransform*/, const coord_t /*radiusSquared*/, coord_t *, signal_t & ) const {};
  virtual void getBoxes(std::vector<IMDBox<MDE,nd> *>&  /*boxes*/, size_t /*maxDepth*/, bool) {};
  virtual void generalBin(MDBin<MDE,nd> & /*bin*/, Mantid::API::ImplicitFunction & /*function*/) const {}

};


class IMDBoxTest : public CxxTest::TestSuite
{
public:

  void test_default_constructor()
  {
    IMDBoxTester<MDEvent<3>,3> box;
    TS_ASSERT_EQUALS( box.getSignal(), 0.0);
    TS_ASSERT_EQUALS( box.getErrorSquared(), 0.0);
  }

  void test_get_and_set_signal()
  {
    IMDBoxTester<MDEvent<3>,3> box;
    TS_ASSERT_EQUALS( box.getSignal(), 0.0);
    TS_ASSERT_EQUALS( box.getErrorSquared(), 0.0);
    box.setSignal(123.0);
    box.setErrorSquared(456.0);
    TS_ASSERT_EQUALS( box.getSignal(), 123.0);
    TS_ASSERT_EQUALS( box.getErrorSquared(), 456.0);
    TS_ASSERT_DELTA( box.getError(), sqrt(456.0), 1e-4);
  }

  void test_get_and_set_depth()
  {
    IMDBoxTester<MDEvent<3>,3> b;
    b.setDepth(123);
    TS_ASSERT_EQUALS( b.getDepth(), 123);
  }

  /** Setting and getting the extents;
   * also, getting the center */
  void test_setExtents()
  {
    IMDBoxTester<MDEvent<2>,2> b;
    b.setExtents(0, -8.0, 10.0);
    TS_ASSERT_DELTA(b.getExtents(0).min, -8.0, 1e-6);
    TS_ASSERT_DELTA(b.getExtents(0).max, +10.0, 1e-6);

    b.setExtents(1, -4.0, 12.0);
    TS_ASSERT_DELTA(b.getExtents(1).min, -4.0, 1e-6);
    TS_ASSERT_DELTA(b.getExtents(1).max, +12.0, 1e-6);

    TS_ASSERT_THROWS( b.setExtents(2, 0, 1.0), std::invalid_argument);

    coord_t center[2];
    b.getCenter(center);
    TS_ASSERT_DELTA( center[0], +1.0, 1e-6);
    TS_ASSERT_DELTA( center[1], +4.0, 1e-6);
  }

  void test_copy_constructor()
  {
    IMDBoxTester<MDEvent<2>,2> b;
    b.setExtents(0, -10.0, 10.0);
    b.setExtents(1, -4.0, 6.0);
    b.setSignal(123.0);
    b.setErrorSquared(456.0);

    // Perform the copy
    IMDBoxTester<MDEvent<2>,2> box(b);
    TS_ASSERT_DELTA(box.getExtents(0).min, -10.0, 1e-6);
    TS_ASSERT_DELTA(box.getExtents(0).max, +10.0, 1e-6);
    TS_ASSERT_DELTA(box.getExtents(1).min, -4.0, 1e-6);
    TS_ASSERT_DELTA(box.getExtents(1).max, +6.0, 1e-6);
    TS_ASSERT_EQUALS( box.getSignal(), 123.0);
    TS_ASSERT_EQUALS( box.getErrorSquared(), 456.0);
  }



  /** Calculating volume and normalizing signal by it. */
  void test_calcVolume()
  {
    IMDBoxTester<MDEvent<2>,2> b;
    b.setExtents(0, -10.0, 10.0);
    b.setExtents(1, -4.0, 6.0);
    b.calcVolume();
    TS_ASSERT_DELTA( b.getVolume(), 200.0, 1e-5);
    TS_ASSERT_DELTA( b.getInverseVolume(), 1.0/200.0, 1e-5);

    b.setSignal(100.0);
    b.setErrorSquared(300.0);

    TS_ASSERT_DELTA( b.getSignal(), 100.0, 1e-5);
    TS_ASSERT_DELTA( b.getSignalNormalized(), 0.5, 1e-5);
    TS_ASSERT_DELTA( b.getErrorSquared(), 300.0, 1e-5);
    TS_ASSERT_DELTA( b.getErrorSquaredNormalized(), 1.5, 1e-5);
  }



  /** Get vertexes using the extents */
  void test_getVertexes()
  {
    IMDBoxTester<MDEvent<2>,2> b;
    b.setExtents(0, -10.0, 10.0);
    b.setExtents(1, -4.0, 6.0);
    std::vector<Mantid::Geometry::Coordinate> v = b.getVertexes();
    TS_ASSERT_EQUALS( v[0].getX(), -10.0);
    TS_ASSERT_EQUALS( v[0].getY(), -4.0);
    TS_ASSERT_EQUALS( v[1].getX(), 10.0);
    TS_ASSERT_EQUALS( v[1].getY(), -4.0);
    TS_ASSERT_EQUALS( v[2].getX(), -10.0);
    TS_ASSERT_EQUALS( v[2].getY(), 6.0);
    TS_ASSERT_EQUALS( v[3].getX(), 10.0);
    TS_ASSERT_EQUALS( v[3].getY(), 6.0);
  }

};


#endif /* MANTID_MDEVENTS_IMDBOXTEST_H_ */

