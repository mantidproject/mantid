#ifndef MDEVENTTEST_H
#define MDEVENTTEST_H

#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDEvent.h"
#include <cxxtest/TestSuite.h>
#include <map>
#include <memory>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::MDEvents;

class MDEventTest :    public CxxTest::TestSuite
{
public:
static MDEventTest *createSuite() { return new MDEventTest(); }
static void destroySuite(MDEventTest * suite) { delete suite; }    


  void test_simple_constructors()
  {
    MDEvent<3> a;
    TS_ASSERT_EQUALS( a.getNumDims(), 3);
    TS_ASSERT_EQUALS( a.getSignal(), 1.0);
    TS_ASSERT_EQUALS( a.getErrorSquared(), 1.0);
    TS_ASSERT_EQUALS( a.getRunIndex(), 0);
    TS_ASSERT_EQUALS( a.getDetectorID(), 0);

    MDEvent<4> b(2.5, 1.5);
    TS_ASSERT_EQUALS( b.getNumDims(), 4);
    TS_ASSERT_EQUALS( b.getSignal(), 2.5);
    TS_ASSERT_EQUALS( b.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( b.getRunIndex(), 0);
    TS_ASSERT_EQUALS( b.getDetectorID(), 0);

    // NOTE: The pragma (pack,2) call has no effect on some platforms: RHEL5, Ubuntu 10.04 and MacOS as of now.
    // Therefore these tests fail and the events are somewhat too big on these platforms:
    // TS_ASSERT_EQUALS( sizeof(a), sizeof(coord_t)*3+8+6);
    // TS_ASSERT_EQUALS( sizeof(b), sizeof(coord_t)*4+8+6);
  }

  void test_constructor()
  {
    MDEvent<3> b(2.5, 1.5, 123, 456789);
    TS_ASSERT_EQUALS( b.getNumDims(), 3);
    TS_ASSERT_EQUALS( b.getSignal(), 2.5);
    TS_ASSERT_EQUALS( b.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( b.getRunIndex(), 123);
    TS_ASSERT_EQUALS( b.getDetectorID(), 456789);
  }

  void test_constructor_withCoords()
  {
    // Fixed-size array
    coord_t coords[3] = {0.125, 1.25, 2.5};
    MDEvent<3> b(2.5, 1.5, 123, 456789, coords );
    TS_ASSERT_EQUALS( b.getSignal(), 2.5);
    TS_ASSERT_EQUALS( b.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( b.getCenter(0), 0.125);
    TS_ASSERT_EQUALS( b.getCenter(1), 1.25);
    TS_ASSERT_EQUALS( b.getCenter(2), 2.5);
    TS_ASSERT_EQUALS( b.getRunIndex(), 123);
    TS_ASSERT_EQUALS( b.getDetectorID(), 456789);
  }

  /** Note: the copy constructor is not explicitely written but rather is filled in by the compiler */
  void test_CopyConstructor()
  {
    coord_t coords[3] = {0.125, 1.25, 2.5};
    MDEvent<3> b(2.5, 1.5, 123, 456789, coords );
    MDEvent<3> a(b);
    TS_ASSERT_EQUALS( a.getNumDims(), 3);
    TS_ASSERT_EQUALS( a.getSignal(), 2.5);
    TS_ASSERT_EQUALS( a.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( b.getCenter(0), 0.125);
    TS_ASSERT_EQUALS( b.getCenter(1), 1.25);
    TS_ASSERT_EQUALS( b.getCenter(2), 2.5);
    TS_ASSERT_EQUALS( a.getRunIndex(), 123);
    TS_ASSERT_EQUALS( a.getDetectorID(), 456789);
  }

  void test_serialize_deserializeLean()
  {
      size_t nPoints=99; // the number should not be nPoints%4=0 to hold test TS_ASSERT_THROWS below
      std::vector<MDLeanEvent<3> > events(nPoints);
      double sumGuess(0),errGuess(0);
      for(size_t i=0;i<nPoints;i++)
      {

          events[i].setSignal(static_cast<float>(i));
          events[i].setErrorSquared(static_cast<float>(i*i));
          sumGuess+=double(i);
          errGuess+=double(i*i);
          events[i].setCenter(0,0.1*static_cast<double>(i));
          events[i].setCenter(1,static_cast<double>(i));
          events[i].setCenter(2,10*static_cast<double>(i));

      }

      std::vector<coord_t> data;
      size_t ncols;
      double totalSignal(0);
      double totalErrSq(0); 
      TS_ASSERT_THROWS_NOTHING(MDLeanEvent<3>::eventsToData(events,data,ncols,totalSignal,totalErrSq));
      TS_ASSERT_EQUALS(3+2,ncols);
      TS_ASSERT_EQUALS((3+2)*nPoints,data.size());
      TS_ASSERT_DELTA(sumGuess,totalSignal,1.e-7);
      TS_ASSERT_DELTA(errGuess,totalErrSq,1.e-7);

      for(size_t i=0;i<nPoints;i++)
      {
          TS_ASSERT_DELTA(events[i].getSignal(),data[ncols*i+0],1.e-6);
          TS_ASSERT_DELTA(events[i].getErrorSquared(),data[ncols*i+1],1.e-6);
          TS_ASSERT_DELTA(events[i].getCenter(0),data[ncols*i+2],1.e-6);
          TS_ASSERT_DELTA(events[i].getCenter(1),data[ncols*i+3],1.e-6);
          TS_ASSERT_DELTA(events[i].getCenter(2),data[ncols*i+4],1.e-6);
      }


      std::vector<MDLeanEvent<4> > transfEvents4;
      TS_ASSERT_THROWS(MDLeanEvent<4>::dataToEvents(data,transfEvents4),std::invalid_argument);

      std::vector<MDLeanEvent<3> > transfEvents;
      TS_ASSERT_THROWS_NOTHING(MDLeanEvent<3>::dataToEvents(data,transfEvents));
      for(size_t i=0;i<nPoints;i++)
      {
          TS_ASSERT_DELTA(events[i].getSignal(),transfEvents[i].getSignal(),1.e-6);
          TS_ASSERT_DELTA(events[i].getErrorSquared(),transfEvents[i].getErrorSquared(),1.e-6);
          TS_ASSERT_DELTA(events[i].getCenter(0),transfEvents[i].getCenter(0),1.e-6);
          TS_ASSERT_DELTA(events[i].getCenter(1),transfEvents[i].getCenter(1),1.e-6);
          TS_ASSERT_DELTA(events[i].getCenter(2),transfEvents[i].getCenter(2),1.e-6);
      }
      /// test append
      transfEvents.reserve(2*nPoints);
      TS_ASSERT_THROWS_NOTHING(MDLeanEvent<3>::dataToEvents(data,transfEvents,false));
      TS_ASSERT_EQUALS(2*nPoints,transfEvents.size());
      for(size_t i=0;i<nPoints;i++)
      {
          TS_ASSERT_DELTA(transfEvents[i].getSignal(),transfEvents[nPoints+i].getSignal(),1.e-6);
          TS_ASSERT_DELTA(transfEvents[i].getErrorSquared(),transfEvents[nPoints+i].getErrorSquared(),1.e-6);
          TS_ASSERT_DELTA(transfEvents[i].getCenter(0),transfEvents[nPoints+i].getCenter(0),1.e-6);
          TS_ASSERT_DELTA(transfEvents[i].getCenter(1),transfEvents[nPoints+i].getCenter(1),1.e-6);
          TS_ASSERT_DELTA(transfEvents[i].getCenter(2),transfEvents[nPoints+i].getCenter(2),1.e-6);
      }



  }
  void test_serialize_deserializeFat()
  {
      size_t nPoints=100; // the number should not be nPoints%3=0 to hold test TS_ASSERT_THROWS below
      std::vector<MDEvent<4> > events(nPoints);
      double sumGuess(0),errGuess(0);
      for(size_t i=0;i<nPoints;i++)
      {

          events[i].setSignal(static_cast<float>(i));
          events[i].setErrorSquared(static_cast<float>(i*i));
          events[i].setDetectorId(uint32_t(i));
          events[i].setRunIndex(uint16_t(i/10));
          sumGuess+=double(i);
          errGuess+=double(i*i);
          events[i].setCenter(0,0.1*static_cast<double>(i));
          events[i].setCenter(1,static_cast<double>(i));
          events[i].setCenter(2,10*static_cast<double>(i));
          events[i].setCenter(3,100*static_cast<double>(i));

      }

      std::vector<coord_t> data;
      size_t ncols;
      double totalSignal(0);
      double totalErrSq(0); 
      TS_ASSERT_THROWS_NOTHING(MDEvent<4>::eventsToData(events,data,ncols,totalSignal,totalErrSq));
      TS_ASSERT_EQUALS(4+4,ncols);
      TS_ASSERT_EQUALS((4+4)*nPoints,data.size());
      TS_ASSERT_DELTA(sumGuess,totalSignal,1.e-7);
      TS_ASSERT_DELTA(errGuess,totalErrSq,1.e-7);

      for(size_t i=0;i<nPoints;i++)
      {
          TS_ASSERT_DELTA(events[i].getSignal(),data[ncols*i+0],1.e-6);
          TS_ASSERT_DELTA(events[i].getErrorSquared(),data[ncols*i+1],1.e-6);
          TS_ASSERT_EQUALS(events[i].getRunIndex(),uint16_t(data[ncols*i+2]));
          TS_ASSERT_EQUALS(events[i].getDetectorID(),uint32_t(data[ncols*i+3]));

          TS_ASSERT_DELTA(events[i].getCenter(0),data[ncols*i+4],1.e-6);
          TS_ASSERT_DELTA(events[i].getCenter(1),data[ncols*i+5],1.e-6);
          TS_ASSERT_DELTA(events[i].getCenter(2),data[ncols*i+6],1.e-6);
          TS_ASSERT_DELTA(events[i].getCenter(3),data[ncols*i+7],1.e-6);
      }


      std::vector<MDEvent<3> > transfEvents3;
      TS_ASSERT_THROWS(MDEvent<3>::dataToEvents(data,transfEvents3),std::invalid_argument);

      std::vector<MDEvent<4> > transfEvents;
      TS_ASSERT_THROWS_NOTHING(MDEvent<4>::dataToEvents(data,transfEvents));
      for(size_t i=0;i<nPoints;i++)
      {
          TS_ASSERT_DELTA(events[i].getSignal(),transfEvents[i].getSignal(),1.e-6);
          TS_ASSERT_DELTA(events[i].getErrorSquared(),transfEvents[i].getErrorSquared(),1.e-6);
          TS_ASSERT_EQUALS(events[i].getRunIndex(),transfEvents[i].getRunIndex());
          TS_ASSERT_EQUALS(events[i].getDetectorID(),transfEvents[i].getDetectorID());

          TS_ASSERT_DELTA(events[i].getCenter(0),transfEvents[i].getCenter(0),1.e-6);
          TS_ASSERT_DELTA(events[i].getCenter(1),transfEvents[i].getCenter(1),1.e-6);
          TS_ASSERT_DELTA(events[i].getCenter(2),transfEvents[i].getCenter(2),1.e-6);
          TS_ASSERT_DELTA(events[i].getCenter(3),transfEvents[i].getCenter(3),1.e-6);
      }

      /// test append
      transfEvents.reserve(2*nPoints);
      TS_ASSERT_THROWS_NOTHING(MDEvent<4>::dataToEvents(data,transfEvents,false));
      TS_ASSERT_EQUALS(2*nPoints,transfEvents.size());
      for(size_t i=0;i<nPoints;i++)
      {
          TS_ASSERT_DELTA(transfEvents[i].getSignal(),transfEvents[nPoints+i].getSignal(),1.e-6);
          TS_ASSERT_DELTA(transfEvents[i].getErrorSquared(),transfEvents[nPoints+i].getErrorSquared(),1.e-6);
          TS_ASSERT_DELTA(transfEvents[i].getCenter(0),transfEvents[nPoints+i].getCenter(0),1.e-6);
          TS_ASSERT_DELTA(transfEvents[i].getCenter(1),transfEvents[nPoints+i].getCenter(1),1.e-6);
          TS_ASSERT_DELTA(transfEvents[i].getCenter(2),transfEvents[nPoints+i].getCenter(2),1.e-6);
          TS_ASSERT_DELTA(transfEvents[i].getCenter(3),transfEvents[nPoints+i].getCenter(3),1.e-6);
      }



  }


};


class MDEventTestPerformance :  public CxxTest::TestSuite
{
public:
  std::vector<MDEvent<3> > events3;
  std::vector<MDLeanEvent<3> > lean_events3;
  std::vector<MDEvent<4> > events4;
  std::vector<MDLeanEvent<4> > lean_events4;
  size_t num;

  void setUp()
  {
    num = 1000000;
    events3.clear(); events3.reserve(num);
    events4.clear(); events4.reserve(num);
    lean_events3.clear(); lean_events3.reserve(num);
    lean_events4.clear(); lean_events4.reserve(num);
  }

  void test_create_MDEvent3()
  {
    float signal(1.5);
    float error(2.5);
    uint16_t runIndex = 123;
    uint16_t detectorId = 45678;
    coord_t center[3] = {1.25, 2.5, 3.5};
    for (size_t i=0; i<num; i++)
      events3.push_back( MDEvent<3>(signal, error, runIndex, detectorId, center) );
  }

  void test_create_MDEvent4()
  {
    float signal(1.5);
    float error(2.5);
    uint16_t runIndex = 123;
    uint16_t detectorId = 45678;
    coord_t center[4] = {1.25, 2.5, 3.5, 4.75};
    for (size_t i=0; i<num; i++)
      events4.push_back( MDEvent<4>(signal, error, runIndex, detectorId, center) );
  }

  void test_create_MDLeanEvent3()
  {
    float signal(1.5);
    float error(2.5);
    coord_t center[3] = {1.25, 2.5, 3.5};
    for (size_t i=0; i<num; i++)
      lean_events3.push_back( MDLeanEvent<3>(signal, error, center) );
  }

  void test_create_MDLeanEvent4()
  {
    float signal(1.5);
    float error(2.5);
    coord_t center[4] = {1.25, 2.5, 3.5, 4.75};
    for (size_t i=0; i<num; i++)
      lean_events4.push_back( MDLeanEvent<4>(signal, error, center) );
  }


  void test_serialize_deserializeLean()
  {
      size_t nPoints=num;
      std::vector<MDLeanEvent<3> > events(nPoints);
      double sumGuess(0),errGuess(0);
      for(size_t i=0;i<nPoints;i++)
      {

          events[i].setSignal(static_cast<float>(i));
          events[i].setErrorSquared(static_cast<float>(i*i));
          sumGuess+=double(i);
          errGuess+=double(i*i);
          events[i].setCenter(0,0.1*static_cast<double>(i));
          events[i].setCenter(1,static_cast<double>(i));
          events[i].setCenter(2,10*static_cast<double>(i));

      }

      std::vector<coord_t> data;
      size_t ncols;
      double totalSignal(0);
      double totalErrSq(0); 
      TS_ASSERT_THROWS_NOTHING(MDLeanEvent<3>::eventsToData(events,data,ncols,totalSignal,totalErrSq));
      TS_ASSERT_EQUALS(3+2,ncols);
      TS_ASSERT_EQUALS((3+2)*nPoints,data.size());
      double relerr = 2*std::fabs(sumGuess-totalSignal)/(sumGuess+totalSignal);
      TS_ASSERT_DELTA(0.,relerr,1.e-7);
      relerr = 2*std::fabs(errGuess-totalErrSq)/(errGuess+totalErrSq);
      TS_ASSERT_DELTA(0,relerr,1.e-7);


      std::vector<MDLeanEvent<3> > transfEvents;
      TS_ASSERT_THROWS_NOTHING(MDLeanEvent<3>::dataToEvents(data,transfEvents));
  }
  void test_serialize_deserializeFat()
  {
      size_t nPoints=num; 
      std::vector<MDEvent<4> > events(nPoints);
      double sumGuess(0),errGuess(0);
      for(size_t i=0;i<nPoints;i++)
      {

          events[i].setSignal(static_cast<float>(i));
          events[i].setErrorSquared(static_cast<float>(i*i));
          events[i].setDetectorId(uint32_t(i));
          events[i].setRunIndex(uint16_t(i/10));
          sumGuess+=double(i);
          errGuess+=double(i*i);
          events[i].setCenter(0,0.1*static_cast<double>(i));
          events[i].setCenter(1,static_cast<double>(i));
          events[i].setCenter(2,10*static_cast<double>(i));
          events[i].setCenter(3,100*static_cast<double>(i));

      }

      std::vector<coord_t> data;
      size_t ncols;
      double totalSignal(0);
      double totalErrSq(0); 
      TS_ASSERT_THROWS_NOTHING(MDEvent<4>::eventsToData(events,data,ncols,totalSignal,totalErrSq));
      TS_ASSERT_EQUALS(4+4,ncols);
      TS_ASSERT_EQUALS((4+4)*nPoints,data.size());

      double relerr = 2*std::fabs(sumGuess-totalSignal)/(sumGuess+totalSignal);
      TS_ASSERT_DELTA(0.,relerr,1.e-7);
      relerr = 2*std::fabs(errGuess-totalErrSq)/(errGuess+totalErrSq);
      TS_ASSERT_DELTA(0,relerr,1.e-7);


      std::vector<MDEvent<4> > transfEvents;
      TS_ASSERT_THROWS_NOTHING(MDEvent<4>::dataToEvents(data,transfEvents));

  }


};

#endif
