#ifndef LOGPARSERTEST_H_
#define LOGPARSERTEST_H_

#include <cxxtest/TestSuite.h>

#include <iostream>
#include <fstream>

#include "MantidKernel/LogParser.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "Poco/File.h"

using namespace Mantid::Kernel;

class LogParserTest : public CxxTest::TestSuite
{
public: 
  
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
        LogParser lp(icp_file.path());
        Property* p1 = lp.createLogProperty(log_num_good.path(),"good");
        TS_ASSERT(p1);
        TimeSeriesProperty<double>* tp1 = dynamic_cast<TimeSeriesProperty<double>*>(p1);
        std::map<dateAndTime, double> vmap = tp1->valueAsMap();
        std::map<dateAndTime, double>::iterator v= vmap.begin();
        // time 1
        TS_ASSERT_EQUALS(vmap.size(), 9);
        TS_ASSERT_EQUALS(v->second, 1);
        ti_data = boost::posix_time::to_tm( v->first ); ti = &ti_data;

        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        v++;v++;v++;v++;
        // time 5
        //TS_ASSERT(isNaN(v->second));
        ti_data = boost::posix_time::to_tm( v->first ); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        // last time
        std::map<dateAndTime, double>::reverse_iterator rv = vmap.rbegin();
        TS_ASSERT_EQUALS(rv->second, 9);
        ti_data = boost::posix_time::to_tm( rv->first ); ti = &ti_data;
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
        LogParser lp(icp_file.path());
        Property* p1 = lp.createLogProperty(log_num_late.path(),"late");
        TS_ASSERT(p1);
        TimeSeriesProperty<double>* tp1 = dynamic_cast<TimeSeriesProperty<double>*>(p1);
        std::map<dateAndTime, double> vmap = tp1->valueAsMap();
        std::map<dateAndTime, double>::iterator v= vmap.begin();

        // time 1
        TS_ASSERT_EQUALS(vmap.size(), 8);
        TS_ASSERT_EQUALS(v->second, 2);

