/*
 * TimeSplitterTest.h
 *
 *  Created on: Sep 24, 2010
 *      Author: janik
 */

#ifndef TIMESPLITTERTEST_H_
#define TIMESPLITTERTEST_H_


#include <cxxtest/TestSuite.h>
#include <ctime>
#include "MantidKernel/TimeSplitter.h"
#include "MantidKernel/DateAndTime.h"

using namespace Mantid::Kernel;

class TimeSplitterTest : public CxxTest::TestSuite
{
public:

  //----------------------------------------------------------------------------
  /** Tests the AND operator checking for overlap between two
   * SplittingIntervals.
   */
  void test_SplittingInterval_AND()
  {
    DateAndTime start_a, stop_a, start_b, stop_b;

    start_a = DateAndTime("2007-11-30T16:17:10");
    stop_a =  DateAndTime("2007-11-30T16:17:20");
    SplittingInterval a(start_a, stop_a, 0);

    SplittingInterval b, c;
    // b is all inside a
    start_b = DateAndTime("2007-11-30T16:17:12");
    stop_b =  DateAndTime("2007-11-30T16:17:18");
    b = SplittingInterval(start_b, stop_b, 0);
    c = a & b;
    TS_ASSERT( a.overlaps(b) );
    TS_ASSERT_EQUALS( c.start(), start_b );
    TS_ASSERT_EQUALS( c.stop(), stop_b );

    // a is all inside b
    start_b = DateAndTime("2007-11-30T16:17:05");
    stop_b =  DateAndTime("2007-11-30T16:17:23");
    b = SplittingInterval(start_b, stop_b, 0);
    c = a & b;
    TS_ASSERT( a.overlaps(b) );
    TS_ASSERT_EQUALS( c.start(), start_a );
    TS_ASSERT_EQUALS( c.stop(), stop_a );

    // b goes past the end of a
    start_b = DateAndTime("2007-11-30T16:17:12");
    stop_b =  DateAndTime("2007-11-30T16:17:25");
    b = SplittingInterval(start_b, stop_b, 0);
    c = a & b;
    TS_ASSERT( a.overlaps(b) );
    TS_ASSERT_EQUALS( c.start(), start_b );
    TS_ASSERT_EQUALS( c.stop(), stop_a );

    // b starts before a and ends before
    start_b = DateAndTime("2007-11-30T16:17:05");
    stop_b =  DateAndTime("2007-11-30T16:17:15");
    b = SplittingInterval(start_b, stop_b, 0);
    c = a & b;
    TS_ASSERT( a.overlaps(b) );
    TS_ASSERT_EQUALS( c.start(), start_a );
    TS_ASSERT_EQUALS( c.stop(), stop_b );

    // No overlap (b < a)
    start_b = DateAndTime("2007-11-30T16:17:01");
    stop_b =  DateAndTime("2007-11-30T16:17:02");
    b = SplittingInterval(start_b, stop_b, 0);
    c = a & b;
    TS_ASSERT( !a.overlaps(b) );
    TS_ASSERT_LESS_THAN_EQUALS( c.duration(), 0.0 );

    // No overlap (a < b)
    start_b = DateAndTime("2007-11-30T16:17:30");
    stop_b =  DateAndTime("2007-11-30T16:17:42");
    b = SplittingInterval(start_b, stop_b, 0);
    c = a & b;
    TS_ASSERT( !a.overlaps(b) );
    TS_ASSERT_LESS_THAN_EQUALS( c.duration(), 0.0 );
  }


