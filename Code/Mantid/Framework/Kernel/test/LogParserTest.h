#ifndef LOGPARSERTEST_H_
#define LOGPARSERTEST_H_

#include <cxxtest/TestSuite.h>

#include <iostream>
#include <fstream>

#include "MantidKernel/LogParser.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <boost/scoped_ptr.hpp>

#include <Poco/File.h>

namespace{

class TmpFile{
  Poco::File m_file;
public:
  TmpFile(const std::string& fname):m_file(fname){}
  ~TmpFile(){remove();}
  const std::string& path() const {return m_file.path();}
  bool exists() const {return m_file.exists();}
  void remove() {if (m_file.exists()) m_file.remove();}
};

}

using namespace Mantid::Kernel;

class LogParserTest : public CxxTest::TestSuite
{
public:
  static LogParserTest *createSuite() { return new LogParserTest(); }
  static void destroySuite(LogParserTest *suite) { delete suite; }
  
  LogParserTest()
        :log_num_good("TST000000_good.txt"),
         log_num_late("TST000000_late.txt"),
         log_num_early("TST000000_early.txt"),
         log_num_single("TST000000_single.txt"),
         log_str("TST000000_str.txt"),
         icp_file("TST000000_icpevent.txt"),
         log_str_repeat("TST000000_repeat.txt"),
         log_num_repeat("TST000000_num_repeat.txt"),
         log_str_continuations("TST000000_str_continue.txt")
    {
    }
  
    void testGood()
    {
        mkICP();
        mkGood();
        Property *icp_log = LogParser::createLogProperty(icp_file.path(),"icpevent");
        LogParser lp(icp_log);
        Property* p1 = lp.createLogProperty(log_num_good.path(),"good");
        TS_ASSERT(p1);
        TimeSeriesProperty<double>* tp1 = dynamic_cast<TimeSeriesProperty<double>*>(p1);
        std::map<DateAndTime, double> vmap = tp1->valueAsMap();
        std::map<DateAndTime, double>::iterator v= vmap.begin();
        // time 1
        TS_ASSERT_EQUALS(vmap.size(), 9);
        TS_ASSERT_EQUALS(v->second, 1);
        ti_data = v->first.to_tm(); ti = &ti_data;

        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        v++;v++;v++;v++;
        // time 5
        //TS_ASSERT(isNaN(v->second));
        ti_data = v->first.to_tm(); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        // last time
        std::map<DateAndTime, double>::reverse_iterator rv = vmap.rbegin();
        TS_ASSERT_EQUALS(rv->second, 9);
        ti_data = rv->first.to_tm(); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 14);
        TS_ASSERT_EQUALS(ti->tm_min, 3);
        TS_ASSERT_DELTA(timeMean(p1),8.4904, 0.001);

        TS_ASSERT_EQUALS(tp1->nthValue(0),1);
        TS_ASSERT_EQUALS(tp1->nthValue(1),2);
        TS_ASSERT_EQUALS(tp1->nthValue(2),3);
        TS_ASSERT_EQUALS(tp1->nthValue(3),4);
        TS_ASSERT_EQUALS(tp1->nthValue(4),5);
        TS_ASSERT_EQUALS(tp1->nthValue(5),6);
        TS_ASSERT_EQUALS(tp1->nthValue(6),7);
        TS_ASSERT_EQUALS(tp1->nthValue(7),8);

        TS_ASSERT_EQUALS(tp1->firstValue(),1);
        //TS_ASSERT_EQUALS(secondValue(p1),2);
        TS_ASSERT_EQUALS(tp1->lastValue(),9);
        
