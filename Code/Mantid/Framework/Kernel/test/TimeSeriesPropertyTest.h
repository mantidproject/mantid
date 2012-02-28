#ifndef TIMESERIESPROPERTYTEST_H_
#define TIMESERIESPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>
#include <ctime>
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/DateAndTime.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include "MantidKernel/CPUTimer.h"

using namespace Mantid::Kernel;

class TimeSeriesPropertyTest : public CxxTest::TestSuite
{
public:
  void setUp()
  {
    iProp = new TimeSeriesProperty<int>("intProp");
    dProp = new TimeSeriesProperty<double>("doubleProp");
    sProp = new TimeSeriesProperty<std::string>("stringProp");
  }

  void tearDown()
  {
    delete iProp;
    delete dProp;
    delete sProp;
  }

  void test_Constructor()
  {
    // Test that all the base class member variables are correctly assigned to
    TS_ASSERT( ! iProp->name().compare("intProp") );
    TS_ASSERT( ! iProp->documentation().compare("") );
    TS_ASSERT( typeid( std::multimap<DateAndTime, int> ) == *iProp->type_info()  );
    //TS_ASSERT( iProp->isDefault() )

    TS_ASSERT( ! dProp->name().compare("doubleProp") );
    TS_ASSERT( ! dProp->documentation().compare("") );
    TS_ASSERT( typeid( std::multimap<DateAndTime, double> ) == *dProp->type_info()  );
    ///TS_ASSERT( dProp->isDefault() )

    TS_ASSERT( ! sProp->name().compare("stringProp") );
    TS_ASSERT( ! sProp->documentation().compare("") );
    TS_ASSERT( typeid( std::multimap<DateAndTime, std::string> ) == *sProp->type_info()  );
    //TS_ASSERT( sProp->isDefault() )
  }

  void test_SetValue()
  {
    TS_ASSERT_THROWS( iProp->setValue("1"), Exception::NotImplementedError );
    TS_ASSERT_THROWS( dProp->setValue("5.5"), Exception::NotImplementedError );
    TS_ASSERT_THROWS( sProp->setValue("aValue"), Exception::NotImplementedError );
  }

  void test_AddValue()
  {
    const std::string tester("2007-11-30T16:17:00");
    TS_ASSERT( iProp->addValue(tester,1) );
    TS_ASSERT( iProp->addValue("2007-11-30T16:17:10",1) );

    TS_ASSERT( dProp->addValue("2007-11-30T16:17:00",9.99) );
    TS_ASSERT( dProp->addValue("2007-11-30T16:17:10",5.55) );

    TS_ASSERT( sProp->addValue("2007-11-30T16:17:00","test") );
    TS_ASSERT( sProp->addValue("2007-11-30T16:17:10","test2") );

    //Now try the other overloads
    TimeSeriesProperty<int> otherProp("otherProp");
    TS_ASSERT( otherProp.addValue( static_cast<std::time_t>( 123 ), 1) );
    TS_ASSERT( otherProp.addValue( boost::posix_time::second_clock::local_time(), 1) );

    const std::string dString = dProp->value();
    TS_ASSERT_EQUALS( dString.substr(0,27), "2007-Nov-30 16:17:00  9.99\n" );
    const std::string iString = iProp->value();
    TS_ASSERT_EQUALS( iString.substr(0,24), "2007-Nov-30 16:17:00  1\n" );
    const std::string sString = sProp->value();
    TS_ASSERT_EQUALS( sString.substr(0,27), "2007-Nov-30 16:17:00  test\n" );
  }

