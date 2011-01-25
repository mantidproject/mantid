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

        TS_ASSERT_EQUALS( flt.data()->size(), 6 );

        TS_ASSERT_EQUALS( flt.data()->nthInterval(0).begin_str(), "2007-Nov-30 16:16:50" );
        TS_ASSERT_EQUALS( flt.data()->nthInterval(0).end_str(), "2007-Nov-30 16:17:00" );
        TS_ASSERT_EQUALS( flt.data()->nthValue(0), 1 );

        TS_ASSERT_EQUALS( flt.data()->nthInterval(1).begin_str(), "2007-Nov-30 16:17:00" );
        TS_ASSERT_EQUALS( flt.data()->nthInterval(1).end_str(), "2007-Nov-30 16:17:10" );
        TS_ASSERT_EQUALS( flt.data()->nthValue(1), 1 );

        TS_ASSERT_EQUALS( flt.data()->nthInterval(2).begin_str(), "2007-Nov-30 16:17:10" );
        TS_ASSERT_EQUALS( flt.data()->nthInterval(2).end_str(), "2007-Nov-30 16:17:20" );
        TS_ASSERT_EQUALS( flt.data()->nthValue(2), 2 );

        TS_ASSERT_EQUALS( flt.data()->nthInterval(3).begin_str(), "2007-Nov-30 16:17:20" );
        TS_ASSERT_EQUALS( flt.data()->nthInterval(3).end_str(), "2007-Nov-30 16:17:25" );
        TS_ASSERT_EQUALS( flt.data()->nthValue(3), 3 );

        TS_ASSERT_EQUALS( flt.data()->nthInterval(4).begin_str(), "2007-Nov-30 16:17:39" );
        TS_ASSERT_EQUALS( flt.data()->nthInterval(4).end_str(), "2007-Nov-30 16:17:40" );
        TS_ASSERT_EQUALS( flt.data()->nthValue(4), 4 );

        TS_ASSERT_EQUALS( flt.data()->nthInterval(5).begin_str(), "2007-Nov-30 16:17:40" );
        TS_ASSERT_EQUALS( flt.data()->nthInterval(5).end_str(), "2007-Nov-30 16:17:41" );
        TS_ASSERT_EQUALS( flt.data()->nthValue(5), 5 );

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

    void testF12()
    {
        TimeSeriesProperty<bool> f("1");
        f.addValue("2007-11-30T16:16:50",true);
        f.addValue("2007-11-30T16:17:25",false);
        f.addValue("2007-11-30T16:17:39",true);

        TimeSeriesProperty<bool> f2("2");
        f2.addValue("2007-11-30T16:17:05",false);
        f2.addValue("2007-11-30T16:17:12",true);

        LogFilter flt(p);
        flt.addFilter(&f);
        flt.addFilter(&f2);

        TS_ASSERT_EQUALS( flt.data()->size(), 5 );

        TS_ASSERT_EQUALS( flt.data()->nthInterval(0).begin_str(), "2007-Nov-30 16:16:50" );
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

        TS_ASSERT_EQUALS( flt.data()->nthInterval(0).begin_str(), "2007-Nov-30 16:17:40" );
        TS_ASSERT_EQUALS( flt.data()->nthInterval(0).end_str(), "2007-Nov-30 16:17:45" );
        TS_ASSERT_EQUALS( flt.data()->nthValue(0), 5 );

        TS_ASSERT_EQUALS( flt.data()->nthInterval(1).begin_str(), "2007-Nov-30 16:17:50" );
        TS_ASSERT_EQUALS( flt.data()->nthInterval(1).end_str(), "2007-Nov-30 16:18:00" );
        TS_ASSERT_EQUALS( flt.data()->nthValue(1), 5 );

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