  //----------------------------------------------------------------------------
  /** Tests the AND operator checking for overlap between two
   * SplittingIntervals.
   */
  void test_SplittingInterval_OR()
  {
    DateAndTime start_a, stop_a, start_b, stop_b;

    start_a = DateAndTime("2007-11-30T16:17:10");
    stop_a =  DateAndTime("2007-11-30T16:17:20");
    SplittingInterval a(start_a, stop_a, 0);

    SplittingInterval b, c;
    // b is all inside a
    start_b = DateAndTime("2007-11-30T16:17:12");
    stop_b =  DateAndTime("2007-11-30T16:17:18");
    b = SplittingInterval(start_b, stop_b, 0);
    c = a | b;
    TS_ASSERT( a.overlaps(b) );
    TS_ASSERT_EQUALS( c.start(), start_a );
    TS_ASSERT_EQUALS( c.stop(), stop_a );

    // a is all inside b
    start_b = DateAndTime("2007-11-30T16:17:05");
    stop_b =  DateAndTime("2007-11-30T16:17:23");
    b = SplittingInterval(start_b, stop_b, 0);
    c = a | b;
    TS_ASSERT( a.overlaps(b) );
    TS_ASSERT_EQUALS( c.start(), start_b );
    TS_ASSERT_EQUALS( c.stop(), stop_b );

    // b goes past the end of a
    start_b = DateAndTime("2007-11-30T16:17:12");
    stop_b =  DateAndTime("2007-11-30T16:17:25");
    b = SplittingInterval(start_b, stop_b, 0);
    c = a | b;
    TS_ASSERT( a.overlaps(b) );
    TS_ASSERT_EQUALS( c.start(), start_a );
    TS_ASSERT_EQUALS( c.stop(), stop_b );

    // b starts before a and ends before
    start_b = DateAndTime("2007-11-30T16:17:05");
    stop_b =  DateAndTime("2007-11-30T16:17:15");
    b = SplittingInterval(start_b, stop_b, 0);
    c = a | b;
    TS_ASSERT( a.overlaps(b) );
    TS_ASSERT_EQUALS( c.start(), start_b );
    TS_ASSERT_EQUALS( c.stop(), stop_a );

    // No overlap (b < a) - This throws an exception because you need two outputs!
    start_b = DateAndTime("2007-11-30T16:17:01");
    stop_b =  DateAndTime("2007-11-30T16:17:02");
    b = SplittingInterval(start_b, stop_b, 0);
    TS_ASSERT( !a.overlaps(b) );
    TS_ASSERT_THROWS( c = a | b;, std::invalid_argument);

    // No overlap (a < b)
    start_b = DateAndTime("2007-11-30T16:17:30");
    stop_b =  DateAndTime("2007-11-30T16:17:42");
    b = SplittingInterval(start_b, stop_b, 0);
    TS_ASSERT( !a.overlaps(b) );
    TS_ASSERT_THROWS( c = a | b;, std::invalid_argument);
  }