  void test_timesAsVector()
  {
    TimeSeriesProperty<double> * p = new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT( p->addValue("2007-11-30T16:17:20",5.55) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:00",9.99) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:10",5.55) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:30",5.55) );
    std::vector<double> timeSec;
    timeSec = p->timesAsVectorSeconds();
    TS_ASSERT_DELTA( timeSec[0], 0.0, 1e-6);
    TS_ASSERT_DELTA( timeSec[1], 10.0, 1e-6);
    TS_ASSERT_DELTA( timeSec[2], 20.0, 1e-6);
    TS_ASSERT_DELTA( timeSec[3], 30.0, 1e-6);
    std::vector<DateAndTime> time;
    time = p->timesAsVector();
    TS_ASSERT_EQUALS( time[0], DateAndTime("2007-11-30T16:17:00"));
    TS_ASSERT_EQUALS( time[1], DateAndTime("2007-11-30T16:17:10"));
    TS_ASSERT_EQUALS( time[2], DateAndTime("2007-11-30T16:17:20"));
    TS_ASSERT_EQUALS( time[3], DateAndTime("2007-11-30T16:17:30"));

    delete p;
  }

  void test_addValues()
  {
    size_t num=1000;
    DateAndTime first("2007-11-30T16:17:10");
    std::vector<DateAndTime> times;

    std::vector<double> values;
    for (size_t i=0; i<num; i++)
    {
      times.push_back( first + double(i) );
      values.push_back( double(i) );
    }
    CPUTimer tim;
    TimeSeriesProperty<double> tsp("test");
    tsp.addValues(times, values);
    TS_ASSERT_EQUALS( tsp.size(), 1000);
    TS_ASSERT_EQUALS( tsp.nthValue(3), 3.0);
    //std::cout << tim << " to add " << num << " entries." << std::endl;

  }

  void test_Casting()
  {
    TS_ASSERT_DIFFERS( dynamic_cast<Property*>(iProp), static_cast<Property*>(0) );
    TS_ASSERT_DIFFERS( dynamic_cast<Property*>(dProp), static_cast<Property*>(0) );
    TS_ASSERT_DIFFERS( dynamic_cast<Property*>(sProp), static_cast<Property*>(0) );
  }


