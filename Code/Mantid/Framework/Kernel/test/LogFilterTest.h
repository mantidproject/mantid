#ifndef LOGFILTERTEST_H_
#define LOGFILTERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/LogFilter.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <ctime>

using namespace Mantid::Kernel;

class LogFilterTest : public CxxTest::TestSuite
{
    TimeSeriesProperty<double>* p;
public:
    static LogFilterTest *createSuite() { return new LogFilterTest(); }
    static void destroySuite(LogFilterTest *suite) { delete suite; }

    LogFilterTest()
        :p(new TimeSeriesProperty<double>("test"))
    {
        p->addValue("2007-11-30T16:17:00",1);
        p->addValue("2007-11-30T16:17:10",2);
        p->addValue("2007-11-30T16:17:20",3);
        p->addValue("2007-11-30T16:17:30",4);
        p->addValue("2007-11-30T16:17:40",5);
    }

    ~LogFilterTest()
    {
      delete p;
    }

    void testnthValue()
    {

      TS_ASSERT_EQUALS( p->size(), 5 );
      TS_ASSERT_EQUALS( p->nthValue(0), 1 );
      TS_ASSERT_EQUALS( p->nthValue(1), 2 );
      TS_ASSERT_EQUALS( p->nthValue(2), 3 );
      TS_ASSERT_EQUALS( p->nthValue(3), 4 );
      TS_ASSERT_EQUALS( p->nthValue(4), 5 );
      TS_ASSERT_EQUALS( p->nthValue(5), 5 );

      TS_ASSERT_EQUALS( p->nthInterval(0).begin_str(), "2007-Nov-30 16:17:00" );
      TS_ASSERT_EQUALS( p->nthInterval(0).end_str(), "2007-Nov-30 16:17:10" );

      TS_ASSERT_EQUALS( p->nthInterval(1).begin_str(), "2007-Nov-30 16:17:10" );
      TS_ASSERT_EQUALS( p->nthInterval(1).end_str(), "2007-Nov-30 16:17:20" );

      TS_ASSERT_EQUALS( p->nthInterval(2).begin_str(), "2007-Nov-30 16:17:20" );
      TS_ASSERT_EQUALS( p->nthInterval(2).end_str(), "2007-Nov-30 16:17:30" );

      TS_ASSERT_EQUALS( p->nthInterval(3).begin_str(), "2007-Nov-30 16:17:30" );
      TS_ASSERT_EQUALS( p->nthInterval(3).end_str(), "2007-Nov-30 16:17:40" );

      TS_ASSERT_EQUALS( p->nthInterval(4).begin_str(), "2007-Nov-30 16:17:40" );
      TS_ASSERT_EQUALS( p->nthInterval(4).end_str(), "2007-Nov-30 16:17:50" ); //nth interval changed to use previous interval now.

    }

    void testF1()
    {
      TimeSeriesProperty<bool> f("1");
      f.addValue("2007-11-30T16:16:50",true);
      f.addValue("2007-11-30T16:17:25",false);
      f.addValue("2007-11-30T16:17:39",true);

      LogFilter flt(p);
      flt.addFilter(&f);

      TS_ASSERT_EQUALS( flt.data()->size(), 5 );

      TS_ASSERT_EQUALS( flt.data()->nthInterval(0).begin_str(), "2007-Nov-30 16:17:00" );
      TS_ASSERT_EQUALS( flt.data()->nthInterval(0).end_str(), "2007-Nov-30 16:17:10" );
      TS_ASSERT_EQUALS( flt.data()->nthValue(0), 1 );

      TS_ASSERT_EQUALS( flt.data()->nthInterval(1).begin_str(), "2007-Nov-30 16:17:10" );
      TS_ASSERT_EQUALS( flt.data()->nthInterval(1).end_str(), "2007-Nov-30 16:17:20" );
      TS_ASSERT_EQUALS( flt.data()->nthValue(1), 2 );

      TS_ASSERT_EQUALS( flt.data()->nthInterval(2).begin_str(), "2007-Nov-30 16:17:20" );
      TS_ASSERT_EQUALS( flt.data()->nthInterval(2).end_str(), "2007-Nov-30 16:17:25" );
      TS_ASSERT_EQUALS( flt.data()->nthValue(2), 3 );

      TS_ASSERT_EQUALS( flt.data()->nthInterval(3).begin_str(), "2007-Nov-30 16:17:39" );
      TS_ASSERT_EQUALS( flt.data()->nthInterval(3).end_str(), "2007-Nov-30 16:17:40" );
      TS_ASSERT_EQUALS( flt.data()->nthValue(3), 4 );

      TS_ASSERT_EQUALS( flt.data()->nthInterval(4).begin_str(), "2007-Nov-30 16:17:40" );
      TS_ASSERT_EQUALS( flt.data()->nthInterval(4).end_str(), "2007-Nov-30 16:17:41" );
      TS_ASSERT_EQUALS( flt.data()->nthValue(4), 5 );

    }