  //----------------------------------------------------------------------------
  void test_AND()
  {
    //Make a splitter
    DateAndTime start, stop;
    TimeSplitterType a,b;
    start = DateAndTime("2007-11-30T16:17:00");
    stop =  DateAndTime("2007-11-30T16:17:10");
    a.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:17:20");
    stop =  DateAndTime("2007-11-30T16:17:30");
    a.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:17:40");
    stop =  DateAndTime("2007-11-30T16:17:50");
    a.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:18:00");
    stop =  DateAndTime("2007-11-30T16:18:10");
    a.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:18:20");
    stop =  DateAndTime("2007-11-30T16:18:30");
    a.push_back( SplittingInterval(start, stop, 0) );

    //Smaller than the first one
    start = DateAndTime("2007-11-30T16:17:01");
    stop =  DateAndTime("2007-11-30T16:17:25");
    b.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:17:26");
    stop =  DateAndTime("2007-11-30T16:17:27");
    b.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:17:45");
    stop =  DateAndTime("2007-11-30T16:18:15");
    b.push_back( SplittingInterval(start, stop, 0) );

    //Now AND the splitters (filters) together
    TimeSplitterType c;
    c = a & b;

    TS_ASSERT_EQUALS( c.size(), 5);
    if (c.size() < 5) return; //avoid segfaults if this part of the test fails

    SplittingInterval i;
    i = c[0];
    TS_ASSERT_EQUALS( i.start(), DateAndTime("2007-11-30T16:17:01") );
    TS_ASSERT_EQUALS( i.stop(),  DateAndTime("2007-11-30T16:17:10") );
    i = c[1];
    TS_ASSERT_EQUALS( i.start(), DateAndTime("2007-11-30T16:17:20") );
    TS_ASSERT_EQUALS( i.stop(),  DateAndTime("2007-11-30T16:17:25") );
    i = c[2];
    TS_ASSERT_EQUALS( i.start(), DateAndTime("2007-11-30T16:17:26") );
    TS_ASSERT_EQUALS( i.stop(),  DateAndTime("2007-11-30T16:17:27") );
    i = c[3];
    TS_ASSERT_EQUALS( i.start(), DateAndTime("2007-11-30T16:17:45") );
    TS_ASSERT_EQUALS( i.stop(),  DateAndTime("2007-11-30T16:17:50") );
    i = c[4];
    TS_ASSERT_EQUALS( i.start(), DateAndTime("2007-11-30T16:18:00") );
    TS_ASSERT_EQUALS( i.stop(),  DateAndTime("2007-11-30T16:18:10") );
  }



  //----------------------------------------------------------------------------
  void test_OR()
  {
    //Make a splitter
    DateAndTime start, stop;
    TimeSplitterType a,b;
    start = DateAndTime("2007-11-30T16:17:00");
    stop =  DateAndTime("2007-11-30T16:17:10");
    a.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:17:20");
    stop =  DateAndTime("2007-11-30T16:17:30");
    a.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:17:40");
    stop =  DateAndTime("2007-11-30T16:17:50");
    a.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:18:00");
    stop =  DateAndTime("2007-11-30T16:18:10");
    a.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:18:20");
    stop =  DateAndTime("2007-11-30T16:18:30");
    a.push_back( SplittingInterval(start, stop, 0) );

    //Smaller than the first one
    start = DateAndTime("2007-11-30T16:17:01");
    stop =  DateAndTime("2007-11-30T16:17:25");
    b.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:17:26");
    stop =  DateAndTime("2007-11-30T16:17:27");
    b.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:17:45");
    stop =  DateAndTime("2007-11-30T16:18:15");
    b.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:18:50");
    stop =  DateAndTime("2007-11-30T16:18:55");
    b.push_back( SplittingInterval(start, stop, 0) );

    //Now AND the splitters (filters) together
    TimeSplitterType c;
    c = a | b;

    TS_ASSERT_EQUALS( c.size(), 4);
    if (c.size() < 4) return; //avoid segfaults if this part of the test fails

    SplittingInterval i;
    i = c[0];
    TS_ASSERT_EQUALS( i.start(), DateAndTime("2007-11-30T16:17:00") );
    TS_ASSERT_EQUALS( i.stop(),  DateAndTime("2007-11-30T16:17:30") );
    i = c[1];
    TS_ASSERT_EQUALS( i.start(), DateAndTime("2007-11-30T16:17:40") );
    TS_ASSERT_EQUALS( i.stop(),  DateAndTime("2007-11-30T16:18:15") );
    i = c[2];
    TS_ASSERT_EQUALS( i.start(), DateAndTime("2007-11-30T16:18:20") );
    TS_ASSERT_EQUALS( i.stop(),  DateAndTime("2007-11-30T16:18:30") );
    i = c[3];
    TS_ASSERT_EQUALS( i.start(), DateAndTime("2007-11-30T16:18:50") );
    TS_ASSERT_EQUALS( i.stop(),  DateAndTime("2007-11-30T16:18:55") );
  }




