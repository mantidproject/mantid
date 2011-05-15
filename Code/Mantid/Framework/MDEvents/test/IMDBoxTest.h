#ifndef MANTID_MDEVENTS_IMDBOXTEST_H_
#define MANTID_MDEVENTS_IMDBOXTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/IMDBox.h"

using namespace Mantid::MDEvents;


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

};


#endif /* MANTID_MDEVENTS_IMDBOXTEST_H_ */