    void testF1a()
    {
        TimeSeriesProperty<bool> f("1");
        f.addValue("2007-11-30T16:16:50",false);
        f.addValue("2007-11-30T16:17:25",true);
        f.addValue("2007-11-30T16:17:39",false);

        LogFilter flt(p);
        flt.addFilter(&f);

        TS_ASSERT_EQUALS( flt.data()->size(), 2 );

        TS_ASSERT_EQUALS( flt.data()->nthInterval(0).begin_str(), "2007-Nov-30 16:17:25" );
        TS_ASSERT_EQUALS( flt.data()->nthInterval(0).end_str(), "2007-Nov-30 16:17:30" );
        TS_ASSERT_EQUALS( flt.data()->nthValue(0), 3 );

        TS_ASSERT_EQUALS( flt.data()->nthInterval(1).begin_str(), "2007-Nov-30 16:17:30" );
        TS_ASSERT_EQUALS( flt.data()->nthInterval(1).end_str(), "2007-Nov-30 16:17:39" );
        TS_ASSERT_EQUALS( flt.data()->nthValue(1), 4 );

    }


    /*
     * this is a test for two filters doing "AND" operation according to previous unit test.
     */
    void testF12()
    {
      TimeSeriesProperty<bool> f("1");
      f.addValue("2007-11-30T16:16:50",true);
      f.addValue("2007-11-30T16:17:25",false);
      f.addValue("2007-11-30T16:17:39",true);

      TimeSeriesProperty<bool> f2("2");
      f2.addValue("2007-11-30T16:17:00", true);
      f2.addValue("2007-11-30T16:17:05",false);
      f2.addValue("2007-11-30T16:17:12",true);

      LogFilter flt(p);
      flt.addFilter(&f);
      flt.addFilter(&f2);


      TS_ASSERT_EQUALS( flt.data()->size(), 5 );

      TS_ASSERT_EQUALS( flt.data()->nthInterval(0).begin_str(), "2007-Nov-30 16:17:00" );
      TS_ASSERT_EQUALS( flt.data()->nthInterval(0).end_str(), "2007-Nov-30 16:17:05" );
      TS_ASSERT_EQUALS( flt.data()->nthValue(0), 1 );

      TS_ASSERT_EQUALS( flt.data()->nthInterval(1).begin_str(), "2007-Nov-30 16:17:12" );
      TS_ASSERT_EQUALS( flt.data()->nthInterval(1).end_str(), "2007-Nov-30 16:17:20" );
      TS_ASSERT_EQUALS( flt.data()->nthValue(1), 2 );

      TS_ASSERT_EQUALS( flt.data()->nthInterval(2).begin_str(), "2007-Nov-30 16:17:20" );
      TS_ASSERT_EQUALS( flt.data()->nthInterval(2).end_str(), "2007-Nov-30 16:17:25" );
      TS_ASSERT_EQUALS( flt.data()->nthValue(2), 3 );

      TS_ASSERT_EQUALS( flt.data()->nthInterval(3).begin_str(), "2007-Nov-30 16:17:39" );
      TS_ASSERT_EQUALS( flt.data()->nthInterval(3).end_str(), "2007-Nov-30 16:17:40" );
      TS_ASSERT_EQUALS( flt.data()->nthValue(3), 4 );

      TS_ASSERT_EQUALS( flt.data()->nthInterval(4).begin_str(), "2007-Nov-30 16:17:40" );
      TS_ASSERT_EQUALS( flt.data()->nthInterval(4).end_str(), "2007-Nov-30 16:17:41" );
      TS_ASSERT_EQUALS( flt.data()->nthValue(4), 5 );

    }

    void testF3()
    {
      TimeSeriesProperty<bool> f("1");
      f.addValue("2007-11-30T16:17:00",false);
      f.addValue("2007-11-30T16:17:40",true);
      f.addValue("2007-11-30T16:17:45",false);
      f.addValue("2007-11-30T16:17:50",true);
      f.addValue("2007-11-30T16:18:00",false);

      LogFilter flt(p);
      flt.addFilter(&f);

      TS_ASSERT_EQUALS( flt.data()->size(), 2);

      return;

      TS_ASSERT_EQUALS( flt.data()->nthInterval(0).begin_str(), "2007-Nov-30 16:17:40" );
      TS_ASSERT_EQUALS( flt.data()->nthInterval(0).end_str(), "2007-Nov-30 16:17:45" );
      TS_ASSERT_EQUALS( flt.data()->nthValue(0), 5 );

      TS_ASSERT_EQUALS( flt.data()->nthInterval(1).begin_str(), "2007-Nov-30 16:17:50" );
      TS_ASSERT_EQUALS( flt.data()->nthInterval(1).end_str(), "2007-Nov-30 16:18:00" );
      TS_ASSERT_EQUALS( flt.data()->nthValue(1), 5 );

      return;
    }