  //----------------------------------------------------------------------------
  void test_OR_with_a_bad_input()
  {
    //Make a splitter
    DateAndTime start, stop;
    TimeSplitterType a,b;

    start = DateAndTime("2007-11-30T16:17:20");
    stop =  DateAndTime("2007-11-30T16:17:30");
    a.push_back( SplittingInterval(start, stop, 0) );

    // A bad (reversed) interval
    start = DateAndTime("2007-11-30T16:17:32");
    stop =  DateAndTime("2007-11-30T16:17:31");
    a.push_back( SplittingInterval(start, stop, 0) );

    // REVERSED interval that is before the first one
    start = DateAndTime("2007-11-30T16:17:15");
    stop =  DateAndTime("2007-11-30T16:17:00");
    b.push_back( SplittingInterval(start, stop, 0) );

    // Another bad interval
    start = DateAndTime("2007-11-30T16:17:45");
    stop =  DateAndTime("2007-11-30T16:17:35");
    b.push_back( SplittingInterval(start, stop, 0) );

    //Now AND the splitters (filters) together
    TimeSplitterType c;
    c = a | b;

    TS_ASSERT_EQUALS( c.size(), 1);
    if (c.size() < 1) return; //avoid segfaults if this part of the test fails

    SplittingInterval i;
    i = c[0];
    TS_ASSERT_EQUALS( i.start(), DateAndTime("2007-11-30T16:17:20") );
    TS_ASSERT_EQUALS( i.stop(),  DateAndTime("2007-11-30T16:17:30") );
  }


  //----------------------------------------------------------------------------
  void test_NOT_Normal()
  {
    DateAndTime start, stop;
    TimeSplitterType a,b,c;
    SplittingInterval i;

    //---- Normal Case ------
    start = DateAndTime("2007-11-30T16:17:00");
    stop =  DateAndTime("2007-11-30T16:17:10");
    a.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:17:20");
    stop =  DateAndTime("2007-11-30T16:17:30");
    a.push_back( SplittingInterval(start, stop, 0) );

    //Perform the NOT operation
    c = ~a;

    TS_ASSERT_EQUALS( c.size(), 3);
    if (c.size() < 3) return; //avoid segfaults if this part of the test fails

    i = c[0];
    TS_ASSERT_EQUALS( i.start(), DateAndTime::minimum() );
    TS_ASSERT_EQUALS( i.stop(),  DateAndTime("2007-11-30T16:17:00") );
    i = c[1];
    TS_ASSERT_EQUALS( i.start(), DateAndTime("2007-11-30T16:17:10") );
    TS_ASSERT_EQUALS( i.stop(),  DateAndTime("2007-11-30T16:17:20") );
    i = c[2];
    TS_ASSERT_EQUALS( i.start(), DateAndTime("2007-11-30T16:17:30") );
    TS_ASSERT_EQUALS( i.stop(), DateAndTime::maximum() );
  }

  //----------------------------------------------------------------------------
  void test_NOT_empty()
  {
    DateAndTime start, stop;
    TimeSplitterType a,b,c;
    SplittingInterval i;

    //---- Empty case ----------
    c = ~b;
    TS_ASSERT_EQUALS( c.size(), 1);
    if (c.size() < 1) return; //avoid segfaults if this part of the test fails
    i = c[0];
    TS_ASSERT_EQUALS( i.start(), DateAndTime::minimum());
    TS_ASSERT_EQUALS( i.stop(), DateAndTime::maximum() );
  }


