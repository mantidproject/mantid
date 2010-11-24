#ifndef EVENTLISTTEST_H_
#define EVENTLISTTEST_H_ 1

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include <cmath>
#include <boost/math/special_functions/fpclassify.hpp>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

using std::runtime_error;
using std::size_t;
using std::vector;

//==========================================================================================
class TofEventTest : public CxxTest::TestSuite
{
private:
  TofEvent e;

public:
  TofEventTest()
  {
    e = TofEvent(123, 456);
  }

  void testInit()
  {
    TS_ASSERT_EQUALS(e.tof(), 123);
    TS_ASSERT_EQUALS(e.pulseTime(), 456);
  }

  void testAssign()
  {
    TofEvent e2;
    e2 = e;
    TS_ASSERT_EQUALS(e2.tof(), 123);
    TS_ASSERT_EQUALS(e2.pulseTime(), 456);
  }

  void testConstructors()
  {
    TofEvent e2 = TofEvent(e);
    TS_ASSERT_EQUALS(e2.tof(), 123);
    TS_ASSERT_EQUALS(e2.pulseTime(), 456);

    TofEvent e3 = TofEvent(890.234, 321);
    TS_ASSERT_EQUALS(e3.tof(), 890.234);
    TS_ASSERT_EQUALS(e3.pulseTime(), 321);
  }

};


//==========================================================================================
class WeightedEventTest : public CxxTest::TestSuite
{
private:

public:
  WeightedEventTest()
  {
  }

  void testConstructors()
  {
    TofEvent e(123, 456);
    WeightedEvent we, we2;

    //Empty
    we = WeightedEvent();
    TS_ASSERT_EQUALS(we.tof(), 0);
    TS_ASSERT_EQUALS(we.pulseTime(), 0);
    TS_ASSERT_EQUALS(we.weight(), 1.0);
    TS_ASSERT_EQUALS(we.error(), 1.0);

    //Default one weight
    we = WeightedEvent(e);
    TS_ASSERT_EQUALS(we.tof(), 123);
    TS_ASSERT_EQUALS(we.pulseTime(), 456);
    TS_ASSERT_EQUALS(we.weight(), 1.0);
    TS_ASSERT_EQUALS(we.error(), 1.0);

    //TofEvent + weights
    we = WeightedEvent(e, 3.5, 0.5*0.5);
    TS_ASSERT_EQUALS(we.tof(), 123);
    TS_ASSERT_EQUALS(we.pulseTime(), 456);
    TS_ASSERT_EQUALS(we.weight(), 3.5);
    TS_ASSERT_EQUALS(we.error(), 0.5);

    //Full constructor
    we = WeightedEvent(456, 789, 2.5, 1.5*1.5);
    TS_ASSERT_EQUALS(we.tof(), 456);
    TS_ASSERT_EQUALS(we.pulseTime(), 789);
    TS_ASSERT_EQUALS(we.weight(), 2.5);
    TS_ASSERT_EQUALS(we.error(), 1.5);
  }

  void testAssignAndCopy()
  {
    WeightedEvent we, we2;

    //Copy constructor
    we = WeightedEvent();
    we2 = WeightedEvent(456, 789, 2.5, 1.5*1.5);
    we = we2;
    TS_ASSERT_EQUALS(we.tof(), 456);
    TS_ASSERT_EQUALS(we.pulseTime(), 789);
    TS_ASSERT_EQUALS(we.weight(), 2.5);
    TS_ASSERT_EQUALS(we.error(), 1.5);
  }


};




//==========================================================================================
class EventListTest : public CxxTest::TestSuite
{
private:
  EventList el;
  static const int NUMEVENTS = 100;
  static const int MAX_TOF = 10e6;
  static const int NUMBINS = 160;
  int BIN_DELTA;


public:
  EventListTest()
  {
    BIN_DELTA = 10000;
  }

  void setUp()
  {
    //Make a little event list with 3 events
    vector<TofEvent> mylist;
    mylist.push_back(TofEvent(100,200));
    mylist.push_back(TofEvent(3.5, 400));
    mylist.push_back(TofEvent(50,60));
    el = EventList(mylist);
  }

  //==================================================================================
  //--- Basics  ----
  //==================================================================================