    void testFilterByPeriod() // Test for Wendou to look at
    {
      TimeSeriesProperty<double> height_log("height_log");
      height_log.addValue("2008-Jun-17 11:10:44", -0.86526);
      height_log.addValue("2008-Jun-17 11:10:45", -1.17843);
      height_log.addValue("2008-Jun-17 11:10:47", -1.27995);
      height_log.addValue("2008-Jun-17 11:20:15", -1.38216);
      height_log.addValue("2008-Jun-17 11:20:16", -1.87435);
      height_log.addValue("2008-Jun-17 11:20:17", -2.70547);
      height_log.addValue("2008-Jun-17 11:20:19", -2.99125);
      height_log.addValue("2008-Jun-17 11:20:20", -3);
      height_log.addValue("2008-Jun-17 11:20:27", -2.98519);
      height_log.addValue("2008-Jun-17 11:20:29", -2.68904);
      height_log.addValue("2008-Jun-17 11:20:30", -2.5);
      height_log.addValue("2008-Jun-17 11:20:38", -2.45909);
      height_log.addValue("2008-Jun-17 11:20:39", -2.08764);
      height_log.addValue("2008-Jun-17 11:20:40", -2);
      height_log.addValue("2008-Jun-17 11:20:50", -1.85174);
      height_log.addValue("2008-Jun-17 11:20:51", -1.51258);
      height_log.addValue("2008-Jun-17 11:20:52", -1.5);
      height_log.addValue("2008-Jun-17 11:21:01", -1.48566);
      height_log.addValue("2008-Jun-17 11:21:02", -1.18799);
      height_log.addValue("2008-Jun-17 11:21:04", -1);
      height_log.addValue("2008-Jun-17 11:21:11", -0.98799);
      height_log.addValue("2008-Jun-17 11:21:13", -0.63694);
      height_log.addValue("2008-Jun-17 11:21:14", -0.5);
      height_log.addValue("2008-Jun-17 11:21:23", -0.46247);
      height_log.addValue("2008-Jun-17 11:21:24", -0.08519);
      height_log.addValue("2008-Jun-17 11:21:25", 0);

      TimeSeriesProperty<bool> period_log("period 7");
      period_log.addValue("2008-Jun-17 11:11:13", false);
      period_log.addValue("2008-Jun-17 11:11:13", false);
      period_log.addValue("2008-Jun-17 11:11:18", false);
      period_log.addValue("2008-Jun-17 11:11:30", false);
      period_log.addValue("2008-Jun-17 11:11:42", false);
      period_log.addValue("2008-Jun-17 11:11:52", false);
      period_log.addValue("2008-Jun-17 11:12:01", false);
      period_log.addValue("2008-Jun-17 11:12:11", false);
      period_log.addValue("2008-Jun-17 11:12:21", true);
      period_log.addValue("2008-Jun-17 11:12:32", false);
      period_log.addValue("2008-Jun-17 11:12:42", false);
      period_log.addValue("2008-Jun-17 11:12:52", false);
      period_log.addValue("2008-Jun-17 11:13:02", false);
      period_log.addValue("2008-Jun-17 11:16:55", false);
      period_log.addValue("2008-Jun-17 11:17:00", false);
      period_log.addValue("2008-Jun-17 11:17:16", false);
      period_log.addValue("2008-Jun-17 11:17:28", false);
      period_log.addValue("2008-Jun-17 11:17:37", false);
      period_log.addValue("2008-Jun-17 11:17:48", false);
      period_log.addValue("2008-Jun-17 11:17:57", false);
      period_log.addValue("2008-Jun-17 11:18:07", true);
      period_log.addValue("2008-Jun-17 11:18:18", false);
      period_log.addValue("2008-Jun-17 11:18:28", false);
      period_log.addValue("2008-Jun-17 11:18:38", false);
      period_log.addValue("2008-Jun-17 11:18:48", false);
      period_log.addValue("2008-Jun-17 11:20:07", false);
      period_log.addValue("2008-Jun-17 11:20:11", false);
      period_log.addValue("2008-Jun-17 11:20:24", false);
      period_log.addValue("2008-Jun-17 11:20:34", false);
      period_log.addValue("2008-Jun-17 11:20:46", false);
      period_log.addValue("2008-Jun-17 11:20:58", false);
      period_log.addValue("2008-Jun-17 11:21:08", false);
      period_log.addValue("2008-Jun-17 11:21:19", true);
      
      TS_ASSERT_EQUALS(height_log.size(), 26);
      
      LogFilter filter(&height_log);
      filter.addFilter(&period_log); // This throws when it shouldn't
      const TimeSeriesProperty<double> *filteredLog = filter.data();
      TS_ASSERT_EQUALS(filteredLog->size(), 6)

      return;
    }


private:
  std::time_t createTime_t_FromString(const std::string &str)
  {
    std::tm time_since_1900;
    time_since_1900.tm_isdst = -1;

    // create tm struct

    time_since_1900.tm_year = atoi(str.substr(0, 4).c_str()) - 1900;
    time_since_1900.tm_mon = atoi(str.substr(5, 2).c_str()) - 1;
    time_since_1900.tm_mday = atoi(str.substr(8, 2).c_str());
    time_since_1900.tm_hour = atoi(str.substr(11, 2).c_str());
    time_since_1900.tm_min = atoi(str.substr(14, 2).c_str());
    time_since_1900.tm_sec = atoi(str.substr(17, 2).c_str());

    return std::mktime(&time_since_1900);
  }
};


#endif /*LOGFILTERTEST_H_*/
