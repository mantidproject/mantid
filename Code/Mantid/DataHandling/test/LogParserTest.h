#ifndef LOGPARSERTEST_H_
#define LOGPARSERTEST_H_

#include <cxxtest/TestSuite.h>

#include <iostream>
#include <sstream>

#include "MantidDataHandling/LogParser.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <boost/timer.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

using namespace boost::posix_time;
using namespace boost::gregorian;

#include <boost/filesystem.hpp>
using namespace boost::filesystem;

class LogParserTest : public CxxTest::TestSuite
{
public: 
  
  LogParserTest()
        :log_num_good("TST000000_good.txt"),
         log_num_late("TST000000_late.txt"),
         log_num_early("TST000000_early.txt"),
         log_num_single("TST000000_single.txt"),
         log_str("TST000000_str.txt"),
         icp_file("TST000000_icpevent.txt"),
         start_time(date(2000,5,9),time_duration(12,22,33)),
         end_time(date(2000,5,9),time_duration(14,03,54))
    {
    }
  
  ~LogParserTest()
    {
        if ( exists(log_num_good) ) remove(log_num_good);
        if ( exists(log_num_late) ) remove(log_num_late);
        if ( exists(log_num_early) ) remove(log_num_early);
        if ( exists(log_num_single) ) remove(log_num_single);
        if ( exists(log_str) ) remove(log_str);
        if ( exists(icp_file) ) remove(icp_file);


    }
  
