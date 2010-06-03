/*
 * EventWorkspaceTest.h
 *
 *  Created on: May 28, 2010
 *      Author: Janik Zikovsky
 */

#ifndef EVENTWORKSPACETEST_H_
#define EVENTWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::Kernel::Exception;
using namespace Mantid::API;

using std::runtime_error;
using std::size_t;
using std::vector;
using std::cout;


//==========================================================================================
class EventWorkspaceTest : public CxxTest::TestSuite
{
private:
  EventWorkspace_sptr ew;

public:
  EventWorkspaceTest()
  {
  }

//
//  /** Create event workspace with:
//   * 500 pixels
//   * 1000 histogrammed bins.
//   */
//  EventWorkspace_sptr createEventWorkspace()
//  {
//    EventWorkspace_sptr retVal(new EventWorkspace);
//    retVal->initialize(500,1,1);
//
//    //Create the x-axis for histogramming.
//    Kernel::cow_ptr<MantidVec> axis;
//    MantidVec& xRef = axis.access();
//    int bins = 1000;
//    double delta = 1e3;
//    xRef.resize(bins);
//    for (int i = 0; i < bins; ++i)
//      xRef[i] = i*delta;
//
//    //Try setting a single axis
//    retVal->setX(2, axis);
//
//    //Set all the histograms at once.
//    retVal->setAllX(axis);
//
//    return retVal;
//  }
//
//  void setUp()
//  {
//    ew = createEventWorkspace();
//  }
//
//  void test_constructor()
//  {
//    TS_ASSERT_EQUALS( ew->getNumberHistograms(), 500);
//    //TS_ASSERT_EQUALS( ew->blocksize(), 1000);
//    //std::cout << ew.size();
//    const EventList el(ew->getEventList(1));
//    //TS_ASSERT( el.dataX()[0] == 0);
//  }
//
//  void test_data_access()
//  {
//    //Non-const access throws errors
//    TS_ASSERT_THROWS( ew->dataX(1), NotImplementedError );
//    TS_ASSERT_THROWS( ew->dataY(2), NotImplementedError );
//    TS_ASSERT_THROWS( ew->dataE(3), NotImplementedError );
//    //Out of range
//    TS_ASSERT_THROWS( ew->dataX(-123), std::range_error );
//    TS_ASSERT_THROWS( ew->dataX(5123), std::range_error );
//
//    //Can't try the const access; copy constructors are not allowed.
//  }
//
//  void test_const()
//  {
//    /*const EventWorkspace ewc;
//    cow_ptr<MantidVec> x_ptr;
//    ewc.setX(x_ptr);
//    std::cout << ewc.dataX(1)[0];
//    */
//
//  }
//
//  void test_setX()
//  {
//    /*
//    MantidVec myX;
//    double tof; //in ns
//    for (tof=0; tof<16e3*1e3; tof += 1e4)
//    {
//      //bins of 10 microsec
//      myX.push_back(tof);
//    }
//    Kernel::cow_ptr<MantidVec> x_ptr;
//    x_ptr = &myX;
//    ew.setX(0, x_ptr);
//    //MantidVec mydata = ew.dataX(1);
//    TS_ASSERT_THROWS( MantidVec & mydata = ew.dataX(1), Exception::NotImplementedError);
//    */
//  }

};




#endif /* EVENTWORKSPACETEST_H_ */
