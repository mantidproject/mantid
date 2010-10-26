//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/LogParser.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <fstream>  // used to get ifstream
#include <sstream>
#include <algorithm>
#include <limits>

namespace Mantid
{
  namespace Kernel
  {

    Kernel::Logger& LogParser::g_log = Mantid::Kernel::Logger::get("LogParser");

    enum commands {NONE = 0,BEGIN,END,CHANGE_PERIOD};

    /**
     * Destructor
     */
    LogParser::~LogParser()
    {
    }

    /** 
    * Constructor.
    * @param eventFName ICPevent file name.
    */
    LogParser::LogParser(const std::string& eventFName)
      :m_nOfPeriods(1)
    {
      Kernel::TimeSeriesProperty<int>* periods = new Kernel::TimeSeriesProperty<int> ("periods");
      Kernel::TimeSeriesProperty<bool>* status = new Kernel::TimeSeriesProperty<bool> ("running");
      m_periods.reset( periods );
      m_status.reset( status );

      std::ifstream file(eventFName.c_str());
      if (!file)
      {
        periods->addValue(Kernel::dateAndTime() + Kernel::DateAndTime::oneSecond, 1);
        status->addValue(Kernel::dateAndTime() + Kernel::DateAndTime::oneSecond,true);
        g_log.warning()<<"Cannot open ICPevent file "<<eventFName<<". Period 1 assumed for all data.\n";
        return;
      }

      // Command map. BEGIN means start recording, END is stop recording, CHANGE_PERIOD - the period changed
      std::map<std::string,commands> command_map;
      command_map["BEGIN"] = BEGIN;
      command_map["RESUME"] = BEGIN;
      command_map["END_SE_WAIT"] = BEGIN;
      command_map["PAUSE"] = END;
      command_map["END"] = END;
      command_map["ABORT"] = END;
      command_map["UPDATE"] = END;
      command_map["START_SE_WAIT"] = END;
      command_map["CHANGE"] = CHANGE_PERIOD;

      std::string str,start_time;
      m_nOfPeriods = 1;

      while(Mantid::Kernel::extractToEOL(file,str))
      {
        std::string stime,sdata;
        stime = str.substr(0,19);
        sdata = str.substr(19);
        if (start_time.empty()) start_time = stime;

        std::string scom;
        std::istringstream idata(sdata);
        idata >> scom;
        commands com = command_map[scom];
        if (com == CHANGE_PERIOD)
        {
          int ip = -1;
          std::string s;
          idata >> s >> ip;
          if (ip > 0 && s == "PERIOD")
          {
            if (ip > m_nOfPeriods) m_nOfPeriods = ip;
            periods->addValue(stime,ip);
          }
        }
        else if (com == BEGIN)
        {
          status->addValue(stime,true);
        }
        else if (com == END)
        {
          status->addValue(stime,false);
        }
      };

      if (periods->size() == 0) periods->addValue(start_time,1);
      if (status->size() == 0) status->addValue(start_time,true);

    }