    void testGood()
    {
        mkICP();
        mkGood();
        LogParser lp(icp_file.file_string());
        Property* p1 = lp.createLogProperty(log_num_good.file_string(),"good",1);
        Property* p2 = lp.createLogProperty(log_num_good.file_string(),"good",2);
        Property* p3 = lp.createLogProperty(log_num_good.file_string(),"good",4);
        TS_ASSERT(p1)
        TS_ASSERT(p2)
        TS_ASSERT(!p3)
        TimeSeriesProperty<double>* tp1 = dynamic_cast<TimeSeriesProperty<double>*>(p1);
        std::map<TimeSeriesProperty<double>::dateAndTime, double> vmap = tp1->valueAsMap();
        std::map<TimeSeriesProperty<double>::dateAndTime, double>::iterator v= vmap.begin();
        // time 1
        TS_ASSERT_EQUALS(vmap.size(), 9);
        TS_ASSERT_EQUALS(v->second, 1);
        tm* ti = localtime(&v->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        v++;v++;v++;v++;
        // time 5
        TS_ASSERT(isNaN(v->second));
        ti = localtime(&v->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 30);
        // last time
        std::map<TimeSeriesProperty<double>::dateAndTime, double>::reverse_iterator rv = vmap.rbegin();
        TS_ASSERT_EQUALS(rv->second, 8);
        ti = localtime(&rv->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 14);
        TS_ASSERT_EQUALS(ti->tm_min, 3);
        TS_ASSERT_DELTA(timeMean(p1),7.194, 0.001);

        TimeSeriesProperty<double>* tp2 = dynamic_cast<TimeSeriesProperty<double>*>(p2);
        vmap = tp2->valueAsMap();
        v= vmap.begin();
        // time 1
        TS_ASSERT_EQUALS(vmap.size(), 4);
        TS_ASSERT_EQUALS(v->second, 6);
        ti = localtime(&v->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 37);
        v++;
        // time 2 
        TS_ASSERT(isNaN(v->second));
        ti = localtime(&v->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 43);
        // last time
        rv = vmap.rbegin();
        TS_ASSERT_EQUALS(rv->second, 8);
        ti = localtime(&rv->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 13);
        TS_ASSERT_EQUALS(ti->tm_min, 13);
        TS_ASSERT_DELTA(timeMean(p2),7.076, 0.001);
    }

    void testLate()
    {
        mkICP();
        mkLate();
        LogParser lp(icp_file.file_string());
        Property* p1 = lp.createLogProperty(log_num_late.file_string(),"late",1);
        Property* p2 = lp.createLogProperty(log_num_late.file_string(),"late",2);
        Property* p3 = lp.createLogProperty(log_num_late.file_string(),"late",4);
        TS_ASSERT(p1);
        TS_ASSERT(p2);
        TS_ASSERT(!p3);
        TimeSeriesProperty<double>* tp1 = dynamic_cast<TimeSeriesProperty<double>*>(p1);
        std::map<TimeSeriesProperty<double>::dateAndTime, double> vmap = tp1->valueAsMap();
        std::map<TimeSeriesProperty<double>::dateAndTime, double>::iterator v= vmap.begin();

        // time 1
        TS_ASSERT_EQUALS(vmap.size(), 9);
        TS_ASSERT_EQUALS(v->second, 2);
        tm* ti = localtime(&v->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        v++;v++;v++;v++;
        // time 5
        TS_ASSERT(isNaN(v->second));
        ti = localtime(&v->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 30);
        // last time
        std::map<TimeSeriesProperty<double>::dateAndTime, double>::reverse_iterator rv = vmap.rbegin();
        TS_ASSERT_EQUALS(rv->second, 8);
        ti = localtime(&rv->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 14);
        TS_ASSERT_EQUALS(ti->tm_min, 3);
        TS_ASSERT_DELTA(timeMean(p1),7.263, 0.001);

    }

    void testEarly()
    {
        mkICP();
        mkEarly();
        LogParser lp(icp_file.file_string());
        Property* p1 = lp.createLogProperty(log_num_early.file_string(),"early",1);
        Property* p2 = lp.createLogProperty(log_num_early.file_string(),"early",2);
        Property* p3 = lp.createLogProperty(log_num_early.file_string(),"early",4);
        TS_ASSERT(p1);
        TS_ASSERT(p2);
        TS_ASSERT(!p3);
        TimeSeriesProperty<double>* tp1 = dynamic_cast<TimeSeriesProperty<double>*>(p1);
        std::map<TimeSeriesProperty<double>::dateAndTime, double> vmap = tp1->valueAsMap();
        std::map<TimeSeriesProperty<double>::dateAndTime, double>::iterator v= vmap.begin();

        // time 1
        TS_ASSERT_EQUALS(vmap.size(), 9);
        TS_ASSERT_EQUALS(v->second, 1);
        tm* ti = localtime(&v->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        v++;v++;v++;v++;
        // time 5
        TS_ASSERT(isNaN(v->second));
        ti = localtime(&v->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 30);
        // last time
        std::map<TimeSeriesProperty<double>::dateAndTime, double>::reverse_iterator rv = vmap.rbegin();
        TS_ASSERT_EQUALS(rv->second, 8);
        ti = localtime(&rv->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 14);
        TS_ASSERT_EQUALS(ti->tm_min, 3);
        TS_ASSERT_DELTA(timeMean(p1),7.194, 0.001);

    }

    void testSingle()
    {
        mkICP();
        mkSingle();
        LogParser lp(icp_file.file_string());
        Property* p1 = lp.createLogProperty(log_num_single.file_string(),"single",1);
        Property* p2 = lp.createLogProperty(log_num_single.file_string(),"single",2);
        Property* p3 = lp.createLogProperty(log_num_single.file_string(),"single",4);
        TS_ASSERT(p1);
        TS_ASSERT(p2);
        TS_ASSERT(!p3);
        TimeSeriesProperty<double>* tp1 = dynamic_cast<TimeSeriesProperty<double>*>(p1);
        std::map<TimeSeriesProperty<double>::dateAndTime, double> vmap = tp1->valueAsMap();
        std::map<TimeSeriesProperty<double>::dateAndTime, double>::iterator v= vmap.begin();

        // time 1
        TS_ASSERT_EQUALS(vmap.size(), 6);
        TS_ASSERT_EQUALS(v->second, 4);
        tm* ti = localtime(&v->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        v++;v++;v++;
        // time 4
        TS_ASSERT(isNaN(v->second));
        ti = localtime(&v->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 57);
        // last time
        std::map<TimeSeriesProperty<double>::dateAndTime, double>::reverse_iterator rv = vmap.rbegin();
        TS_ASSERT_EQUALS(rv->second, 4);
        ti = localtime(&rv->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 14);
        TS_ASSERT_EQUALS(ti->tm_min, 3);
        TS_ASSERT_DELTA(timeMean(p1),4., 0.001);

    }

    void testStr()
    {
        mkICP();
        mkStr();
        LogParser lp(icp_file.file_string());
        Property* p1 = lp.createLogProperty(log_str.file_string(),"str",1);
        Property* p2 = lp.createLogProperty(log_str.file_string(),"str",2);
        Property* p3 = lp.createLogProperty(log_str.file_string(),"str",4);
        TS_ASSERT(p1);
        TS_ASSERT(p2);
        TS_ASSERT(!p3);
        TimeSeriesProperty<std::string>* tp1 = dynamic_cast<TimeSeriesProperty<std::string>*>(p1);
        std::map<TimeSeriesProperty<std::string>::dateAndTime, std::string> vmap = tp1->valueAsMap();
        std::map<TimeSeriesProperty<std::string>::dateAndTime, std::string>::iterator v= vmap.begin();
        // time 1
        TS_ASSERT_EQUALS(vmap.size(), 9);
        TS_ASSERT_EQUALS(v->second, "   line 1");
        tm* ti = localtime(&v->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        v++;v++;v++;
        // time 4
        TS_ASSERT_EQUALS(v->second, "   line 4");
        ti = localtime(&v->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 28);
        // last time
        std::map<TimeSeriesProperty<std::string>::dateAndTime, std::string>::reverse_iterator rv = vmap.rbegin();
        TS_ASSERT_EQUALS(rv->second, "   line 8");
        ti = localtime(&rv->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 14);
        TS_ASSERT_EQUALS(ti->tm_min, 3);
        // assert_throws(timeMean(p1));

        TimeSeriesProperty<std::string>* tp2 = dynamic_cast<TimeSeriesProperty<std::string>*>(p2);
        vmap = tp2->valueAsMap();
        v= vmap.begin();
        // time 1
        TS_ASSERT_EQUALS(vmap.size(), 4);
        TS_ASSERT_EQUALS(v->second, "   line 6");
        ti = localtime(&v->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 37);
        v++;
        // time 2 
        TS_ASSERT_EQUALS(rv->second, "   line 8");
        ti = localtime(&v->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 43);
        // last time
        rv = vmap.rbegin();
        TS_ASSERT_EQUALS(rv->second, "   line 8");
        ti = localtime(&rv->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 13);
        TS_ASSERT_EQUALS(ti->tm_min, 13);
        TS_ASSERT_THROWS(timeMean(p2),std::runtime_error);
    }

    void testNoICPevent()
    {
        if ( exists(icp_file) ) remove(icp_file);
        mkGood();
        LogParser lp(icp_file.file_string());
        Property* p1 = lp.createLogProperty(log_num_good.file_string(),"good",1);
        Property* p2 = lp.createLogProperty(log_num_good.file_string(),"good",2);
        Property* p3 = lp.createLogProperty(log_num_good.file_string(),"good",4);
        TS_ASSERT(p1);
        TS_ASSERT(!p2);
        TS_ASSERT(!p3);
        TimeSeriesProperty<double>* tp1 = dynamic_cast<TimeSeriesProperty<double>*>(p1);
        std::map<TimeSeriesProperty<double>::dateAndTime, double> vmap = tp1->valueAsMap();
        std::map<TimeSeriesProperty<double>::dateAndTime, double>::iterator v= vmap.begin();

        // time 1
        TS_ASSERT_EQUALS(vmap.size(), 9);
        TS_ASSERT_EQUALS(v->second, 1);
        tm* ti = localtime(&v->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 20);
        v++;v++;v++;v++;
        // time 5
        TS_ASSERT(!isNaN(v->second));
        // last time
        std::map<TimeSeriesProperty<double>::dateAndTime, double>::reverse_iterator rv = vmap.rbegin();
        TS_ASSERT_EQUALS(rv->second, 8);
        ti = localtime(&rv->first);
        TS_ASSERT_EQUALS(ti->tm_hour, 14);
        TS_ASSERT_EQUALS(ti->tm_min, 5);
        TS_ASSERT_DELTA(timeMean(p1),7.031, 0.001);

    }
//*/
private:

    void mkICP()
    {
        std::ofstream f( icp_file.file_string().c_str());
        int dt = 0;
        f << to_iso_extended_string(start_time - minutes(5))<<"   START_SE_WAIT"<<'\n';
        f << to_iso_extended_string(start_time + minutes(dt))<<"   BEGIN"<<'\n';dt+=8;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   PAUSE"<<'\n';dt+=4;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   CHANGE PERIOD 2"<<'\n';dt+=3;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   RESUME"<<'\n';dt+=6;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   PAUSE"<<'\n';dt+=4;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   CHANGE PERIOD 1"<<'\n';dt+=2;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   RESUME"<<'\n';dt+=8;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   START_SE_WAIT"<<'\n';dt+=4;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   CHANGE PERIOD 2"<<'\n';dt+=5;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   RESUME"<<'\n';dt+=7;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   ABORT"<<'\n';dt+=3;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   CHANGE PERIOD 1"<<'\n';dt+=5;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   END_SE_WAIT"<<'\n';
        f << to_iso_extended_string(end_time + minutes(0))<<"   END"<<'\n';
        f.close();
    }

    void mkGood()
    {
        std::ofstream f( log_num_good.file_string().c_str());
        int dt = 4;
        f << to_iso_extended_string(start_time - minutes(2))<<"   "<<1<<'\n';
        f << to_iso_extended_string(start_time + minutes(dt))<<"   "<<2<<'\n';dt+=1;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   "<<3<<'\n';dt+=1;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   "<<4<<'\n';dt+=3;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   "<<5<<'\n';dt+=5;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   "<<6<<'\n';dt+=9;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   "<<7<<'\n';dt+=4;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   "<<8<<'\n';
        f << to_iso_extended_string(end_time + minutes(2))<<"   "<<9<<'\n';
        f.close();
    }

    void mkLate()
    {
        std::ofstream f( log_num_late.file_string().c_str());
        int dt = 4;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   "<<2<<'\n';dt+=1;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   "<<3<<'\n';dt+=1;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   "<<4<<'\n';dt+=3;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   "<<5<<'\n';dt+=5;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   "<<6<<'\n';dt+=9;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   "<<7<<'\n';dt+=4;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   "<<8<<'\n';
        f << to_iso_extended_string(end_time + minutes(2))<<"   "<<9<<'\n';
        f.close();
    }

    void mkEarly()
    {
        std::ofstream f( log_num_early.file_string().c_str());
        int dt = 4;
        f << to_iso_extended_string(start_time - minutes(2))<<"   "<<1<<'\n';
        f << to_iso_extended_string(start_time + minutes(dt))<<"   "<<2<<'\n';dt+=1;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   "<<3<<'\n';dt+=1;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   "<<4<<'\n';dt+=3;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   "<<5<<'\n';dt+=5;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   "<<6<<'\n';dt+=9;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   "<<7<<'\n';dt+=4;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   "<<8<<'\n';
        f.close();
    }

    void mkSingle()
    {
        std::ofstream f( log_num_single.file_string().c_str());
        f << to_iso_extended_string(start_time + minutes(18))<<"   "<<4<<'\n';
        f.close();
    }

    void mkStr()
    {
        std::ofstream f( log_str.file_string().c_str());
        int dt = 4;
        f << to_iso_extended_string(start_time - minutes(2))<<"   line "<<1<<'\n';
        f << to_iso_extended_string(start_time + minutes(dt))<<"   line "<<2<<'\n';dt+=1;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   line "<<3<<'\n';dt+=1;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   line "<<4<<'\n';dt+=3;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   line "<<5<<'\n';dt+=5;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   line "<<6<<'\n';dt+=9;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   line "<<7<<'\n';dt+=4;
        f << to_iso_extended_string(start_time + minutes(dt))<<"   line "<<8<<'\n';
        f << to_iso_extended_string(end_time + minutes(2))<<"   line "<<9<<'\n';
        f.close();
    }
//*/
    path log_num_good;// run time interval is within first - last times of the log
    path log_num_late;// first time is later than run start
    path log_num_early;// last time is earlier than run ends
    path log_num_single;// single value
    path log_str;// file of strings
    path icp_file;// icpevent file
    ptime start_time;
    ptime end_time;

};
  
#endif /*LOGPARSERTEST_H_*/