  void testInit()
  {
    vector<TofEvent> rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 3);
    TS_ASSERT_EQUALS(rel[0].tof(), 100);
    TS_ASSERT_EQUALS(rel[0].pulseTime(), 200);
    TS_ASSERT_EQUALS(rel[2].tof(), 50);
  }

  //==================================================================================
  //--- Plus Operators  ----
  //==================================================================================

  void testPlusOperator()
  {
    vector<TofEvent> mylist;
    mylist.push_back(TofEvent(45,67));
    mylist.push_back(TofEvent(89,12));
    mylist.push_back(TofEvent(34,56));
    el += mylist;
    vector<TofEvent> rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 6);
    TS_ASSERT_EQUALS(rel[3].tof(), 45);
    TS_ASSERT_EQUALS(rel[5].tof(), 34);

    el += TofEvent(999, 888);
    rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 7);
    TS_ASSERT_EQUALS(rel[6].tof(), 999);

    EventList el2;
    el2 += TofEvent(1,2);
    el2 += TofEvent(3,4);
    el += el2;
    rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 9);
    el += el;
    rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 18);

    el.addEventQuickly( TofEvent(333, 444));
    rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 19);
  }

  void testPlusOperator2()
  {
    vector<TofEvent> rel;
    el += el;
    rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 6);
    TS_ASSERT_EQUALS(rel[3].tof(), 100);
    TS_ASSERT_EQUALS(rel[5].tof(), 50);
  }

  void testDetectorIDs()
  {
    EventList el1;
    el1.addDetectorID( 14 );
    TS_ASSERT_EQUALS( el1.getDetectorIDs().size(), 1);
    el1.addDetectorID( 21 );
    TS_ASSERT_EQUALS( el1.getDetectorIDs().size(), 2);
    el1.addDetectorID( 21 );
    TS_ASSERT_EQUALS( el1.getDetectorIDs().size(), 2);

    EventList el2;
    el2.addDetectorID( 7 );
    el2.addDetectorID( 14 );
    el2.addDetectorID( 28 );
    TS_ASSERT_EQUALS( el2.getDetectorIDs().size(), 3);

    //One detID was repeated, so it doesn't appear twice
    el2 += el1;
    TS_ASSERT_EQUALS( el2.getDetectorIDs().size(), 4);
    //Find the right stuff
    for (int i=7; i< 35; i+=7)
      TS_ASSERT( el2.hasDetectorID(i) );
    TS_ASSERT( !el2.hasDetectorID(0) );
  }


  //==================================================================================
  //--- Switching to Weighted Events ----
  //==================================================================================

  //----------------------------------
  void test_switchToWeightedEvents()
  {
    //Start with a bit of fake data
    this->fake_data();
    TS_ASSERT_EQUALS( el.getEvents().size(), NUMEVENTS );
    TS_ASSERT_EQUALS( el.getNumberEvents(), NUMEVENTS);
    TS_ASSERT_THROWS( el.getWeightedEvents().size(), std::runtime_error);

    el.switchToWeightedEvents();
    TS_ASSERT_THROWS( el.getEvents().size(), std::runtime_error);
    TS_ASSERT_EQUALS( el.getWeightedEvents().size(), NUMEVENTS );
    TS_ASSERT_EQUALS( el.getNumberEvents(), NUMEVENTS);
    TS_ASSERT_EQUALS( el.getWeightedEvents()[0].weight(), 1.0 );
    TS_ASSERT_EQUALS( el.getWeightedEvents()[0].error(), 1.0 );
  }


  //----------------------------------
  void test_switch_on_the_fly()
  {
    fake_data();
    TS_ASSERT( !el.hasWeights() );

    // Add a weighted event = everything switches
    WeightedEvent we(123, 456, 2.0, 3.0*3.0);
    el += we;
    TS_ASSERT( el.hasWeights() );
    TS_ASSERT_EQUALS( el.getWeightedEvents()[0].weight(), 1.0 );
    TS_ASSERT_EQUALS( el.getWeightedEvents()[0].error(), 1.0 );
    //New one is at the end
    TS_ASSERT_EQUALS( el.getWeightedEvents()[NUMEVENTS], we );

    //But you can still add a plain one
    TofEvent e(789, 654);
    el += e;
    TS_ASSERT_EQUALS( el.getWeightedEvents()[NUMEVENTS+1], e );
    TS_ASSERT_EQUALS( el.getWeightedEvents()[NUMEVENTS+1].weight(), 1.0 );

  }


  //----------------------------------
  void test_switch_on_the_fly_when_appending_lists_1_none_plus_weights()
  {
    TS_ASSERT( !el.hasWeights() );
    vector<WeightedEvent> mylist;
    mylist.push_back(WeightedEvent(45,67, 4.5, 6.5*6.5));
    mylist.push_back(WeightedEvent(89,12, 1.0, 1.0*1.0));
    mylist.push_back(WeightedEvent(34,56, 3.0, 2.0*2.0));

    el += mylist;
    TS_ASSERT( el.hasWeights() );
    vector<WeightedEvent> rel = el.getWeightedEvents();
    TS_ASSERT_EQUALS(rel.size(), 6);
    TS_ASSERT_EQUALS(rel[3].tof(), 45);
    TS_ASSERT_EQUALS(rel[3].weight(), 4.5);
    TS_ASSERT_EQUALS(rel[5].tof(), 34);
    TS_ASSERT_EQUALS(rel[5].error(), 2.0);
  }

  //----------------------------------
  void test_switch_on_the_fly_when_appending_lists2_none_plus_weights()
  {
    TS_ASSERT( !el.hasWeights() );
    EventList el2 = el;
    el2.switchToWeightedEvents();
    el += el2;

    TS_ASSERT( el.hasWeights() );
    vector<WeightedEvent> rel = el.getWeightedEvents();
    TS_ASSERT_EQUALS(rel.size(), 6);
    TS_ASSERT_EQUALS(rel[3].weight(), 1.0);
    TS_ASSERT_EQUALS(rel[5].error(), 1.0);
  }


  //----------------------------------
  void test_switch_on_the_fly_when_appending_lists3_weights_plus_none()
  {
    TS_ASSERT( !el.hasWeights() );
    EventList el2 = el;
    el2.switchToWeightedEvents();
    el2 += el;
    TS_ASSERT( el2.hasWeights() );
    vector<WeightedEvent> rel = el2.getWeightedEvents();
    TS_ASSERT_EQUALS(rel.size(), 6);
    TS_ASSERT_EQUALS(rel[3].weight(), 1.0);
    TS_ASSERT_EQUALS(rel[5].error(), 1.0);
  }

  //----------------------------------
  void test_switch_on_the_fly_when_appending_lists4_weights_plus_weights()
  {
    EventList el2 = el;
    el2.switchToWeightedEvents();
    el2 += el2;
    TS_ASSERT( el2.hasWeights() );
    vector<WeightedEvent> rel = el2.getWeightedEvents();
    TS_ASSERT_EQUALS(rel.size(), 6);
    TS_ASSERT_EQUALS(rel[3].weight(), 1.0);
    TS_ASSERT_EQUALS(rel[5].error(), 1.0);
  }



  //==================================================================================
  //--- Multiplying  ----
  //==================================================================================

  void test_multiply_scalar_simple()
  {
    //No weights
    this->fake_uniform_data();
    //Perform the multiply; no error on the scalar
    el.multiply(2.0, 0.0);
    TS_ASSERT_DELTA( el.getWeightedEvents()[0].weight(), 2.0, 1e-5);
    TS_ASSERT_DELTA( el.getWeightedEvents()[0].error(), 2.0, 1e-5);

    this->fake_uniform_data();
    //Multiply by zero with error
    el.multiply(0.0, 1.0);
    TS_ASSERT_DELTA( el.getWeightedEvents()[0].weight(), 0.0, 1e-5);
    // Gives zero error. Relative error no longer has a meaning.
    TS_ASSERT_DELTA( el.getWeightedEvents()[0].error(), 0.0, 1e-5);
  }

  void test_multiply_by_one_doesnt_give_weights()
  {
    //No weights
    this->fake_uniform_data();
    //Perform the multiply by one without error.
    el.multiply(1.0, 0.0);
    TS_ASSERT( !el.hasWeights() );
  }


  void test_multiply_scalar()
  {
    //Weight 2, error (2.5)
    this->fake_uniform_data_weights();
    //Perform the multiply
    el.multiply(2.0, 0.5);

    TS_ASSERT_DELTA( el.getWeightedEvents()[0].weight(), 4.0, 1e-5);
    //Error^2 = 2*2.5^2/2 + 2*(0.5^2) / 2  = 6.5
    TS_ASSERT_DELTA( el.getWeightedEvents()[0].errorSquared(), 6.5, 1e-5);

    //Try it with no scalar error
    this->fake_uniform_data_weights();
    el.multiply(2.0);
    TS_ASSERT_DELTA( el.getWeightedEvents()[0].weight(), 4.0, 1e-5);
    TS_ASSERT_DELTA( el.getWeightedEvents()[0].error(), 1.25*4.0, 1e-5);

    // *= operator
    this->fake_uniform_data_weights();
    el *= 2.0;
    TS_ASSERT_DELTA( el.getWeightedEvents()[0].weight(), 4.0, 1e-5);
    TS_ASSERT_DELTA( el.getWeightedEvents()[0].error(), 1.25*4.0, 1e-5);
  }




  void test_multiply_histogram()
  {

    //Make the histogram we are multiplying.
    MantidVec X, Y, E;
    // one tenth of the # of bins
    double step = BIN_DELTA*10;
    for (double tof=step; tof<BIN_DELTA*(NUMBINS+1); tof += step)
    {
      X.push_back(tof);
      //std::cout << tof << "\n";
    }
    for (int i=0; i < X.size()-1; i++)
    {
      Y.push_back( 1.0 * (i+1));
      E.push_back( sqrt(double(1.0 * (i+1))) );
    }

    //Make the data and multiply
    this->fake_uniform_data_weights();
    el.multiply(X, Y, E);

    vector<WeightedEvent> & rwel = el.getWeightedEvents();
    for (int i=0; i < rwel.size(); i++)
    {
      double tof = rwel[i].tof();
      if (tof>=step && tof<BIN_DELTA*NUMBINS)
      {
        int bini = tof / step;
        double value = bini;
        double errorsquared = value;
        //Check the formulae for value and error
        TS_ASSERT_DELTA( rwel[i].weight(), 2.0*value, 1e-6);
        TS_ASSERT_DELTA( rwel[i].errorSquared(), 2.5*2.5*value/2.0 + 2.0 * errorsquared / value, 1e-6);
      }
    }
  }

  void test_divide_scalar_simple()
  {
    this->fake_uniform_data();
    el.divide(2.0, 0.0);
    TS_ASSERT_DELTA( el.getWeightedEvents()[0].weight(), 0.5, 1e-5);
    TS_ASSERT_DELTA( el.getWeightedEvents()[0].error(), 0.5, 1e-5);

    this->fake_uniform_data();
    el.divide(2.0);
    TS_ASSERT_DELTA( el.getWeightedEvents()[0].weight(), 0.5, 1e-5);
    TS_ASSERT_DELTA( el.getWeightedEvents()[0].error(), 0.5, 1e-5);
  }


  void test_divide_scalar()
  {
    //Weight 2, error 2.5
    this->fake_uniform_data_weights();
    el.divide(2.0, 0.5);

    TS_ASSERT_DELTA( el.getWeightedEvents()[0].weight(), 1.0, 1e-5);
    // Relative errors sum, so (sqrt(2.5)/2)^2+0.25^2 = 1.625; error is sqrt(1.625 * 1.0)
    TS_ASSERT_DELTA( el.getWeightedEvents()[0].error(), sqrt(1.625), 1e-5);

    //Try it with no scalar error
    this->fake_uniform_data_weights();
    el.divide(2.0);
    TS_ASSERT_DELTA( el.getWeightedEvents()[0].weight(), 1.0, 1e-5);
    //Same relative error of 1.25
    TS_ASSERT_DELTA( el.getWeightedEvents()[0].error(), 1.25, 1e-5);

    // *= operator
    this->fake_uniform_data_weights();
    el /= 2.0;
    TS_ASSERT_DELTA( el.getWeightedEvents()[0].weight(), 1.0, 1e-5);
    TS_ASSERT_DELTA( el.getWeightedEvents()[0].error(), 1.25, 1e-5);
  }

  void test_divide_by_zero()
  {
    //Perform the multiply
    TS_ASSERT_THROWS( el.divide(0.0, 0.5), std::invalid_argument);
    TS_ASSERT_THROWS( el.divide(0.0), std::invalid_argument);
    TS_ASSERT_THROWS( el /= 0, std::invalid_argument);
  }


  void test_divide_histogram()
  {

    //Make the histogram we are multiplying.
    MantidVec X, Y, E;
    // one tenth of the # of bins
    double step = BIN_DELTA*10;
    for (double tof=step; tof<BIN_DELTA*(NUMBINS+1); tof += step)
    {
      X.push_back(tof);
      //std::cout << tof << "\n";
    }
    for (int i=0; i < X.size()-1; i++)
    {
      //Have one zero bin in there
      if (i == 6)
        Y.push_back( 0.0 );
      else
        Y.push_back( 2.0 );
      E.push_back( 0.5 );
    }

    //Make the data and multiply
    this->fake_uniform_data_weights();
    el.divide(X, Y, E);

    vector<WeightedEvent> & rwel = el.getWeightedEvents();
    for (int i=0; i < rwel.size(); i++)
    {
      double tof = rwel[i].tof();
      if (tof>=step && tof<BIN_DELTA*NUMBINS)
      {
        int bini = tof / step;
        double value = bini;
        double errorsquared = value;
        if (bini == 7)
        {
          //That was zeros
          TS_ASSERT( boost::math::isnan(rwel[i].weight()) );
          TS_ASSERT( boost::math::isnan(rwel[i].errorSquared()) );
        }
        else
        {
          //Same weight error as dividing by a scalar before, since we divided by 2+-0.5 again
          TS_ASSERT_DELTA( rwel[i].weight(), 1.0, 1e-5);
          TS_ASSERT_DELTA( rwel[i].error(), sqrt(1.625), 1e-5);
        }
      }
    }
  }


  //==================================================================================
  //--- Minus Operation ----
  //==================================================================================

  /// Make a big bin holding all events
  MantidVecPtr one_big_bin()
  {
    //Generate the histrogram bins
    MantidVecPtr x;
    MantidVec & shared_x = x.access();
    shared_x.push_back(0);
    shared_x.push_back(1e10);
    return x;
  }

  void testMinusOperator()
  {
    this->fake_uniform_data();
    EventList el2(el); //An EventList with 1.0 weights implied
    int num2 = el2.getNumberEvents();

    this->fake_uniform_data();
    int num1 = el2.getNumberEvents();

    //Subtract the non-weighted from the weighted
    el -= el2;

    TS_ASSERT_EQUALS( el.getNumberEvents(), num1+num2 );

    //Put a single big bin with all events
    el.setX(one_big_bin() );
    //But the total neutrons is 0.0! They've been cancelled out :)
    TS_ASSERT_DELTA( (*el.dataY())[0], 0.0, 1e-6);
    TS_ASSERT_DELTA( (*el.dataE())[0], sqrt((double)el.getNumberEvents()), 1e-6);

  }


  //==================================================================================
  //--- Sorting Tests ---
  //==================================================================================

  void testSortTOF()
  {
    int i;

    el.sortTof();
    vector<TofEvent> rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 3);
    TS_ASSERT_EQUALS(rel[0].tof(), 3.5);
    TS_ASSERT_EQUALS(rel[1].tof(), 50);
    TS_ASSERT_EQUALS(rel[2].tof(), 100);

    this->fake_data();
    el.sort(TOF_SORT);
    rel = el.getEvents();
    for (i=1; i<100; i++)
    {
      TS_ASSERT_LESS_THAN_EQUALS(rel[i-1].tof(), rel[i].tof());
    }
  }

  void testSortTOF_weights()
  {
    this->fake_data();
    el.switchToWeightedEvents();
    el.sort(TOF_SORT);
    vector<WeightedEvent> rwel = el.getWeightedEvents();
    for (int i=1; i<100; i++)
    {
      TS_ASSERT_LESS_THAN_EQUALS(rwel[i-1].tof(), rwel[i].tof());
    }
  }

  void testSortPulseTime()
  {
    el.sortPulseTime();
    vector<TofEvent> rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 3);
    TS_ASSERT_EQUALS(rel[0].pulseTime(), 60);
    TS_ASSERT_EQUALS(rel[1].pulseTime(), 200);
    TS_ASSERT_EQUALS(rel[2].pulseTime(), 400);

    this->fake_data();
    el.sort(PULSETIME_SORT);
    rel = el.getEvents();
    for (int i=1; i<100; i++)
    {
      TS_ASSERT_LESS_THAN_EQUALS(rel[i-1].pulseTime(), rel[i].pulseTime());
    }
  }

  void testSortPulseTime_weights()
  {
    this->fake_data();
    el.switchToWeightedEvents();
    el.sort(PULSETIME_SORT);
    vector<WeightedEvent> rwel = el.getWeightedEvents();
    for (int i=1; i<100; i++)
    {
      TS_ASSERT_LESS_THAN_EQUALS(rwel[i-1].pulseTime(), rwel[i].pulseTime());
    }
  }


  //==================================================================================
  //--- Histogramming Tests ---
  //==================================================================================

  void test_setX()
  {
    //Generate the histrogram bins
    MantidVec shared_x;
    double tof; //in ns
    for (tof=0; tof<BIN_DELTA*(NUMBINS+1); tof += BIN_DELTA)
    {
      //bins of 10 microsec
      shared_x.push_back(tof);
    }
    el.setX(shared_x);
    //Do we have the same data in X?
    const EventList el2(el);
    TS_ASSERT(el2.dataX()==shared_x);
  }


  void test_setX_empty_constructor()
  {
    el = EventList();
    //Generate the histrogram bins
    MantidVec shared_x;
    double tof; //in ns
    for (tof=0; tof<16e3*1e3; tof += 1e4)
    {
      //bins of 10 microsec
      shared_x.push_back(tof);
    }
    el.setX(shared_x);
    //Do we have the same data in X?
    const EventList el2(el);
    TS_ASSERT(el2.dataX()==shared_x);
  }

  void test_empty_histogram()
  {
    //Make sure there's no data
    el.clear();
    const EventList el2(el);

    //Getting data before setting X returns empty vector
    TS_ASSERT_EQUALS(el2.dataY()->size(), 0);

    //Now do set up an X axis.
    this->test_setX();
    MantidVec X, Y;
    const EventList el3(el);
    X = el3.dataX();
    Y = *el3.dataY();
    //Histogram is 0, since I cleared all the events
    for (int i=0; i<X.size()-1; i++)
    {
      TS_ASSERT_EQUALS(Y[i], 0);
    }
  }

  void test_no_histogram_x()
  {
    //Make sure there's no data
    el.clear();
    //Now give it some fake data, with NUMEVENTS events in it.
    this->fake_data();
    const EventList el4(el);
    TS_ASSERT_EQUALS(el4.dataY()->size(), 0);
  }





  void test_histogram()
  {
    this->fake_uniform_data();
    this->test_setX(); //Set it up
    MantidVec X, Y, E;
    const EventList el3(el); //need to copy to a const method in order to access the data directly.
    X = el3.dataX();
    Y = *el3.dataY();
    E = *el3.dataE();
    TS_ASSERT_EQUALS(Y.size(), X.size()-1);
    //The data was created so that there should be exactly 2 events per bin
    // The last bin entry will be 0 since we use it as the top boundary of i-1.
    for (int i=0; i<Y.size(); i++)
    {
      TS_ASSERT_EQUALS(Y[i], 2.0);
      TS_ASSERT_DELTA(E[i], sqrt(2.0), 1e-5);
    }
  }

  void test_histogram_weights_simple()
  {
    // 5 events per bin, simple non-weighted
    this->fake_uniform_data(5.0);
    this->test_setX();

    // Multiply by a simple scalar
    el *= 3.2;

    TS_ASSERT( el.hasWeights() );

    MantidVec X, Y, E;
    const EventList el3(el); //need to copy to a const method in order to access the data directly.
    X = el3.dataX();
    Y = *el3.dataY();
    E = *el3.dataE();
    TS_ASSERT_EQUALS(Y.size(), X.size()-1);
    for (int i=0; i<Y.size(); i++)
    {
      // 5 events, each with a weight of 3.2
      TS_ASSERT_DELTA(Y[i], 5 * 3.2, 1e-6);
      // Error should be scaled the same, by a factor of 3.2 - maintaining the same signal/error ratio.
      TS_ASSERT_DELTA(E[i], sqrt((double)5.0) * 3.2, 1e-6);
    }
  }

  void test_histogram_weights()
  {
    //This one has a weight of 2.0, error is 2.5
    this->fake_uniform_data_weights();

    this->test_setX(); //Set it up
    MantidVec X, Y, E;
    const EventList el3(el); //need to copy to a const method in order to access the data directly.
    X = el3.dataX();
    Y = *el3.dataY();
    E = *el3.dataE();
    TS_ASSERT_EQUALS(Y.size(), X.size()-1);
    //The data was created so that there should be exactly 2 events per bin
    // The last bin entry will be 0 since we use it as the top boundary of i-1.
    for (int i=0; i<Y.size(); i++)
    {
      TS_ASSERT_EQUALS(Y[i], 4.0);
      //Two errors of (2.5) adds up to sqrt(2 * 2.5*2.5)
      TS_ASSERT_DELTA(E[i], sqrt(2 * 2.5*2.5), 1e-5);
    }
  }



  void test_histogram_with_first_bin_higher_than_first_event()
  {
    //Make sure the algorithm handles it if the first bin > then the first event tof
    this->fake_uniform_data();

    //Generate the histrogram bins starting at 1000
    MantidVec shared_x;
    for (double tof=BIN_DELTA*10; tof<BIN_DELTA*(NUMBINS+1); tof += BIN_DELTA)
      shared_x.push_back(tof);
    el.setX(shared_x);

    //Get them back
    MantidVec X, Y;
    const EventList el3(el); //need to copy to a const method in order to access the data directly.
    X = el3.dataX();
    Y = *el3.dataY();
    TS_ASSERT_EQUALS(Y.size(), X.size()-1);

    //The data was created so that there should be exactly 2 events per bin. The first 10 bins (20 events) are empty.
    for (int i=0; i<Y.size(); i++)
    {
      TS_ASSERT_EQUALS(Y[i], 2.0);
    }
  }


  void test_histogram_with_first_bin_higher_than_first_event_Weights()
  {
    //Make sure the algorithm handles it if the first bin > then the first event tof
    this->fake_uniform_data_weights();
    MantidVec shared_x;
    for (double tof=BIN_DELTA*10; tof<BIN_DELTA*(NUMBINS+1); tof += BIN_DELTA)
      shared_x.push_back(tof);
    el.setX(shared_x);
    MantidVec X, Y;
    const EventList el3(el); //need to copy to a const method in order to access the data directly.
    X = el3.dataX();
    Y = *el3.dataY();
    TS_ASSERT_EQUALS(Y.size(), X.size()-1);
    for (int i=0; i<Y.size(); i++)
    {
      TS_ASSERT_EQUALS(Y[i], 4.0);
    }
  }

  void test_random_histogram()
  {
    this->fake_data();
    this->test_setX();
    MantidVec X, Y;
    const EventList el3(el);
    X = el3.dataX();
    Y = *el3.dataY();
    TS_ASSERT_EQUALS(Y.size(), X.size()-1);
    for (int i=0; i<X.size()-1; i++)
    {
      //No data was generated above 10 ms.
      if (X[i] > 10e6)
        TS_ASSERT_EQUALS(Y[i], 0.0);
    }
  }


  void test_histogram_const_call()
  {
    this->fake_uniform_data();
    this->test_setX(); //Set it up WITH THE default binning
    //Ok, we have this many bins
    TS_ASSERT_EQUALS(this->el.getRefX()->size(), NUMBINS+1);

    //Make one with half the bins
    MantidVec some_other_x;
    double tof; //in ns
    for (tof=0; tof<BIN_DELTA*(NUMBINS+1); tof += BIN_DELTA*2)
      some_other_x.push_back(tof);

    const EventList el3(el); //need to copy to a const method in order to access the data directly.
    MantidVec Y, E;
    el3.generateCountsHistogram(some_other_x, Y);
    el3.generateErrorsHistogram(Y, E);
    TS_ASSERT_EQUALS(Y.size(), some_other_x.size()-1);
    TS_ASSERT_EQUALS(E.size(), some_other_x.size()-1);
    //Now there are 4 events per bin
    for (int i=0; i<Y.size(); i++)
      TS_ASSERT_EQUALS(Y[i], 4.0);

    //With all this jazz, the original element is unchanged
    TS_ASSERT_EQUALS(this->el.getRefX()->size(), NUMBINS+1);
  }




  //-----------------------------------------------------------------------------------------------
  void test_convertTof()
  {
    this->fake_uniform_data();
    size_t old_num = this->el.getEvents().size();

    //Do convert
    this->el.convertTof( 2.5 );
    //Unchanged size
    TS_ASSERT_EQUALS(old_num, this->el.getEvents().size());

    //Original tofs were 100, 5100, 10100, etc.)
    TS_ASSERT_EQUALS(this->el.getEvents()[0].tof(), 250.0);
    TS_ASSERT_EQUALS(this->el.getEvents()[1].tof(), 12750.0);

  }

  void test_convertTof_weights()
  {
    this->fake_uniform_data();
    el.switchToWeightedEvents();
    size_t old_num = this->el.getWeightedEvents().size();
    this->el.convertTof( 2.5 );
    TS_ASSERT_EQUALS(old_num, this->el.getWeightedEvents().size());
    TS_ASSERT_EQUALS(this->el.getWeightedEvents()[0].tof(), 250.0);
    TS_ASSERT_EQUALS(this->el.getWeightedEvents()[1].tof(), 12750.0);
  }


  void test_integrate()
  {
    this->fake_uniform_data();
    TS_ASSERT_EQUALS( el.integrate(0, MAX_TOF, false),  el.getNumberEvents() );
    TS_ASSERT_EQUALS( el.integrate(10, 1, true),  el.getNumberEvents() );
    //Two events per bin
    TS_ASSERT_EQUALS( el.integrate(0, BIN_DELTA, false),  2 );
    TS_ASSERT_EQUALS( el.integrate(BIN_DELTA*10, BIN_DELTA*20, false),  20 );
    //Exactly on the first event's TOF?
    TS_ASSERT_EQUALS( el.integrate(100, 100, false),  1 );
    //Go past the ends?
    TS_ASSERT_EQUALS( el.integrate(-MAX_TOF, MAX_TOF*2, false), el.getNumberEvents()  );
    //Give max < min ?
    TS_ASSERT_EQUALS( el.integrate(1000, 100, false),  0 );
  }

  void test_integrate_weighted()
  {
    this->fake_uniform_data_weights();
    TS_ASSERT_EQUALS( el.integrate(0, MAX_TOF, false),  el.getNumberEvents()*2.0 );
    TS_ASSERT_EQUALS( el.integrate(10, 1, true),  el.getNumberEvents()*2.0 );
    //Two events per bin
    TS_ASSERT_EQUALS( el.integrate(0, BIN_DELTA, false),  2*2.0 );
    TS_ASSERT_EQUALS( el.integrate(BIN_DELTA*10, BIN_DELTA*20, false),  20*2.0 );
    //Exactly on the first event's TOF?
    TS_ASSERT_EQUALS( el.integrate(100, 100, false),  1*2.0 );
    //Go past the ends?
    TS_ASSERT_EQUALS( el.integrate(-MAX_TOF, MAX_TOF*2, false), el.getNumberEvents()*2.0  );
    //Give max < min ?
    TS_ASSERT_EQUALS( el.integrate(1000, 100, false),  0 );
  }



  //-----------------------------------------------------------------------------------------------
  void testMaskTOF()
  {
    //tof steps of 5 microseconds, starting at 100 ns, up to 20 msec
    this->fake_uniform_data();
    //How many events did we make?
    TS_ASSERT_EQUALS( el.getNumberEvents(), 2*MAX_TOF/BIN_DELTA);
    //Mask out 5-10 milliseconds
    double min = MAX_TOF * 0.25;
    double max = MAX_TOF * 0.5;
    el.maskTof( min, max);
    vector<TofEvent> rel = el.getEvents();
    for (int i=0; i<rel.size(); i++)
    {
      //No tofs in that range
      TS_ASSERT((rel[i].tof() < min) || (rel[i].tof() > max));
    }
    TS_ASSERT_EQUALS( el.getNumberEvents(), 0.75 * 2*MAX_TOF/BIN_DELTA);
  }

  void testMaskTOF_weights()
  {
    // ---- Same, with weights ------
    this->fake_uniform_data();
    el.switchToWeightedEvents();
    TS_ASSERT_EQUALS( el.getNumberEvents(), 2*MAX_TOF/BIN_DELTA);
    double min = MAX_TOF * 0.25;
    double max = MAX_TOF * 0.5;
    el.maskTof( min, max);
    vector<WeightedEvent> rwel = el.getWeightedEvents();
    for (int i=0; i<rwel.size(); i++)
    {
      TS_ASSERT((rwel[i].tof() < min) || (rwel[i].tof() > max));
    }
    TS_ASSERT_EQUALS( el.getNumberEvents(),  0.75 * 2*MAX_TOF/BIN_DELTA);
  }


  //-----------------------------------------------------------------------------------------------
  void test_getTofs_and_setTofs()
  {
    this->fake_data();

    // Grab original data as it will become "new" data
    MantidVec T;
    T = *el.getTofs();

    // Convert to make values something different
    this->el.convertTof(4.0, 2.0);
    double old_value = this->el.getEvents()[0].tof();
    size_t old_size = this->el.getEvents().size();

    // Set "new" data
    this->el.setTofs(T);
    double new_value = this->el.getEvents()[0].tof();
    size_t new_size = this->el.getEvents().size();

    TS_ASSERT_EQUALS(old_size, new_size);
    TS_ASSERT_DIFFERS(old_value, new_value);
  }

  void test_getTofs_and_setTofs_weights()
  {
    this->fake_data();
    el.switchToWeightedEvents();

    // Grab original data as it will become "new" data
    MantidVec T;
    T = *el.getTofs();

    // Convert to make values something different
    this->el.convertTof(4.0, 2.0);
    double old_value = this->el.getWeightedEvents()[0].tof();
    size_t old_size = this->el.getWeightedEvents().size();

    // Set "new" data
    this->el.setTofs(T);
    double new_value = this->el.getWeightedEvents()[0].tof();
    size_t new_size = this->el.getWeightedEvents().size();

    TS_ASSERT_EQUALS(old_size, new_size);
    TS_ASSERT_DIFFERS(old_value, new_value);
  }


  //-----------------------------------------------------------------------------------------------
  void testFilterByPulseTime()
  {
    this->fake_data();

    //Filter into this
    EventList out = EventList();
    el.filterByPulseTime(100, 200, out);

    std::vector<TofEvent> eventsIn = el.getEvents();
    int numGood = 0;
    for (int i=0; i < eventsIn.size(); i++)
      if ((eventsIn[i].pulseTime() >= 100) && (eventsIn[i].pulseTime() < 200))
        numGood++;

    //Good # of events.
    TS_ASSERT_EQUALS( numGood, out.getNumberEvents());

    std::vector<TofEvent> events = out.getEvents();
    for (int i=0; i < events.size(); i++)
    {
      //Check that the times are within the given limits.
      TS_ASSERT_LESS_THAN_EQUALS( DateAndTime(100), events[i].pulseTime());
      TS_ASSERT_LESS_THAN( events[i].pulseTime(), DateAndTime(200));
    }
  }

  void testFilterByPulseTime_weights()
  {
    this->fake_data();
    el.switchToWeightedEvents();

    //Filter into this
    EventList out = EventList();
    el.filterByPulseTime(100, 200, out);

    std::vector<WeightedEvent> eventsIn = el.getWeightedEvents();
    int numGood = 0;
    for (int i=0; i < eventsIn.size(); i++)
      if ((eventsIn[i].pulseTime() >= DateAndTime(100)) && (eventsIn[i].pulseTime() < DateAndTime(200)))
        numGood++;

    //Good # of events.
    TS_ASSERT_EQUALS( numGood, out.getNumberEvents());

    std::vector<WeightedEvent> events = out.getWeightedEvents();
    for (int i=0; i < events.size(); i++)
    {
      //Check that the times are within the given limits.
      TS_ASSERT_LESS_THAN_EQUALS( DateAndTime(100), events[i].pulseTime());
      TS_ASSERT_LESS_THAN( events[i].pulseTime(), DateAndTime(200));
    }
  }


  //-----------------------------------------------------------------------------------------------
  void testSplit()
  {
    this->fake_uniform_time_data();

    std::vector< EventList * > outputs;
    for (int i=0; i<10; i++)
      outputs.push_back( new EventList() );

    TimeSplitterType split;
    //Start only at 100
    for (int i=1; i<10; i++)
    {
      //Reject the odd hundreds pulse times (100-199, 300-399, etc).
      if ((i%2) == 0)
        split.push_back( SplittingInterval(i*100, (i+1)*100, i) );
      else
        split.push_back( SplittingInterval(i*100, (i+1)*100, -1) );
    }

    //Do the splitting
    el.splitByTime(split, outputs);

    //No events in the first ouput 0-99
    TS_ASSERT_EQUALS( outputs[0]->getNumberEvents(), 0);

    for (int i=1; i<10; i++)
    {
      EventList * myOut = outputs[i];
      //std::cout << i << " " << myOut->getNumberEvents() << "\n";
      if ((i%2) == 0)
      {
        //Even
        TS_ASSERT_EQUALS( myOut->getNumberEvents(), 100);
      }
      else
      {
        //Odd
        TS_ASSERT_EQUALS( myOut->getNumberEvents(), 0);
      }
    }
  }



  //-----------------------------------------------------------------------------------------------
  void testSplit2()
  {
    this->fake_data_only_two_times(150, 850);

    std::vector< EventList * > outputs;
    for (int i=0; i<10; i++)
      outputs.push_back( new EventList() );

    TimeSplitterType split;
    //Slices of 100
    for (int i=0; i<10; i++)
      split.push_back( SplittingInterval(i*100, (i+1)*100, i) );

    //Do the splitting
    el.splitByTime(split, outputs);

    TS_ASSERT_EQUALS( outputs[0]->getNumberEvents(), 0);
    TS_ASSERT_EQUALS( outputs[1]->getNumberEvents(), 1);
    TS_ASSERT_EQUALS( outputs[2]->getNumberEvents(), 0);
    TS_ASSERT_EQUALS( outputs[3]->getNumberEvents(), 0);
    TS_ASSERT_EQUALS( outputs[4]->getNumberEvents(), 0);
    TS_ASSERT_EQUALS( outputs[5]->getNumberEvents(), 0);
    TS_ASSERT_EQUALS( outputs[6]->getNumberEvents(), 0);
    TS_ASSERT_EQUALS( outputs[7]->getNumberEvents(), 0);
    TS_ASSERT_EQUALS( outputs[8]->getNumberEvents(), 1);
    TS_ASSERT_EQUALS( outputs[9]->getNumberEvents(), 0);
  }

  void testSplit2_weights()
  {
    this->fake_data_only_two_times(150, 850);
    el.switchToWeightedEvents();

    std::vector< EventList * > outputs;
    for (int i=0; i<10; i++)
      outputs.push_back( new EventList() );

    TimeSplitterType split;
    //Slices of 100
    for (int i=0; i<10; i++)
      split.push_back( SplittingInterval(i*100, (i+1)*100, i) );

    //Do the splitting
    el.splitByTime(split, outputs);

    TS_ASSERT_EQUALS( outputs[0]->getNumberEvents(), 0);
    TS_ASSERT_EQUALS( outputs[1]->getNumberEvents(), 1);
    TS_ASSERT_EQUALS( outputs[2]->getNumberEvents(), 0);
    TS_ASSERT_EQUALS( outputs[3]->getNumberEvents(), 0);
    TS_ASSERT_EQUALS( outputs[4]->getNumberEvents(), 0);
    TS_ASSERT_EQUALS( outputs[5]->getNumberEvents(), 0);
    TS_ASSERT_EQUALS( outputs[6]->getNumberEvents(), 0);
    TS_ASSERT_EQUALS( outputs[7]->getNumberEvents(), 0);
    TS_ASSERT_EQUALS( outputs[8]->getNumberEvents(), 1);
    TS_ASSERT_EQUALS( outputs[9]->getNumberEvents(), 0);
  }


  //-----------------------------------------------------------------------------------------------
  void testSplit_FilterWithOverlap()
  {
    this->fake_uniform_time_data();

    std::vector< EventList * > outputs;
    for (int i=0; i<1; i++)
      outputs.push_back( new EventList() );

    TimeSplitterType split;
    split.push_back( SplittingInterval(100, 200, 0) );
    split.push_back( SplittingInterval(150, 250, 0) );

    //Do the splitting
    el.splitByTime(split, outputs);

    //No events in the first ouput 0-99
    TS_ASSERT_EQUALS( outputs[0]->getNumberEvents(), 150);

  }







  //==================================================================================
  // Mocking functions
  //==================================================================================

  void fake_data()
  {
    //Clear the list
    el = EventList();
    //Create some mostly-reasonable fake data.
    srand(1234); //Fixed random seed
    for (int i=0; i < NUMEVENTS; i++)
    {
      //Random tof up to 10 ms
      //Random pulse time up to 1000
      el += TofEvent( 1e7*(rand()*1.0/RAND_MAX), rand()%1000);
    }
  }

  /** Create a uniform event list with no weights*/
  void fake_uniform_data( double events_per_bin = 2)
  {
    //Clear the list
    el = EventList();
    //Create some mostly-reasonable fake data.
    srand(1234); //Fixed random seed
    for (double tof=100; tof < MAX_TOF; tof += BIN_DELTA/events_per_bin)
    {
      //tof steps of 5 microseconds, starting at 100 ns, up to 20 msec
      el += TofEvent( tof, rand()%1000);
    }
  }

  /** Create a uniform event list with each event weight of 2.0, error 2.5 */
  void fake_uniform_data_weights()
  {
    //Clear the list
    el = EventList();
    el.switchToWeightedEvents();
    //Create some mostly-reasonable fake data.
    srand(1234); //Fixed random seed
    for (double tof=100; tof < MAX_TOF; tof += BIN_DELTA/2)
    {
      //tof steps of 5 microseconds, starting at 100 ns, up to 20 msec
      el += WeightedEvent( tof, rand()%1000, 2.0, 2.5*2.5);
    }
  }

  void fake_uniform_time_data()
  {
    //Clear the list
    el = EventList();
    //Create some mostly-reasonable fake data.
    srand(1234); //Fixed random seed
    for (double time=0; time < 1000; time++)
    {
      //All pulse times from 0 to 999
      el += TofEvent( rand()%1000, time);
    }
  }

  void fake_data_only_two_times(DateAndTime time1, DateAndTime time2)
  {
    //Clear the list
    el = EventList();
    el += TofEvent(  rand()%1000, time1);
    el += TofEvent(  rand()%1000, time2);
  }


};




#endif /// EVENTLISTTEST_H_