        ti_data = boost::posix_time::to_tm( v->first ); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        v++;v++;v++;v++;
        // time 5
        //TS_ASSERT(isNaN(v->second));
        ti_data = boost::posix_time::to_tm( v->first ); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        // last time
        std::map<dateAndTime, double>::reverse_iterator rv = vmap.rbegin();
        TS_ASSERT_EQUALS(rv->second, 9);
        ti_data = boost::posix_time::to_tm( rv->first ); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 14);
        TS_ASSERT_EQUALS(ti->tm_min, 3);
        TS_ASSERT_DELTA(timeMean(p1),8.4941, 0.001);
        
        delete p1;
    }

    void testEarly()
    {
        mkICP();
        mkEarly();
        LogParser lp(icp_file.path());
        Property* p1 = lp.createLogProperty(log_num_early.path(),"early");
        TS_ASSERT(p1);
        TimeSeriesProperty<double>* tp1 = dynamic_cast<TimeSeriesProperty<double>*>(p1);
        std::map<dateAndTime, double> vmap = tp1->valueAsMap();
        std::map<dateAndTime, double>::iterator v= vmap.begin();

        // time 1
        TS_ASSERT_EQUALS(vmap.size(), 8);
        TS_ASSERT_EQUALS(v->second, 1);
        ti_data = boost::posix_time::to_tm( v->first ); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        v++;v++;v++;v++;
        // time 5
        //TS_ASSERT(isNaN(v->second));
        ti_data = boost::posix_time::to_tm( v->first ); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        // last time
        std::map<dateAndTime, double>::reverse_iterator rv = vmap.rbegin();
        TS_ASSERT_EQUALS(rv->second, 8);
        ti_data = boost::posix_time::to_tm( rv->first ); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 23);
        TS_ASSERT_DELTA(timeMean(p1),4.9090, 0.001);

        delete p1;
    }

    void testSingle()
    {
        mkICP();
        mkSingle();
        LogParser lp(icp_file.path());
        Property* p1 = lp.createLogProperty(log_num_single.path(),"single");
        TS_ASSERT(p1);
        TimeSeriesProperty<double>* tp1 = dynamic_cast<TimeSeriesProperty<double>*>(p1);
        std::map<dateAndTime, double> vmap = tp1->valueAsMap();
        std::map<dateAndTime, double>::iterator v= vmap.begin();

        // time 1
        TS_ASSERT_EQUALS(vmap.size(), 1);
        TS_ASSERT_EQUALS(v->second, 4);
        ti_data = boost::posix_time::to_tm( v->first ); ti = &ti_data;
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
        LogParser lp(icp_file.path());
        Property* p1 = lp.createLogProperty(log_str.path(),"str");
        TS_ASSERT(p1);
        TimeSeriesProperty<std::string>* tp1 = dynamic_cast<TimeSeriesProperty<std::string>*>(p1);
        std::map<dateAndTime, std::string> vmap = tp1->valueAsMap();
        std::map<dateAndTime, std::string>::iterator v= vmap.begin();
        // time 1
        TS_ASSERT_EQUALS(vmap.size(), 9);
        TS_ASSERT_EQUALS(v->second, "   line 1");
        ti_data = boost::posix_time::to_tm( v->first ); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        v++;v++;v++;
        // time 4
        TS_ASSERT_EQUALS(v->second, "   line 4");
        ti_data = boost::posix_time::to_tm( v->first ); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        // last time
        std::map<dateAndTime, std::string>::reverse_iterator rv = vmap.rbegin();
        TS_ASSERT_EQUALS(rv->second, "   line 9");
        ti_data = boost::posix_time::to_tm( rv->first ); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 14);
        TS_ASSERT_EQUALS(ti->tm_min, 3);
        // assert_throws(timeMean(p1));
        delete p1;
    }

    void testNoICPevent()
    {
      if ( icp_file.exists() ) icp_file.remove();
        mkGood();
        LogParser lp(icp_file.path());
        Property* p1 = lp.createLogProperty(log_num_good.path(),"good");
        TS_ASSERT(p1);
        TimeSeriesProperty<double>* tp1 = dynamic_cast<TimeSeriesProperty<double>*>(p1);
        std::map<dateAndTime, double> vmap = tp1->valueAsMap();
        std::map<dateAndTime, double>::iterator v= vmap.begin();

        // time 1
        TS_ASSERT_EQUALS(vmap.size(), 9);
        TS_ASSERT_EQUALS(v->second, 1);
        ti_data = boost::posix_time::to_tm( v->first ); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 12);
        TS_ASSERT_EQUALS(ti->tm_min, 22);
        v++;v++;v++;v++;
        // time 5
        //TS_ASSERT(!isNaN(v->second));
        // last time
        std::map<dateAndTime, double>::reverse_iterator rv = vmap.rbegin();
        TS_ASSERT_EQUALS(rv->second, 9);
        ti_data = boost::posix_time::to_tm( rv->first ); ti = &ti_data;
        TS_ASSERT_EQUALS(ti->tm_hour, 14);
        TS_ASSERT_EQUALS(ti->tm_min, 3);
        TS_ASSERT_DELTA(timeMean(p1),8.4904, 0.001);

        delete p1;
    }



    //----------------------------------------------------------------------------
    void test_timeMean()
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

      TS_ASSERT_DELTA(timeMean(log), 6.0, 1e-3);
    }

    void test_timeMean_one_Value()
    {
      TimeSeriesProperty<double> * log  = new TimeSeriesProperty<double>("MydoubleLog");
      TS_ASSERT( log->addValue("2007-11-30T16:17:00",56) );
      TS_ASSERT_EQUALS( log->realSize(), 1);

      TS_ASSERT_DELTA(timeMean(log), 56.0, 1e-3);
    }


//*/
private:

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