  //----------------------------------------------------------------------------
  void test_AdditionOperator()
  {
    TimeSeriesProperty<int> * log  = new TimeSeriesProperty<int>("MyIntLog");
    TS_ASSERT( log->addValue("2007-11-30T16:17:00",1) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:10",2) );

    TimeSeriesProperty<int> * log2  = new TimeSeriesProperty<int>("MyIntLog2");
    TS_ASSERT( log2->addValue("2007-11-30T16:18:00",3) );
    TS_ASSERT( log2->addValue("2007-11-30T16:18:10",4) );
    TS_ASSERT( log2->addValue("2007-11-30T16:18:11",5) );

    TS_ASSERT_EQUALS( log->size(), 2);

    //Concatenate the lists
    (*log) += log2;

    TS_ASSERT_EQUALS( log->size(), 5);

    delete log;
    delete log2;
  }

  //----------------------------------------------------------------------------
  /// Ticket 2097: This caused an infinite loop
  void test_AdditionOperatorOnYourself()
  {
    TimeSeriesProperty<int> * log  = new TimeSeriesProperty<int>("MyIntLog");
    TS_ASSERT( log->addValue("2007-11-30T16:17:00",1) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:10",2) );

    (*log) += log;
    // There is now a check and trying to do this does nothing.
    TS_ASSERT_EQUALS( log->size(), 2);

    delete log;
  }

  //----------------------------------------------------------------------------
  void test_filterByTime_and_getTotalValue()
  {
    TimeSeriesProperty<int> * log  = new TimeSeriesProperty<int>("MyIntLog");
    TS_ASSERT( log->addValue("2007-11-30T16:17:00",1) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:10",2) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:20",3) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:30",4) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:40",5) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:50",6) );

    TS_ASSERT_EQUALS( log->realSize(), 6);
    TS_ASSERT_EQUALS( log->getTotalValue(), 21);
    DateAndTime start = DateAndTime("2007-11-30T16:17:10");
    DateAndTime stop = DateAndTime("2007-11-30T16:17:40");

    //Since the filter is < stop, the last one is not counted, so there are  3 taken out.

    log->filterByTime(start, stop);
    TS_ASSERT_EQUALS( log->realSize(), 3);
    TS_ASSERT_EQUALS( log->getTotalValue(), 9);

    delete log;
  }


  //----------------------------------------------------------------------------
  /// Ticket #2591
  void test_filterByTime_ifOnlyOneValue_assumes_constant_instead()
  {
    TimeSeriesProperty<int> * log  = new TimeSeriesProperty<int>("MyIntLog");
    TS_ASSERT( log->addValue("2007-11-30T16:17:00",1) );
    TS_ASSERT_EQUALS( log->realSize(), 1);
    TS_ASSERT_EQUALS( log->getTotalValue(), 1);

    DateAndTime start = DateAndTime("2007-11-30T16:17:10");
    DateAndTime stop = DateAndTime("2007-11-30T16:17:40");
    log->filterByTime(start, stop);

    // Still there!
    TS_ASSERT_EQUALS( log->realSize(), 1);
    TS_ASSERT_EQUALS( log->getTotalValue(), 1);

    delete log;
  }


  //----------------------------------------------------------------------------
  /// Ticket #2591
  void test_filterByTime_ifOnlyOneValue_assumes_constant_instead_2()
  {
    TimeSeriesProperty<int> * log  = new TimeSeriesProperty<int>("MyIntLog");
    TS_ASSERT( log->addValue("1990-01-01",1) );
    TS_ASSERT_EQUALS( log->realSize(), 1);
    TS_ASSERT_EQUALS( log->getTotalValue(), 1);

    DateAndTime start = DateAndTime("2007-11-30T16:17:10");
    DateAndTime stop = DateAndTime("2007-11-30T16:17:40");
    log->filterByTime(start, stop);

    // Still there!
    TS_ASSERT_EQUALS( log->realSize(), 1);
    TS_ASSERT_EQUALS( log->getTotalValue(), 1);

    delete log;
  }



  //----------------------------------------------------------------------------
  void test_makeFilterByValue()
  {
    TimeSeriesProperty<double> * log  = new TimeSeriesProperty<double>("MyIntLog");
    TS_ASSERT( log->addValue("2007-11-30T16:17:00",1) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:10",2) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:20",3) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:30",2.0) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:40",2.01) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:50",6) );

    TS_ASSERT_EQUALS( log->realSize(), 6);

    TimeSplitterType splitter;
    log->makeFilterByValue(splitter, 1.8, 2.2, 1.0);

    TS_ASSERT_EQUALS( splitter.size(), 2);
    SplittingInterval s;
    DateAndTime t;

    s = splitter[0];
    t = DateAndTime("2007-11-30T16:17:09");
    TS_ASSERT_DELTA( s.start(), t, 1e-3);
    t = DateAndTime("2007-11-30T16:17:11");
    TS_ASSERT_DELTA( s.stop(), t, 1e-3);

    s = splitter[1];
    t = DateAndTime("2007-11-30T16:17:29");
    TS_ASSERT_DELTA( s.start(), t, 1e-3);
    t = DateAndTime("2007-11-30T16:17:41");
    TS_ASSERT_DELTA( s.stop(), t, 1e-3);

    delete log;
  }