  //----------------------------------------------------------------------------
  void test_NOT_overlap()
  {
    DateAndTime start, stop;
    TimeSplitterType a,b,c;
    SplittingInterval i;
    //Overlapping case ------
    start = DateAndTime("2007-11-30T16:17:00");
    stop =  DateAndTime("2007-11-30T16:17:15");
    a.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:17:10");
    stop =  DateAndTime("2007-11-30T16:17:30");
    a.push_back( SplittingInterval(start, stop, 0) );

    c = ~a;
    TS_ASSERT_EQUALS( c.size(), 2);
    if (c.size() < 3) return; //avoid segfaults if this part of the test fails

    i = c[0];
    TS_ASSERT_EQUALS( i.start(), DateAndTime::minimum() );
    TS_ASSERT_EQUALS( i.stop(),  DateAndTime("2007-11-30T16:17:00") );
    i = c[1];
    TS_ASSERT_EQUALS( i.start(), DateAndTime("2007-11-30T16:17:30") );
    TS_ASSERT_EQUALS( i.stop(), DateAndTime::maximum());

  }



  //----------------------------------------------------------------------------
  void test_PLUS()
  {
    DateAndTime start, stop;
    TimeSplitterType a,b,c;
    SplittingInterval i;

    //  the splitter ------
    start = DateAndTime("2007-11-30T16:15:00");
    stop =  DateAndTime("2007-11-30T16:16:00");
    b.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:17:00");
    stop =  DateAndTime("2007-11-30T16:18:00");
    b.push_back( SplittingInterval(start, stop, 1) );

    start = DateAndTime("2007-11-30T16:18:00");
    stop =  DateAndTime("2007-11-30T16:19:00");
    b.push_back( SplittingInterval(start, stop, 2) );

    start = DateAndTime("2007-11-30T16:19:00");
    stop =  DateAndTime("2007-11-30T16:20:00");
    b.push_back( SplittingInterval(start, stop, 3) );


    // the filter ------
    start = DateAndTime("2007-11-30T16:16:50");
    stop =  DateAndTime("2007-11-30T16:17:10");
    a.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:17:20");
    stop =  DateAndTime("2007-11-30T16:17:30");
    a.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:17:40");
    stop =  DateAndTime("2007-11-30T16:18:10");
    a.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:18:50");
    stop =  DateAndTime("2007-11-30T16:18:55");
    a.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:22:20");
    stop =  DateAndTime("2007-11-30T16:22:30");
    a.push_back( SplittingInterval(start, stop, 0) );


    //Do the PLUS operation
    c = a + b;

    TS_ASSERT_EQUALS( c.size(), 5);
    if (c.size() < 5) return; //avoid segfaults if this part of the test fails

    i = c[0];
    TS_ASSERT_EQUALS( i.start(), DateAndTime("2007-11-30T16:17:00") );
    TS_ASSERT_EQUALS( i.stop(),  DateAndTime("2007-11-30T16:17:10") );
    TS_ASSERT_EQUALS( i.index(), 1);

    i = c[1];
    TS_ASSERT_EQUALS( i.start(), DateAndTime("2007-11-30T16:17:20") );
    TS_ASSERT_EQUALS( i.stop(),  DateAndTime("2007-11-30T16:17:30") );
    TS_ASSERT_EQUALS( i.index(), 1);

    i = c[2];
    TS_ASSERT_EQUALS( i.start(), DateAndTime("2007-11-30T16:17:40") );
    TS_ASSERT_EQUALS( i.stop(),  DateAndTime("2007-11-30T16:18:00") );
    TS_ASSERT_EQUALS( i.index(), 1);

    i = c[3];
    TS_ASSERT_EQUALS( i.start(), DateAndTime("2007-11-30T16:18:00") );
    TS_ASSERT_EQUALS( i.stop(),  DateAndTime("2007-11-30T16:18:10") );
    TS_ASSERT_EQUALS( i.index(), 2);

    i = c[4];
    TS_ASSERT_EQUALS( i.start(), DateAndTime("2007-11-30T16:18:50") );
    TS_ASSERT_EQUALS( i.stop(),  DateAndTime("2007-11-30T16:18:55") );
    TS_ASSERT_EQUALS( i.index(), 2);

    //This fails since you can't add splitters together
    TS_ASSERT_THROWS(  b + b, std::invalid_argument);

  }

};



#endif /* TIMESPLITTERTEST_H_ */