        delete p1;
        delete icp_log;
    }


    void testLate()
    {
        mkICP();
        mkLate();
        Property *icp_log = LogParser::createLogProperty(icp_file.path(),"icpevent");
        LogParser lp(icp_log);
        Property* p1 = lp.createLogProperty(log_num_late.path(),"late");
        TS_ASSERT(p1);
        TimeSeriesProperty<double>* tp1 = dynamic_cast<TimeSeriesProperty<double>*>(p1);
        std::map<DateAndTime, double> vmap = tp1->valueAsMap();
        std::map<DateAndTime, double>::iterator v= vmap.begin();

        // time 1
        TS_ASSERT_EQUALS(vmap.size(), 8);
        TS_ASSERT_EQUALS(v->second, 2);

        ti_data = v->first.to_tm(); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        v++;v++;v++;v++;
        // time 5
        //TS_ASSERT(isNaN(v->second));
        ti_data = v->first.to_tm(); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        // last time
        std::map<DateAndTime, double>::reverse_iterator rv = vmap.rbegin();
        TS_ASSERT_EQUALS(rv->second, 9);
        ti_data = rv->first.to_tm(); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 14);
        TS_ASSERT_EQUALS(ti->tm_min, 3);
        TS_ASSERT_DELTA(timeMean(p1),8.4941, 0.001);
        
        delete p1;
        delete icp_log;
    }

    void testEarly()
    {
        mkICP();
        mkEarly();
        Property *icp_log = LogParser::createLogProperty(icp_file.path(),"icpevent");
        LogParser lp(icp_log);
        Property* p1 = lp.createLogProperty(log_num_early.path(),"early");
        TS_ASSERT(p1);
        TimeSeriesProperty<double>* tp1 = dynamic_cast<TimeSeriesProperty<double>*>(p1);
        std::map<DateAndTime, double> vmap = tp1->valueAsMap();
        std::map<DateAndTime, double>::iterator v= vmap.begin();

        // time 1
        TS_ASSERT_EQUALS(vmap.size(), 8);
        TS_ASSERT_EQUALS(v->second, 1);
        ti_data = v->first.to_tm(); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        v++;v++;v++;v++;
        // time 5
        //TS_ASSERT(isNaN(v->second));
        ti_data = v->first.to_tm(); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        // last time
        std::map<DateAndTime, double>::reverse_iterator rv = vmap.rbegin();
        TS_ASSERT_EQUALS(rv->second, 8);
        ti_data = rv->first.to_tm(); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 23);
        TS_ASSERT_DELTA(timeMean(p1),4.9090, 0.001);

        delete p1;
        delete icp_log;
    }

    void testSingle()
    {
        mkICP();
        mkSingle();
        Property *icp_log = LogParser::createLogProperty(icp_file.path(),"icpevent");
        LogParser lp(icp_log);
        Property* p1 = lp.createLogProperty(log_num_single.path(),"single");
        TS_ASSERT(p1);
        TimeSeriesProperty<double>* tp1 = dynamic_cast<TimeSeriesProperty<double>*>(p1);
        std::map<DateAndTime, double> vmap = tp1->valueAsMap();
        std::map<DateAndTime, double>::iterator v= vmap.begin();

        // time 1
        TS_ASSERT_EQUALS(vmap.size(), 1);
        TS_ASSERT_EQUALS(v->second, 4);
        ti_data = v->first.to_tm(); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        //Can't get a valid mean with a single time and no intervals in it.
        //TS_ASSERT_DELTA(timeMean(p1),4., 0.001);

        delete p1;
        delete icp_log;
    }

    void testStr()
    {
        mkICP();
        mkStr();
        Property *icp_log = LogParser::createLogProperty(icp_file.path(),"icpevent");
        LogParser lp(icp_log);
        Property* p1 = lp.createLogProperty(log_str.path(),"str");
        TS_ASSERT(p1);
        TimeSeriesProperty<std::string>* tp1 = dynamic_cast<TimeSeriesProperty<std::string>*>(p1);
        std::map<DateAndTime, std::string> vmap = tp1->valueAsMap();
        std::map<DateAndTime, std::string>::iterator v= vmap.begin();
        // time 1
        TS_ASSERT_EQUALS(vmap.size(), 9);
        TS_ASSERT_EQUALS(v->second, "   line 1");
        ti_data = v->first.to_tm(); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        v++;v++;v++;
        // time 4
        TS_ASSERT_EQUALS(v->second, "   line 4");
        ti_data = v->first.to_tm(); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        // last time
        std::map<DateAndTime, std::string>::reverse_iterator rv = vmap.rbegin();
        TS_ASSERT_EQUALS(rv->second, "   line 9");
        ti_data = rv->first.to_tm(); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 14);
        TS_ASSERT_EQUALS(ti->tm_min, 3);
        // assert_throws(timeMean(p1));
        delete p1;
        delete icp_log;
    }

    // Test a variant of the log file containing CHANGE_PERIOD flags
    void testConstructionFromFileUsingICPVariant_CHANGE_PERIOD()
    {
      mkICPVariant();
      Property *icp_log = LogParser::createLogProperty(icp_file.path(),"icpevent");
      LogParser lp(icp_log);
      const Property * prop = lp.createAllPeriodsLog();
      const auto *timeseriesprop = dynamic_cast<const TimeSeriesProperty<int>*>(prop);
      TS_ASSERT(timeseriesprop);
      //Check the size
      TS_ASSERT_EQUALS(4, timeseriesprop->size());
      //Check the exact time stamps
      TS_ASSERT_EQUALS(DateAndTime("2000-09-05T12:22:55").toSimpleString(), timeseriesprop->nthTime(0).toSimpleString());
      TS_ASSERT_EQUALS(DateAndTime("2000-09-05T12:23:08").toSimpleString(), timeseriesprop->nthTime(1).toSimpleString());
      TS_ASSERT_EQUALS(DateAndTime("2000-09-05T12:23:22").toSimpleString(), timeseriesprop->nthTime(2).toSimpleString());
      TS_ASSERT_EQUALS(DateAndTime("2000-09-05T12:23:37").toSimpleString(), timeseriesprop->nthTime(3).toSimpleString());

      delete prop;
      delete icp_log;
    }

    void testConstructionFromPropertyUsingICPVariant_CHANGE_PERIOD()
    {
      auto* log  = new TimeSeriesProperty<std::string>("ICPLog");
      // Notice we are using "CHANGE_PERIOD"
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:15:00", "CHANGE_PERIOD 1") );
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:16:00", "CHANGE_PERIOD 2") );
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:17:00", "CHANGE_PERIOD 3") );
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:18:00", "CHANGE_PERIOD 2") );
      
      LogParser logparser(log);

      const Property * prop = logparser.createAllPeriodsLog();
      const auto *timeseriesprop = dynamic_cast<const TimeSeriesProperty<int>*>(prop);
      TS_ASSERT(timeseriesprop);
      //Check the size
      TS_ASSERT_EQUALS(4, timeseriesprop->size());
       //Check the exact time stamps
      TS_ASSERT_EQUALS(DateAndTime("2007-11-30T16:15:00").toSimpleString(), timeseriesprop->nthTime(0).toSimpleString());
      TS_ASSERT_EQUALS(DateAndTime("2007-11-30T16:16:00").toSimpleString(), timeseriesprop->nthTime(1).toSimpleString());
      TS_ASSERT_EQUALS(DateAndTime("2007-11-30T16:17:00").toSimpleString(), timeseriesprop->nthTime(2).toSimpleString());
      TS_ASSERT_EQUALS(DateAndTime("2007-11-30T16:18:00").toSimpleString(), timeseriesprop->nthTime(3).toSimpleString());

      delete log;
      delete prop;
    }

    void testConstructionFromPropertyUsingICPVariant_CHANGE_SPACE_PERIOD()
    {
      auto* log  = new TimeSeriesProperty<std::string>("ICPLog");
      // Notice we are using "CHANGE PERIOD"
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:15:00", "CHANGE PERIOD 1") );
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:16:00", "CHANGE PERIOD 2") );
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:17:00", "CHANGE PERIOD 3") );
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:18:00", "CHANGE PERIOD 2") );
      
      LogParser logparser(log);

      const Property *prop = logparser.createAllPeriodsLog();
      const auto *timeseriesprop = dynamic_cast<const TimeSeriesProperty<int>*>(prop);
      TS_ASSERT(timeseriesprop);
      //Check the size
      TS_ASSERT_EQUALS(4, timeseriesprop->size());
       //Check the exact time stamps
      TS_ASSERT_EQUALS(DateAndTime("2007-11-30T16:15:00").toSimpleString(), timeseriesprop->nthTime(0).toSimpleString());
      TS_ASSERT_EQUALS(DateAndTime("2007-11-30T16:16:00").toSimpleString(), timeseriesprop->nthTime(1).toSimpleString());
      TS_ASSERT_EQUALS(DateAndTime("2007-11-30T16:17:00").toSimpleString(), timeseriesprop->nthTime(2).toSimpleString());
      TS_ASSERT_EQUALS(DateAndTime("2007-11-30T16:18:00").toSimpleString(), timeseriesprop->nthTime(3).toSimpleString());

      delete log;
      delete prop;
    }

    // Check that periods that don't have a full "CHANGE PERIOD" flag are not added.
    void testWontAddPeriodWithoutPERIODpartOfCHANGE_SPACE_PERIOD()
    {
      auto* log  = new TimeSeriesProperty<std::string>("ICPLog");
      // Notice we are using "CHANGE PERIOD"
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:15:00", "CHANGE PERIOD 1") );
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:16:00", "CHANGE PERIOD 2") );
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:17:00", "CHANGE PERIOD 3") );
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:18:00", "CHANGE 2") ); //This is a duff entry. Shouldn't get added.
      
      LogParser logparser(log);

      const Property *prop = logparser.createAllPeriodsLog();
      const auto *timeseriesprop = dynamic_cast<const TimeSeriesProperty<int>*>(prop);
      TS_ASSERT(timeseriesprop);
      //Check the size
      TS_ASSERT_EQUALS(3, timeseriesprop->size());
       //Check the exact time stamps
      TS_ASSERT_EQUALS(DateAndTime("2007-11-30T16:15:00").toSimpleString(), timeseriesprop->nthTime(0).toSimpleString());
      TS_ASSERT_EQUALS(DateAndTime("2007-11-30T16:16:00").toSimpleString(), timeseriesprop->nthTime(1).toSimpleString());
      TS_ASSERT_EQUALS(DateAndTime("2007-11-30T16:17:00").toSimpleString(), timeseriesprop->nthTime(2).toSimpleString());

      delete log;
      delete prop;
    }
    
    void testCreatesCurrentPeriodLog()
    {
      //Check it with a few expected period numbers.
      doTestCurrentPeriodLog(1);
      doTestCurrentPeriodLog(2);
      doTestCurrentPeriodLog(3);
    }

    void testNoICPevent()
    {
      if ( icp_file.exists() ) icp_file.remove();
        mkGood();
        Property *icp_log = LogParser::createLogProperty(icp_file.path(),"icpevent");
        LogParser lp(icp_log);
        Property* p1 = lp.createLogProperty(log_num_good.path(),"good");
        TS_ASSERT(p1);
        TimeSeriesProperty<double>* tp1 = dynamic_cast<TimeSeriesProperty<double>*>(p1);
        std::map<DateAndTime, double> vmap = tp1->valueAsMap();
        std::map<DateAndTime, double>::iterator v= vmap.begin();

        // time 1
        TS_ASSERT_EQUALS(vmap.size(), 9);
        TS_ASSERT_EQUALS(v->second, 1);
        ti_data = v->first.to_tm(); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        v++;v++;v++;v++;
        // time 5
        //TS_ASSERT(!isNaN(v->second));
        // last time
        std::map<DateAndTime, double>::reverse_iterator rv = vmap.rbegin();
        TS_ASSERT_EQUALS(rv->second, 9);
        ti_data = rv->first.to_tm(); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 14);
        TS_ASSERT_EQUALS(ti->tm_min, 3);
        TS_ASSERT_DELTA(timeMean(p1),8.4904, 0.001);

        delete p1;
    }



    //----------------------------------------------------------------------------
    void test_timeMean()
    {
      TimeSeriesProperty<double> * log  = new TimeSeriesProperty<double>("MydoubleLog");
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:17:00",1) );
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:17:10",2) );
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:17:20",3) );
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:17:30",4) );
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:17:40",5) );
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:17:50",6) );
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:18:00",7) );
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:18:10",8) );
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:18:20",9) );
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:18:30",10) );
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:18:40",11) );
      TS_ASSERT_EQUALS( log->realSize(), 11);

      TS_ASSERT_DELTA(timeMean(log), 6.0, 1e-3);
      delete log;
    }

    void test_timeMean_one_Value()
    {
      TimeSeriesProperty<double> * log  = new TimeSeriesProperty<double>("MydoubleLog");
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:17:00",56) );
      TS_ASSERT_EQUALS( log->realSize(), 1);

      TS_ASSERT_DELTA(timeMean(log), 56.0, 1e-3);
      delete log;
    }

    /// Tests to see if we can cope with duplicate log values that have the same time.
    void test_timeMean_duplicate_values_with_same_timestamp()
    {
        TimeSeriesProperty<double> * log = new TimeSeriesProperty<double>("MydoubleLog");
        // Add the same value twice
        TS_ASSERT_THROWS_NOTHING( log->addValue("2012-07-19T20:00:00", 666) );
        TS_ASSERT_THROWS_NOTHING( log->addValue("2012-07-19T20:00:00", 666) );
        TS_ASSERT_EQUALS( log->realSize(), 2);
        TS_ASSERT_DELTA( timeMean(log), 666, 1e-3);
        delete log;
    }

    void test_isICPEventLogNewStyle_works()
    {
        TimeSeriesProperty<std::string> * oldlog = new TimeSeriesProperty<std::string>("MyOldICPevent");
        TS_ASSERT_THROWS_NOTHING( oldlog->addValue("2012-07-19T20:00:00", "START") );
        TS_ASSERT_THROWS_NOTHING( oldlog->addValue("2012-07-19T20:00:01", "BEGIN") );
        TS_ASSERT_THROWS_NOTHING( oldlog->addValue("2012-07-19T20:00:02", "PAUSE") );

        auto logm = oldlog->valueAsMultiMap();
        TS_ASSERT( !LogParser::isICPEventLogNewStyle(logm) );
        delete oldlog;

        TimeSeriesProperty<std::string> * newlog = new TimeSeriesProperty<std::string>("MyNewICPevent");
        TS_ASSERT_THROWS_NOTHING( newlog->addValue("2012-07-19T20:00:00", "START") );
        TS_ASSERT_THROWS_NOTHING( newlog->addValue("2012-07-19T20:00:01", "START_COLLECTION PERIOD 1") );
        TS_ASSERT_THROWS_NOTHING( newlog->addValue("2012-07-19T20:00:02", "PAUSE") );

        logm = newlog->valueAsMultiMap();
        TS_ASSERT( LogParser::isICPEventLogNewStyle(logm) );
        delete newlog;

        newlog = new TimeSeriesProperty<std::string>("MyNewICPevent1");
        TS_ASSERT_THROWS_NOTHING( newlog->addValue("2012-07-19T20:00:00", "START") );
        TS_ASSERT_THROWS_NOTHING( newlog->addValue("2012-07-19T20:00:01", "STOP_COLLECTION PERIOD 1") );
        TS_ASSERT_THROWS_NOTHING( newlog->addValue("2012-07-19T20:00:02", "PAUSE") );

        logm = newlog->valueAsMultiMap();
        TS_ASSERT( LogParser::isICPEventLogNewStyle(logm) );
        delete newlog;
    }

    void test_new_style_command_parsing()
    {
        TimeSeriesProperty<std::string> * log = new TimeSeriesProperty<std::string>("MyICPevent");
        log->addValue("2013-10-16T19:04:47", "CHANGE_PERIOD 1");
        log->addValue("2013-10-16T19:04:48", "RESUME");
        log->addValue("2013-10-16T19:04:48", "START_COLLECTION PERIOD 1 GF 60015 RF 75039 GUAH 69.875610");
        log->addValue("2013-10-16T19:06:53", "STOP_COLLECTION PERIOD 1 GF 65024 RF 81303 GUAH 75.712013 DUR 125");
        log->addValue("2013-10-16T19:06:53", "PAUSE");
        log->addValue("2013-10-16T19:06:53", "CHANGE_PERIOD 2");
        log->addValue("2013-10-16T19:06:53", "RESUME");
        log->addValue("2013-10-16T19:06:53", "START_COLLECTION PERIOD 2 GF 65024 RF 81303 GUAH 75.712013");
        log->addValue("2013-10-16T19:08:58", "STOP_COLLECTION PERIOD 2 GF 70033 RF 87567 GUAH 81.547050 DUR 125");
        log->addValue("2013-10-16T19:08:58", "PAUSE");
        log->addValue("2013-10-16T19:08:58", "CHANGE_PERIOD 1");
        log->addValue("2013-10-16T19:08:59", "RESUME");
        log->addValue("2013-10-16T19:08:59", "START_COLLECTION PERIOD 1 GF 70033 RF 87567 GUAH 81.547050");
        log->addValue("2013-10-16T19:11:03", "STOP_COLLECTION PERIOD 1 GF 75005 RF 93784 GUAH 87.339035 DUR 124");
        log->addValue("2013-10-16T19:11:03", "PAUSE");
        log->addValue("2013-10-16T19:11:03", "CHANGE_PERIOD 2");
        log->addValue("2013-10-16T19:11:04", "RESUME");
        log->addValue("2013-10-16T19:11:04", "START_COLLECTION PERIOD 2 GF 75005 RF 93784 GUAH 87.339035");
        log->addValue("2013-10-16T19:13:09", "STOP_COLLECTION PERIOD 2 GF 80016 RF 100049 GUAH 93.174751 DUR 125");
        log->addValue("2013-10-16T19:13:09", "PAUSE");
        log->addValue("2013-10-16T19:13:09", "CHANGE_PERIOD 1");
        log->addValue("2013-10-16T19:13:09", "RESUME");

        std::vector< std::pair<std::string,int> > checkPeriod(5);
        checkPeriod[0] = std::make_pair("2013-10-16T19:04:47", 1 );
        checkPeriod[1] = std::make_pair("2013-10-16T19:06:53", 2 );
        checkPeriod[2] = std::make_pair("2013-10-16T19:08:58", 1 );
        checkPeriod[3] = std::make_pair("2013-10-16T19:11:03", 2 );
        checkPeriod[4] = std::make_pair("2013-10-16T19:13:09", 1 );

        std::vector< std::pair<std::string,bool> > checkRunning(8);
        checkRunning[0] = std::make_pair("2013-10-16T19:04:48", true );
        checkRunning[1] = std::make_pair("2013-10-16T19:06:53", false );
        checkRunning[2] = std::make_pair("2013-10-16T19:06:53", true );
        checkRunning[3] = std::make_pair("2013-10-16T19:08:58", false );
        checkRunning[4] = std::make_pair("2013-10-16T19:08:59", true );
        checkRunning[5] = std::make_pair("2013-10-16T19:11:03", false );
        checkRunning[6] = std::make_pair("2013-10-16T19:11:04", true );
        checkRunning[7] = std::make_pair("2013-10-16T19:13:09", false );

        LogParser logparser( log );

        const Property *prop = logparser.createAllPeriodsLog();
        const auto *allPeriodsProp = dynamic_cast<const TimeSeriesProperty<int>*>(prop);
        TS_ASSERT(allPeriodsProp);

        TS_ASSERT_EQUALS(5, allPeriodsProp->size());
        auto logm = allPeriodsProp->valueAsMultiMap();
        size_t i = 0;
        for(auto it = logm.begin(); it != logm.end(); ++it)
        {
            TS_ASSERT_EQUALS( it->first.toISO8601String(), checkPeriod[i].first );
            TS_ASSERT_EQUALS( it->second, checkPeriod[i].second );
            ++i;
        }

        delete prop;
        prop = logparser.createRunningLog();
        const auto *runningProp = dynamic_cast<const TimeSeriesProperty<bool>*>(prop);
        TS_ASSERT(runningProp);

        TS_ASSERT_EQUALS(8, runningProp->size());
        auto logm1 = runningProp->valueAsMultiMap();
        i = 0;
        for(auto it = logm1.begin(); it != logm1.end(); ++it)
        {
            TS_ASSERT_EQUALS( it->first.toISO8601String(), checkRunning[i].first );
            TS_ASSERT_EQUALS( it->second, checkRunning[i].second );
            ++i;
        }

        delete prop;
        delete log;
    }

    void test_str_repeat()
    {
      mkStrRepeat();
      Property *prop = LogParser::createLogProperty(log_str_repeat.path(),"log");
      const auto *log = dynamic_cast<const TimeSeriesProperty<std::string>*>(prop);
      TS_ASSERT(log);
      auto logm = log->valueAsMultiMap();
      auto it = logm.begin();
      TS_ASSERT_EQUALS( it->first.toISO8601String(), "2000-09-05T12:22:34");
      TS_ASSERT_EQUALS( it->second, "   First line"); ++it;
      TS_ASSERT_EQUALS( it->first.toISO8601String(), "2000-09-05T12:22:34");
      TS_ASSERT_EQUALS( it->second, "   Second line"); ++it;
      TS_ASSERT_EQUALS( it->first.toISO8601String(), "2000-09-05T12:23:33");
      TS_ASSERT_EQUALS( it->second, "   First line"); ++it;
      TS_ASSERT_EQUALS( it->first.toISO8601String(), "2000-09-05T12:23:33");
      TS_ASSERT_EQUALS( it->second, "   Second line"); ++it;
      TS_ASSERT_EQUALS( it->first.toISO8601String(), "2000-09-05T12:23:33");
      TS_ASSERT_EQUALS( it->second, "   Third line"); ++it;
      TS_ASSERT_EQUALS( it->first.toISO8601String(), "2000-09-05T12:23:33");
      TS_ASSERT_EQUALS( it->second, "   Fourth line"); ++it;
    }

    void test_num_repeat()
    {
      mkNumRepeat();
      Property *prop = LogParser::createLogProperty(log_str_repeat.path(),"log");
      const auto *log = dynamic_cast<const TimeSeriesProperty<double>*>(prop);
      TS_ASSERT(log);
      auto logm = log->valueAsMultiMap();
      auto it = logm.begin();
      TS_ASSERT_EQUALS( it->first.toISO8601String(), "2000-09-05T12:22:34");
      TS_ASSERT_EQUALS( it->second, 1); ++it;
      TS_ASSERT_EQUALS( it->first.toISO8601String(), "2000-09-05T12:22:34");
      TS_ASSERT_EQUALS( it->second, 2); ++it;
      TS_ASSERT_EQUALS( it->first.toISO8601String(), "2000-09-05T12:23:33");
      TS_ASSERT_EQUALS( it->second, 3); ++it;
      TS_ASSERT_EQUALS( it->first.toISO8601String(), "2000-09-05T12:23:33");
      TS_ASSERT_EQUALS( it->second, 4); ++it;
      TS_ASSERT_EQUALS( it->first.toISO8601String(), "2000-09-05T12:23:33");
      TS_ASSERT_EQUALS( it->second, 5); ++it;
      TS_ASSERT_EQUALS( it->first.toISO8601String(), "2000-09-05T12:23:33");
      TS_ASSERT_EQUALS( it->second, 6); ++it;
    }

    void test_str_continuation()
    {
      mkStrContinuations();
      Property *prop = LogParser::createLogProperty(log_str_continuations.path(),"log");
      const auto *log = dynamic_cast<const TimeSeriesProperty<std::string>*>(prop);
      TS_ASSERT(log);
      auto logm = log->valueAsMultiMap();
      auto it = logm.begin();
      TS_ASSERT_EQUALS( it->first.toISO8601String(), "2000-09-05T12:22:31");
      TS_ASSERT_EQUALS( it->second, "   First line Second line"); ++it;
      TS_ASSERT_EQUALS( it->first.toISO8601String(), "2000-09-05T12:22:34");
      TS_ASSERT_EQUALS( it->second, "   First line"); ++it;
      TS_ASSERT_EQUALS( it->first.toISO8601String(), "2000-09-05T12:22:34");
      TS_ASSERT_EQUALS( it->second, "   Second line Third line"); ++it;
    }