  //----------------------------------------------------------------------------
  void test_splitByTime_and_getTotalValue()
  {
    TimeSeriesProperty<int> * log  = new TimeSeriesProperty<int>("MyIntLog");
    TS_ASSERT( log->addValue("2007-11-30T16:17:00",1) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:10",2) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:20",3) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:30",4) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:40",5) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:50",6) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:00",7) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:10",8) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:20",9) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:30",10) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:40",11) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:50",12) );
    TS_ASSERT_EQUALS( log->realSize(), 12);

    //Make the outputs
    //    std::vector< TimeSeriesProperty<int> * > outputs2;
    std::vector< Property * > outputs;
    for (std::size_t i=0; i<5; i++)
    {
      TimeSeriesProperty<int> * newlog  = new TimeSeriesProperty<int>("MyIntLog");
      outputs.push_back(newlog);
      //outputs2.push_back(newlog);
    }

    //Make a splitter
    DateAndTime start, stop;
    TimeSplitterType splitter;
    start = DateAndTime("2007-11-30T16:17:10");
    stop = DateAndTime("2007-11-30T16:17:40");
    splitter.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:17:55");
    stop = DateAndTime("2007-11-30T16:17:56");
    splitter.push_back( SplittingInterval(start, stop, 1) );

    start = DateAndTime("2007-11-30T16:17:56");
    stop = DateAndTime("2007-11-30T16:18:01");
    splitter.push_back( SplittingInterval(start, stop, 2) ); //just one entry

    start = DateAndTime("2007-11-30T16:18:09");
    stop = DateAndTime("2007-11-30T16:18:21");
    splitter.push_back( SplittingInterval(start, stop, 3) );

    start = DateAndTime("2007-11-30T16:18:45");
    stop = DateAndTime("2007-11-30T16:22:50");
    splitter.push_back( SplittingInterval(start, stop, 4) );

    log->splitByTime(splitter, outputs);

    TS_ASSERT_EQUALS( dynamic_cast< TimeSeriesProperty<int> * >(outputs[0])->realSize(), 3);
    TS_ASSERT_EQUALS( dynamic_cast< TimeSeriesProperty<int> * >(outputs[1])->realSize(), 0);
    TS_ASSERT_EQUALS( dynamic_cast< TimeSeriesProperty<int> * >(outputs[2])->realSize(), 1);
    TS_ASSERT_EQUALS( dynamic_cast< TimeSeriesProperty<int> * >(outputs[3])->realSize(), 2);
    TS_ASSERT_EQUALS( dynamic_cast< TimeSeriesProperty<int> * >(outputs[4])->realSize(), 1);

    delete log;
    delete outputs[0];
    delete outputs[1];
    delete outputs[2];
    delete outputs[3];
    delete outputs[4];
  }


  //----------------------------------------------------------------------------
  void test_splitByTime_withOverlap()
  {
    TimeSeriesProperty<int> * log  = new TimeSeriesProperty<int>("MyIntLog");
    TS_ASSERT( log->addValue("2007-11-30T16:17:00",1) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:10",2) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:20",3) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:30",4) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:40",5) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:50",6) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:00",7) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:10",8) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:20",9) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:30",10) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:40",11) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:50",12) );
    TS_ASSERT_EQUALS( log->realSize(), 12);

    //Make the outputs
    //    std::vector< TimeSeriesProperty<int> * > outputs2;
    std::vector< Property * > outputs;
    for (std::size_t i=0; i<1; i++)
    {
      TimeSeriesProperty<int> * newlog  = new TimeSeriesProperty<int>("MyIntLog");
      outputs.push_back(newlog);
    }

    //Make a splitter
    DateAndTime start, stop;
    TimeSplitterType splitter;
    start = DateAndTime("2007-11-30T16:17:10");
    stop = DateAndTime("2007-11-30T16:17:40");
    splitter.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:17:35");
    stop = DateAndTime("2007-11-30T16:17:59");
    splitter.push_back( SplittingInterval(start, stop, 0) );

    log->splitByTime(splitter, outputs);

    TS_ASSERT_EQUALS( dynamic_cast< TimeSeriesProperty<int> * >(outputs[0])->realSize(), 5);

    delete log;
    delete outputs[0];
  }

  //----------------------------------------------------------------------------
  void test_statistics()
  {
    TimeSeriesProperty<double> * log  = new TimeSeriesProperty<double>("MydoubleLog");
    TS_ASSERT( log->addValue("2007-11-30T16:17:00",1) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:10",2) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:20",3) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:30",4) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:40",5) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:50",6) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:00",7) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:10",8) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:20",9) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:30",10) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:40",11) );
    TS_ASSERT_EQUALS( log->realSize(), 11);

    TimeSeriesPropertyStatistics stats = log->getStatistics();

    TS_ASSERT_DELTA( stats.minimum, 1.0, 1e-3);
    TS_ASSERT_DELTA( stats.maximum, 11.0, 1e-3);
    TS_ASSERT_DELTA( stats.median, 6.0, 1e-3);
    TS_ASSERT_DELTA( stats.mean, 6.0, 1e-3);
    TS_ASSERT_DELTA( stats.duration, 100.0, 1e-3);
    TS_ASSERT_DELTA( stats.standard_deviation, 3.1622, 1e-3);

    delete log;
  }

  void test_empty_statistics()
  {
    TimeSeriesProperty<double> * log  = new TimeSeriesProperty<double>("MydoubleLog");
    TimeSeriesPropertyStatistics stats = log->getStatistics();
    TS_ASSERT( boost::math::isnan(stats.minimum) );
    TS_ASSERT( boost::math::isnan(stats.maximum) );
    TS_ASSERT( boost::math::isnan(stats.median) );
    TS_ASSERT( boost::math::isnan(stats.mean) );
    TS_ASSERT( boost::math::isnan(stats.standard_deviation) );
    TS_ASSERT( boost::math::isnan(stats.duration) );

    delete log;
  }

  void test_PlusEqualsOperator_Incompatible_Types_dontThrow()
  {
    // Adding incompatible types together should not throw, but issue a warning in the log

    TimeSeriesProperty<double> * log  = new TimeSeriesProperty<double>("MydoubleLog");
    TimeSeriesProperty<int> * logi  = new TimeSeriesProperty<int>("MyIntLog");
    PropertyWithValue<double> * val  = new PropertyWithValue<double>("MySimpleDouble", 1.23);

    log->operator+=(val);
    log->operator+=(logi);
    logi->operator+=(log);
    val->operator+=(log);
    val->operator+=(logi);

    delete log;
    delete logi;
    delete val;
  }

  void test_PlusEqualsOperator_() {
    TimeSeriesProperty<double> * lhs  = new TimeSeriesProperty<double>("doubleLog");
    TS_ASSERT( lhs->addValue("2007-11-30T16:17:00",1) );
    TS_ASSERT( lhs->addValue("2007-11-30T16:17:10",2) );
    TS_ASSERT( lhs->addValue("2007-11-30T16:17:20",3) );
    TS_ASSERT( lhs->addValue("2007-11-30T16:17:30",4) );
    TS_ASSERT( lhs->addValue("2007-11-30T16:17:40",5) );
    TimeSeriesProperty<double> * rhs  = new TimeSeriesProperty<double>("doubleLog");
    TS_ASSERT( rhs->addValue("2007-11-30T16:17:00",1) );
    TS_ASSERT( rhs->addValue("2007-11-30T16:17:10",2) );
    TS_ASSERT( rhs->addValue("2007-11-30T16:17:20",3) );
    TS_ASSERT( rhs->addValue("2007-11-30T16:17:30",4) );
    TS_ASSERT( rhs->addValue("2007-11-30T16:17:40",5) );

    lhs->operator+=(rhs);

    TS_ASSERT_EQUALS(lhs->size(), rhs->size());

    delete lhs;
    delete rhs;
  }

  /*
   * Test include (1) normal interval (2) normal on grid point (3) outside upper boundary
   * (4) outside lower bound
   */
  void test_getSingleValue()
  {
    TimeSeriesProperty<double> * p = new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT( p->addValue("2007-11-30T16:17:00",9.99) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:10",7.55) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:20",5.55) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:30",10.55) );

    DateAndTime time1("2007-11-30T16:17:23");
    double v1 = p->getSingleValue(time1);
    TS_ASSERT_DELTA( v1, 5.55, 1e-6);

    DateAndTime time2("2007-11-30T16:17:03");
    double v2 = p->getSingleValue(time2);
    TS_ASSERT_DELTA( v2, 9.99, 1e-6);

    DateAndTime time3("2007-11-30T16:17:31");
    double v3 = p->getSingleValue(time3);
    TS_ASSERT_DELTA( v3, 10.55, 1e-6);

    DateAndTime time4("2007-11-30T16:17:00");
    double v4 = p->getSingleValue(time4);
    TS_ASSERT_DELTA( v4, 9.99, 1e-6);

    DateAndTime time5("2007-11-30T16:16:59");
    double v5 = p->getSingleValue(time5);
    TS_ASSERT_DELTA( v5, 9.99, 1e-6);

    delete p;
  }

  /*
   * Test firstTime, lastTime, firstValue and lastValue
   */
  void test_firstLastTimeValue()
  {
    TimeSeriesProperty<double> * p = new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT( p->addValue("2007-11-30T16:17:00",9.99) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:10",7.55) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:20",5.55) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:30",10.55) );

    Mantid::Kernel::DateAndTime t0 = p->firstTime();
    Mantid::Kernel::DateAndTime tf = p->lastTime();

    Mantid::Kernel::DateAndTime t0c("2007-11-30T16:17:00");
    Mantid::Kernel::DateAndTime tfc("2007-11-30T16:17:30");

    double v0 = p->firstValue();
    double vf = p->lastValue();

    TS_ASSERT_EQUALS(t0, t0c);
    TS_ASSERT_EQUALS(tf, tfc);

    TS_ASSERT_DELTA(v0, 9.99, 1.0E-8);
    TS_ASSERT_DELTA(vf, 10.55, 1.0E-8);

    delete p;

    return;
  }


  /*
   * Test merge()
   */
  void test_Merge()
  {
    TimeSeriesProperty<double> *p1 = new TimeSeriesProperty<double>("doubleProp1");
    TimeSeriesProperty<double> *p2 = new TimeSeriesProperty<double>("doubleProp2");

    // 1. Construct p1 and p2
    TS_ASSERT( p1->addValue("2007-11-30T16:17:00",9.99) );
    TS_ASSERT( p1->addValue("2007-11-30T16:17:10",7.55) );
    TS_ASSERT( p1->addValue("2007-11-30T16:17:20",5.55) );
    TS_ASSERT( p1->addValue("2007-11-30T16:17:30",10.55) );

    TS_ASSERT( p2->addValue("2007-11-30T16:17:05",19.99) );
    TS_ASSERT( p2->addValue("2007-11-30T16:17:15",17.55) );
    TS_ASSERT( p2->addValue("2007-11-30T16:17:17",15.55) );
    TS_ASSERT( p2->addValue("2007-11-30T16:17:35",110.55) );

    // 2. Test
    p1->merge(p2);

    // 3. Verify
    Mantid::Kernel::DateAndTime t0("2007-11-30T16:17:00");
    Mantid::Kernel::DateAndTime tf("2007-11-30T16:17:35");
    Mantid::Kernel::DateAndTime t1("2007-11-30T16:17:05");

    TS_ASSERT_EQUALS(p1->firstTime(), t0);
    TS_ASSERT_EQUALS(p1->lastTime(), tf);

    TS_ASSERT_DELTA(p1->getSingleValue(t0), 9.99, 1.0E-8);
    TS_ASSERT_DELTA(p1->getSingleValue(tf), 110.55, 1.0E-8);
    TS_ASSERT_DELTA(p1->getSingleValue(t1), 19.99, 1.0E-8);

    // -1. Clean
    delete p1;
    delete p2;

    return;
  }

  /*
   * Test setName and getName
   */
  void test_Name()
  {
    TimeSeriesProperty<double> * p = new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT( p->addValue("2007-11-30T16:17:00",9.99) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:10",7.55) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:20",5.55) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:30",10.55) );

    std::string propertyname("UnitTest");

    p->setName(propertyname);

    TS_ASSERT_EQUALS(p->getName(), propertyname);

    delete p;
  }

  /*
   * Test value()
   */
  void test_Value()
  {
    TimeSeriesProperty<double> * p = new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT( p->addValue("2007-11-30T16:17:00",9.99) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:10",7.55) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:20",5.55) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:30",10.55) );

    std::string pvalue = p->value();
    std::string svalue("2007-Nov-30 16:17:00  9.99\n2007-Nov-30 16:17:10  7.55\n2007-Nov-30 16:17:20  5.55\n2007-Nov-30 16:17:30  10.55\n");

    TS_ASSERT_EQUALS(pvalue, svalue);

    delete p;
  }


  /*
   * Test valueAsVector()
   */
  void test_ValueAsVector()
  {
    // 1. Create property
    TimeSeriesProperty<double> * p = new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT( p->addValue("2007-11-30T16:17:00",1.00) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:20",3.00) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:10",2.00) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:30",4.00) );

    // 2. Value as vector
    std::vector<double> values = p->valuesAsVector();

    TS_ASSERT_EQUALS(values.size(), 4);
    for (size_t i = 0; i < 4; i ++)
    {
      TS_ASSERT_DELTA(values[i], static_cast<double>(i)+1.0, 1.0E-9);
    }

    delete p;

    return;
  }


  /*
   * Test clone
   */
  void test_Clone()
  {
    // 1. Create property
    TimeSeriesProperty<double> * p = new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT( p->addValue("2007-11-30T16:17:00",1.00) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:20",3.00) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:10",2.00) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:30",4.00) );

    // 2. Clone
    TimeSeriesProperty<double>* newp = dynamic_cast<TimeSeriesProperty<double>* >(p->clone());

    // 3. Check
    std::vector<Mantid::Kernel::DateAndTime> times1 = p->timesAsVector();
    std::vector<double> values1 = p->valuesAsVector();

    std::vector<Mantid::Kernel::DateAndTime> times2 = newp->timesAsVector();
    std::vector<double> values2 = newp->valuesAsVector();

    TS_ASSERT_EQUALS(times1, times2);

    if (times1.size() == times2.size())
    {
      for (size_t i = 0; i < times1.size(); i ++)
      {
        TS_ASSERT_EQUALS(times1[i], times2[i]);
        TS_ASSERT_DELTA(values1[i], values2[i], 1.0E-10);
      }
    }

    // 4. Clean
    delete p;
    delete newp;

    return;
  }

  /*
   * Test countSize()
   */
  void test_CountSize()
  {
    // 1. Create property
    TimeSeriesProperty<double> * p = new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT( p->addValue("2007-11-30T16:17:00",1.00) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:20",3.00) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:10",2.00) );
    TS_ASSERT( p->addValue("2007-11-30T16:17:30",4.00) );

    // 2. Check no double entry
    p->countSize();
    TS_ASSERT_EQUALS(p->size() , 4);

    // 3. Add double entry
    TS_ASSERT( p->addValue("2007-11-30T16:17:10",2.00) );
    p->countSize();
    TS_ASSERT_EQUALS(p->size() , 3);

    // 4. Add another entry
    TS_ASSERT( p->addValue("2007-11-30T16:17:30",200.00) );
    p->countSize();
    TS_ASSERT_EQUALS(p->size() , 2);

    // -1. Clean
    delete p;

    return;
  }

  /*
   * Test isTimeString()
   */
  void test_IsTimeString()
  {
    TimeSeriesProperty<double> *p = new TimeSeriesProperty<double>("Test");

    std::string timestring1("2007-11-30T16:17:00");
    TS_ASSERT(p->isTimeString(timestring1));

    std::string timestring2("2007-11-30 T16:17:00");
    TS_ASSERT(!TimeSeriesProperty<double>::isTimeString(timestring2));

    std::string timestring3("2007U11X30T16a17a00");
    TS_ASSERT(TimeSeriesProperty<double>::isTimeString(timestring3));

    std::string timestring4("2007-11-30T16:I7:00");
    TS_ASSERT(!TimeSeriesProperty<double>::isTimeString(timestring4));

    delete p;

    return;
  }


private:
  TimeSeriesProperty<int> *iProp;
  TimeSeriesProperty<double> *dProp;
  TimeSeriesProperty<std::string> *sProp;
};

#endif /*TIMESERIESPROPERTYTEST_H_*/