    /** Create given the icpevent log property.
    *  @param log A pointer to the property
    */
    LogParser::LogParser(const Kernel::Property* log)
      :m_nOfPeriods(1)
    {
      Kernel::TimeSeriesProperty<int>* periods = new Kernel::TimeSeriesProperty<int> ("periods");
      Kernel::TimeSeriesProperty<bool>* status = new Kernel::TimeSeriesProperty<bool> ("running");
      m_periods.reset( periods );
      m_status.reset( status );

      const Kernel::TimeSeriesProperty<std::string>* icpLog = dynamic_cast<const Kernel::TimeSeriesProperty<std::string>*>(log);
      if (!icpLog || icpLog->size() == 0)
      {
        periods->addValue(Kernel::dateAndTime(),1);
        status->addValue(Kernel::dateAndTime(),true);
        g_log.warning()<<"Cannot process ICPevent log. Period 1 assumed for all data.\n";
        return;
      }

      /// Command map. BEGIN means start recording, END is stop recording, CHANGE_PERIOD - the period changed
      std::map<std::string,commands> command_map;
      command_map["BEGIN"] = BEGIN;
      command_map["RESUME"] = BEGIN;
      command_map["END_SE_WAIT"] = BEGIN;
      command_map["PAUSE"] = END;
      command_map["END"] = END;
      command_map["ABORT"] = END;
      command_map["UPDATE"] = END;
      command_map["START_SE_WAIT"] = END;
      command_map["CHANGE"] = CHANGE_PERIOD;

      m_nOfPeriods = 1;

      std::map<Kernel::dateAndTime, std::string> logm = icpLog->valueAsMap();
      std::map<Kernel::dateAndTime, std::string>::const_iterator it = logm.begin();

      for(;it!=logm.end();it++)
      {
        std::string scom;
        std::istringstream idata(it->second);
        idata >> scom;
        commands com = command_map[scom];
        if (com == CHANGE_PERIOD)
        {
          int ip = -1;
          std::string s;
          idata >> s >> ip;
          if (ip > 0 && s == "PERIOD")
          {
            if (ip > m_nOfPeriods) m_nOfPeriods = ip;
            periods->addValue(it->first,ip);
          }
        }
        else if (com == BEGIN)
        {
          status->addValue(it->first,true);
        }
        else if (com == END)
        {
          status->addValue(it->first,false);
        }
      };

      if (periods->size() == 0) periods->addValue(icpLog->firstTime(),1);
      if (status->size() == 0) status->addValue(icpLog->firstTime(),true);
    }


    /**  Reads in log data from a log file and stores them in a TimeSeriesProperty.
    @param logFName The name of the log file
    @param name The name of the property
    @return A pointer to the created property.
    */
    Kernel::Property* LogParser::createLogProperty(const std::string& logFName, const std::string& name)const
    {
      std::ifstream file(logFName.c_str());
      if (!file)
      {
        g_log.warning()<<"Cannot open log file "<<logFName<<"\n";
        return 0;
      }

      // Change times and new values read from file
      std::map<std::string,std::string> change_times;

      // Read in the data and determin if it is numeric
      std::string str,old_data;
      bool isNumeric(false);
      std::string stime,sdata;
      // MG 22/09/09: If the log file was written on a Windows machine and then read on a Linux machine, std::getline will
      // leave CR at the end of the string and this causes problems when reading out the log values. Mantid::extractTOEOL
      // extracts all EOL characters
      while(Mantid::Kernel::extractToEOL(file,str))
      {
        if( str.empty() || str[0]=='#') {continue;}

        if (!Kernel::TimeSeriesProperty<double>::isTimeString(str)) 
        {
          //if the line doesn't start with a time treat it as a continuation of the previous data
          if (change_times.size() == 0 || isNumeric)
          {// if there are no previous data
            std::string mess = "Cannot parse log file "+logFName+". Line:"+str;
            g_log.error(mess);
            throw std::logic_error(mess);
          }
          change_times[stime] += std::string(" ") + str;
          continue;
        }
        stime = str.substr(0,19);
        sdata = str.substr(19);

        if (sdata == old_data) continue;// looking for a change in the data

        //check if the data is numeric
        std::istringstream istr(sdata);
        double tmp;
        istr >> tmp;
        isNumeric = !istr.fail();
        old_data = sdata;

        //if time is repeated and the data aren't numeric append the new string to the old one
        if (!isNumeric && change_times[stime].size() > 0)
          change_times[stime] += std::string(" ") + sdata;
        else
          change_times[stime] = sdata;
      }

      if (change_times.size() == 0) return 0;

      if (isNumeric)
      {
        Kernel::TimeSeriesProperty<double>* logv = new Kernel::TimeSeriesProperty<double>(name);
        std::map<std::string,std::string>::iterator it = change_times.begin();
        for(;it!=change_times.end();it++)
        {
          std::istringstream istr(it->second);
          double d;
          istr >> d;
          logv->addValue(it->first,d);
        }
        return logv;
      }
      else
      {
        Kernel::TimeSeriesProperty<std::string>* logv = new Kernel::TimeSeriesProperty<std::string>(name);
        std::map<std::string,std::string>::iterator it = change_times.begin();
        for(;it!=change_times.end();it++)
        {
          logv->addValue(it->first,it->second);
        }
        return logv;
      }


      return 0;
    }