private:

    /// Helper method to run common test code for checking period logs.
    void doTestCurrentPeriodLog(const int& expected_period)
    {
      auto* log  = new TimeSeriesProperty<std::string>("ICPLog");
      LogParser logparser(log);
      Property* prop = logparser.createCurrentPeriodLog(expected_period);
      PropertyWithValue<int>* prop_with_value = dynamic_cast<PropertyWithValue<int>* >(prop);

      int value;
      TS_ASSERT(prop_with_value != NULL);
      Mantid::Kernel::toValue<int>(prop_with_value->value(), value);
      TS_ASSERT_EQUALS(expected_period, value);
      delete prop;
      delete log;
    }

    void mkICP()
    {
        std::ofstream f( icp_file.path().c_str());
        int dt = 0;
        f << "2000-09-05T12:22:28   START_SE_WAIT"<<'\n';
        f << "2000-09-05T12:22:33   BEGIN"<<'\n';dt+=8;
        f << "2000-09-05T12:22:41   PAUSE"<<'\n';dt+=4;
        f << "2000-09-05T12:22:55   CHANGE PERIOD 2"<<'\n';dt+=3;
        f << "2000-09-05T12:22:58   RESUME"<<'\n';dt+=6;
        f << "2000-09-05T12:23:04   PAUSE"<<'\n';dt+=4;
        f << "2000-09-05T12:23:08   CHANGE PERIOD 1"<<'\n';dt+=2;
        f << "2000-09-05T12:23:10   RESUME"<<'\n';dt+=8;
        f << "2000-09-05T12:23:18   START_SE_WAIT"<<'\n';dt+=4;
        f << "2000-09-05T12:23:22   CHANGE PERIOD 2"<<'\n';dt+=5;
        f << "2000-09-05T12:23:27   RESUME"<<'\n';dt+=7;
        f << "2000-09-05T12:23:34   ABORT"<<'\n';dt+=3;
        f << "2000-09-05T12:23:37   CHANGE PERIOD 1"<<'\n';dt+=5;
        f << "2000-09-05T12:23:42   END_SE_WAIT"<<'\n';
        f << "2000-09-05T14:03:54   END"<<'\n';
        f.close();
    }

    void mkICPVariant()
    {
        std::ofstream f( icp_file.path().c_str());
        int dt = 0;
        f << "2000-09-05T12:22:28   START_SE_WAIT"<<'\n';
        f << "2000-09-05T12:22:33   BEGIN"<<'\n';dt+=8;
        f << "2000-09-05T12:22:41   PAUSE"<<'\n';dt+=4;
        f << "2000-09-05T12:22:55   CHANGE_PERIOD 2"<<'\n';dt+=3;
        f << "2000-09-05T12:22:58   RESUME"<<'\n';dt+=6;
        f << "2000-09-05T12:23:04   PAUSE"<<'\n';dt+=4;
        f << "2000-09-05T12:23:08   CHANGE_PERIOD 1"<<'\n';dt+=2;
        f << "2000-09-05T12:23:10   RESUME"<<'\n';dt+=8;
        f << "2000-09-05T12:23:18   START_SE_WAIT"<<'\n';dt+=4;
        f << "2000-09-05T12:23:22   CHANGE_PERIOD 2"<<'\n';dt+=5;
        f << "2000-09-05T12:23:27   RESUME"<<'\n';dt+=7;
        f << "2000-09-05T12:23:34   ABORT"<<'\n';dt+=3;
        f << "2000-09-05T12:23:37   CHANGE_PERIOD 1"<<'\n';dt+=5;
        f << "2000-09-05T12:23:42   END_SE_WAIT"<<'\n';
        f << "2000-09-05T14:03:54   END"<<'\n';
        f.close();
    }

    void mkGood()
    {
        std::ofstream f( log_num_good.path().c_str());
        int dt = 4;
        f << "2000-09-05T12:22:31   "<<1<<'\n';
        f << "2000-09-05T12:22:37   "<<2<<'\n';dt+=1;
        f << "2000-09-05T12:22:38   "<<3<<'\n';dt+=1;
        f << "2000-09-05T12:22:39   "<<4<<'\n';dt+=3;
        f << "2000-09-05T12:22:42   "<<5<<'\n';dt+=5;
        f << "2000-09-05T12:22:47   "<<6<<'\n';dt+=9;
        f << "2000-09-05T12:22:56   "<<7<<'\n';dt+=4;
        f << "2000-09-05T12:23:00   "<<8<<'\n';
        f << "2000-09-05T14:03:56   "<<9<<'\n';
        f.close();
    }

    void mkLate()
    {
        std::ofstream f( log_num_late.path().c_str());
        int dt = 4;
        f << "2000-09-05T12:22:37   "<<2<<'\n';dt+=1;
        f << "2000-09-05T12:22:38   "<<3<<'\n';dt+=1;
        f << "2000-09-05T12:22:39   "<<4<<'\n';dt+=3;
        f << "2000-09-05T12:22:42   "<<5<<'\n';dt+=5;
        f << "2000-09-05T12:22:47   "<<6<<'\n';dt+=9;
        f << "2000-09-05T12:22:56   "<<7<<'\n';dt+=4;
        f << "2000-09-05T12:23:00   "<<8<<'\n';
        f << "2000-09-05T14:03:56   "<<9<<'\n';
        f.close();
    }

    void mkEarly()
    {
        std::ofstream f( log_num_early.path().c_str());
        int dt = 4;
        f << "2000-09-05T12:22:31   "<<1<<'\n';
        f << "2000-09-05T12:22:37   "<<2<<'\n';dt+=1;
        f << "2000-09-05T12:22:38   "<<3<<'\n';dt+=1;
        f << "2000-09-05T12:22:39   "<<4<<'\n';dt+=3;
        f << "2000-09-05T12:22:42   "<<5<<'\n';dt+=5;
        f << "2000-09-05T12:22:47   "<<6<<'\n';dt+=9;
        f << "2000-09-05T12:22:56   "<<7<<'\n';dt+=4;
        f << "2000-09-05T12:23:00   "<<8<<'\n';
        f.close();
    }

    void mkSingle()
    {
        std::ofstream f( log_num_single.path().c_str());
        f << "2000-09-05T12:22:51   "<<4<<'\n';
        f.close();
    }

    void mkStr()
    {
        std::ofstream f( log_str.path().c_str());
        int dt = 4;
        f << "2000-09-05T12:22:31   line "<<1<<'\n';
        f << "2000-09-05T12:22:37   line "<<2<<'\n';dt+=1;
        f << "2000-09-05T12:22:38   line "<<3<<'\n';dt+=1;
        f << "2000-09-05T12:22:39   line "<<4<<'\n';dt+=3;
        f << "2000-09-05T12:22:42   line "<<5<<'\n';dt+=5;
        f << "2000-09-05T12:22:47   line "<<6<<'\n';dt+=9;
        f << "2000-09-05T12:22:56   line "<<7<<'\n';dt+=4;
        f << "2000-09-05T12:23:00   line "<<8<<'\n';
        f << "2000-09-05T14:03:56   line "<<9<<'\n';
        f.close();
    }

    void mkStrContinuations()
    {
      std::ofstream f( log_str_continuations.path().c_str() );
      f << "2000-09-05T12:22:31   First line" << std::endl;
      f << "Second line" << std::endl;
      f << "2000-09-05T12:22:34   First line" << std::endl;
      f << "2000-09-05T12:22:34   Second line" << std::endl;
      f << "Third line" << std::endl;
      f.close();
    }

    void mkStrRepeat()
    {
      std::ofstream f( log_str_repeat.path().c_str() );
      f << "2000-09-05T12:22:34   First line" << std::endl;
      f << "2000-09-05T12:22:34   Second line" << std::endl;
      f << "2000-09-05T12:23:33   First line" << std::endl;
      f << "2000-09-05T12:23:33   Second line" << std::endl;
      f << "2000-09-05T12:23:33   Third line" << std::endl;
      f << "2000-09-05T12:23:33   Fourth line" << std::endl;
      f.close();
    }

    void mkNumRepeat()
    {
      std::ofstream f( log_str_repeat.path().c_str() );
      f << "2000-09-05T12:22:34   1" << std::endl;
      f << "2000-09-05T12:22:34   2" << std::endl;
      f << "2000-09-05T12:23:33   3" << std::endl;
      f << "2000-09-05T12:23:33   4" << std::endl;
      f << "2000-09-05T12:23:33   5" << std::endl;
      f << "2000-09-05T12:23:33   6" << std::endl;
      f.close();
    }

    TmpFile log_num_good;// run time interval is within first - last times of the log
    TmpFile log_num_late;// first time is later than run start
    TmpFile log_num_early;// last time is earlier than run ends
    TmpFile log_num_single;// single value
    TmpFile log_str;// file of strings
    TmpFile icp_file;// icpevent file
    TmpFile log_str_repeat;// string log with repeating lines
    TmpFile log_num_repeat;// num log with repeating lines
    TmpFile log_str_continuations;// string log with continuation lines

    tm ti_data;
    tm * ti;

};
  
#endif /*LOGPARSERTEST_H_*/
