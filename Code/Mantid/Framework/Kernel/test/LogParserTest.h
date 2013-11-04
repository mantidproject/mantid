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
         icp_file("TST000000_icpevent.txt")
    {
    }
  
  ~LogParserTest()
    {
      if ( log_num_good.exists() ) log_num_good.remove();
      if ( log_num_late.exists() ) log_num_late.remove();
      if ( log_num_early.exists() ) log_num_early.remove();
      if ( log_num_single.exists() ) log_num_single.remove();
      if ( log_str.exists() ) log_str.remove();
      if ( icp_file.exists() ) icp_file.remove();
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
    }
    
    void test_begin_end_treated_same_as_start_collection_stop_collection()
    {
      boost::scoped_ptr<TimeSeriesProperty<std::string> > logICPBeginEnd(new TimeSeriesProperty<std::string>("ICPLog1"));
      TS_ASSERT_THROWS_NOTHING( logICPBeginEnd->addValue("2000-01-01T00:00:00", "BEGIN") );
      TS_ASSERT_THROWS_NOTHING( logICPBeginEnd->addValue("2000-01-01T01:00:00", "END") );
      
      boost::scoped_ptr<TimeSeriesProperty<std::string> > logICPCollectStartStop(new TimeSeriesProperty<std::string>("ICPLog2"));
      TS_ASSERT_THROWS_NOTHING( logICPCollectStartStop->addValue("2000-01-01T00:00:00", "START_COLLECTION") );
      TS_ASSERT_THROWS_NOTHING( logICPCollectStartStop->addValue("2000-01-01T01:00:00", "STOP_COLLECTION") );
      
      LogParser logParserBeginEnd(logICPBeginEnd.get());
      TimeSeriesProperty<bool>* maskBeginEnd = logParserBeginEnd.createRunningLog();
      
      LogParser logParserCollectStartStop(logICPCollectStartStop.get());
      TimeSeriesProperty<bool>* maskCollectStartStop = logParserCollectStartStop.createRunningLog();
      
      TSM_ASSERT_EQUALS("Should have 2 entries", 2, maskCollectStartStop->size());
      TSM_ASSERT_EQUALS("Masks should be equal length", maskBeginEnd->size(), maskCollectStartStop->size());
      
      TSM_ASSERT("Mask should NOT applied Due to start marker", maskCollectStartStop->nthValue(0)) // Mask OFF
      TSM_ASSERT("Mask SHOULD applied Due to stop marker", !maskCollectStartStop->nthValue(1)) // Mask ON
      // Compare for consistency.
      for(int i = 0; i < maskBeginEnd->size(); ++i)
      {
        TS_ASSERT_EQUALS(maskBeginEnd->nthTime(i), maskCollectStartStop->nthTime(i));
        TS_ASSERT_EQUALS(maskBeginEnd->nthValue(i), maskCollectStartStop->nthValue(i));
      }
    }
    
    void test_mixed_start_stop_begin_end()
    {
      boost::scoped_ptr<TimeSeriesProperty<std::string> > logICP(new TimeSeriesProperty<std::string>("ICPLog"));
      TS_ASSERT_THROWS_NOTHING( logICP->addValue("2000-01-01T00:00:00", "BEGIN") );
      TS_ASSERT_THROWS_NOTHING( logICP->addValue("2000-01-01T00:00:00", "START_COLLECTION") );
      TS_ASSERT_THROWS_NOTHING( logICP->addValue("2000-01-01T00:10:00", "STOP_COLLECTION") );
      TS_ASSERT_THROWS_NOTHING( logICP->addValue("2000-01-01T00:20:00", "START_COLLECTION") );
      TS_ASSERT_THROWS_NOTHING( logICP->addValue("2000-01-01T01:30:00", "STOP_COLLECTION") );
      TS_ASSERT_THROWS_NOTHING( logICP->addValue("2000-01-01T01:30:00", "END") );
      
      LogParser logParser(logICP.get());
      TimeSeriesProperty<bool>* mask = logParser.createRunningLog();

      TSM_ASSERT_EQUALS("Should have 4 entries, 2 of the 6 are duplicates", 4, mask->size()); 
      int increment = 0;
      TSM_ASSERT("Mask OFF", mask->nthValue(increment++));
      TSM_ASSERT("Mask ON", !mask->nthValue(increment++));
      TSM_ASSERT("Mask OFF", mask->nthValue(increment++));
      TSM_ASSERT("Mask ON", !mask->nthValue(increment));
    }
    
    void test_multiple_starts_ok()
    {
      boost::scoped_ptr<TimeSeriesProperty<std::string> > logICP(new TimeSeriesProperty<std::string>("ICPLog"));
      TS_ASSERT_THROWS_NOTHING( logICP->addValue("2000-01-01T00:00:00", "BEGIN") );
      TS_ASSERT_THROWS_NOTHING( logICP->addValue("2000-01-01T00:10:00", "START_COLLECTION") );
      
      LogParser logParser(logICP.get());
      TimeSeriesProperty<bool>* mask = logParser.createRunningLog();
      
      TSM_ASSERT_EQUALS("Should have 2 entries", 2, mask->size()); 
      int increment = 0;
      TSM_ASSERT("Mask OFF", mask->nthValue(increment++)); // BEGIN
      TSM_ASSERT("Mask should still be OFF", mask->nthValue(increment++)); // START COLLECT
    }
    
    void test_multiple_ends_ok()
    {
      boost::scoped_ptr<TimeSeriesProperty<std::string> > logICP(new TimeSeriesProperty<std::string>("ICPLog"));
      TS_ASSERT_THROWS_NOTHING( logICP->addValue("2000-01-01T00:00:00", "STOP_COLLECTION") );
      TS_ASSERT_THROWS_NOTHING( logICP->addValue("2000-01-01T00:10:00", "END") );
      
      LogParser logParser(logICP.get());
      TimeSeriesProperty<bool>* mask = logParser.createRunningLog();
      
      TSM_ASSERT_EQUALS("Should have 2 entries", 2, mask->size()); 
      int increment = 0;
      TSM_ASSERT("Mask ON", !mask->nthValue(increment++)); // STOP COLLECT
      TSM_ASSERT("Mask should still be ON", !mask->nthValue(increment++)); // END
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
    }

    void test_timeMean_one_Value()
    {
      TimeSeriesProperty<double> * log  = new TimeSeriesProperty<double>("MydoubleLog");
      TS_ASSERT_THROWS_NOTHING( log->addValue("2007-11-30T16:17:00",56) );
      TS_ASSERT_EQUALS( log->realSize(), 1);

      TS_ASSERT_DELTA(timeMean(log), 56.0, 1e-3);
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
    }

//*/
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
//*/
    Poco::File log_num_good;// run time interval is within first - last times of the log
    Poco::File log_num_late;// first time is later than run start
    Poco::File log_num_early;// last time is earlier than run ends
    Poco::File log_num_single;// single value
    Poco::File log_str;// file of strings
    Poco::File icp_file;// icpevent file
    tm ti_data;
    tm * ti;

};
  
#endif /*LOGPARSERTEST_H_*/