    /** Ctreates a TimeSeriesProperty<bool> showing times when a particular period was active.
    *  @param period The data period
    */
    Kernel::Property* LogParser::createPeriodLog(int period)const
    {
      Kernel::TimeSeriesProperty<int>* periods = dynamic_cast< Kernel::TimeSeriesProperty<int>* >(m_periods.get());
      std::ostringstream ostr;
      ostr<<period;
      Kernel::TimeSeriesProperty<bool>* p = new Kernel::TimeSeriesProperty<bool> ("period "+ostr.str());
      std::map<Kernel::dateAndTime, int> pMap = periods->valueAsMap();
      std::map<Kernel::dateAndTime, int>::const_iterator it = pMap.begin();
      if (it->second != period)
        p->addValue(it->first,false);
      for(;it!=pMap.end();it++)
        p->addValue(it->first, (it->second == period) );

      return p;
    }

    /// Ctreates a TimeSeriesProperty<int> with all data periods
    Kernel::Property* LogParser::createAllPeriodsLog()const
    {
      Kernel::TimeSeriesProperty<int>* p = new Kernel::TimeSeriesProperty<int> ("periods");
      Kernel::TimeSeriesProperty<int>* periods = dynamic_cast< Kernel::TimeSeriesProperty<int>* >(m_periods.get());
      std::map<Kernel::dateAndTime, int> pMap = periods->valueAsMap();
      std::map<Kernel::dateAndTime, int>::const_iterator it = pMap.begin();
      for(;it!=pMap.end();it++)
        p->addValue(it->first, it->second);
      return p;
    }

    /// Ctreates a TimeSeriesProperty<bool> with running status
    Kernel::Property* LogParser::createRunningLog()const
    {
      return dynamic_cast<Kernel::TimeSeriesProperty<bool>*>(m_status.get())->clone();
    }


    /** Returns the mean value if the property is TimeSeriesProperty<double>.
     *
      @param p Property with the data. Will throw if not TimeSeriesProperty<double>.
      @return The mean value over time.
      @throw runtime_error if the property is not TimeSeriesProperty<double>
    */
    double timeMean(const Kernel::Property* p)
    {
      const Kernel::TimeSeriesProperty<double>* dp = dynamic_cast<const Kernel::TimeSeriesProperty<double>*>(p);
      if (!dp)
      {
        throw std::runtime_error("Property of a wrong type.");
      }

      TimeSeriesPropertyStatistics stats = getTimeSeriesPropertyStatistics(dp);

      return stats.mean;
    }

    /**
    * Extract a string until an EOL character is reached. There are 3 scenarios that we need to deal with
    * 1) Windows-style  - CRLF ('\\r\\n');
    * 2) Unix-style     - LF ('\\n');
    * 3) Old MAC style  - CR ('\\r').
    * This function will give the string preceding any of these sequences
    * @param is The input stream to read from
    * @param str The output string to use to accumulate the line
    * @returns A reference to the input stream
    */
    std::istream& extractToEOL(std::istream& is, std::string& str)
    {
      // Empty the string
      str = "";
      char c('\0');
      while( is.get(c) )
      {
        if( c == '\r' )
        {
          c = is.peek();
          if( c == '\n' )
          {
            //Extract this as well
            is.get();
          }
          break;
        }
        else if( c == '\n')
        {
          break;
        }
        else 
        {
          //Accumulate the string
          str += c;
        }
      }
      return is;
    }

  } // namespace Geometry
} // namespace Mantid


