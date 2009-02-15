//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LogParser.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <fstream>  // used to get ifstream
#include <sstream>
#include <algorithm>
#include <limits>

namespace Mantid
{
namespace DataHandling
{

//using namespace Kernel;
//using API::WorkspaceProperty;
//using API::MatrixWorkspace;
//using API::MatrixWorkspace_sptr;
//using DataObjects::Workspace2D;
//using DataObjects::Workspace2D_sptr;

Kernel::Logger& LogParser::g_log = Mantid::Kernel::Logger::get("LogParser");

enum commands {NONE = 0,BEGIN,END,CHANGE_PERIOD};

LogParser::LogParser(const std::string& eventFName)
:m_nOfPeriods(1),m_unknown(true)
{
    std::ifstream file(eventFName.c_str());
    if (!file)
    {
        g_log.warning()<<"Cannot open ICPevent file "<<eventFName<<". Period 1 assumed for all data.\n";
        return;
    }

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

    std::string str;
    ptime last_time(time_from_string("1900-01-01 00:00:01"));
    time_period last_run(time_from_string("1900-01-01 00:00:00"),last_time);
    int cur_period = 1;
    m_nOfPeriods = 1;
    bool running = false;
    ptime start_time;
    while(std::getline(file,str))
    {
        std::string stime,sdata;
        stime = str.substr(0,19);
        sdata = str.substr(19);
        stime[10] = ' ';// make it parsable by boost::...time_from_string(...) function
        ptime t(time_from_string(stime));
        if (t < last_time)
        {
            g_log.error("Broken time sequence in event file "+eventFName);
            throw std::runtime_error("Broken time sequence in event file "+eventFName);
        }
        std::string scom;
        std::istringstream idata(sdata);
        idata >> scom;
        commands com = command_map[scom];
        if (com == CHANGE_PERIOD)
        {
            if (start_time.is_not_a_date_time()) start_time = t;
            int ip = -1;
            std::string s;
            idata >> s >> ip;
            if (ip > 0 && s == "PERIOD")
            {
                cur_period = ip;
                if (ip > m_nOfPeriods) m_nOfPeriods = ip;
            }
            /*else
            {
                g_log.error("Error in parsing event file "+eventFName+" line: "+str);
                throw std::runtime_error("Error in parsing ivent file "+eventFName);
            }*/
        }
        else if (com == BEGIN)
        {
            if (running)
            {
                g_log.error("Error in parsing event file "+eventFName+" line: "+str);
                throw std::runtime_error("Error in parsing ivent file "+eventFName);
            }
            running = true;
            start_time = t;
        }
        else if (com == END)
        {
            if (start_time.is_not_a_date_time()) continue;
            running = false;
            if (start_time == last_run.end())
            {
                if (m_periods[last_run] == cur_period)
                {// expand last run
                    m_periods.erase( m_periods.find(last_run) );
                    time_period new_run(last_run.begin(),t);
                    m_periods[new_run] = cur_period;
                    last_run = new_run;
                    g_log.warning()<<"Last run interval has been expanded to "<<t<<'\n';
                }
                else
                {
                    if (start_time == t)
                    {
                        g_log.warning()<<"Running time interval is considered null "<<t<<"\n";
                    }
                    else
                    {
                        time_period this_run(start_time,t);
                        m_periods[this_run] = cur_period;
                        last_run = this_run;
                    }
                }
            }
            else
            {
                if (start_time == t)
                {
                    // shift the start time slightly so that the interval is non-zero.
                    start_time -= milliseconds(1);
                }
                time_period this_run(start_time,t);
                m_periods[this_run] = cur_period;
                last_run = this_run;
            }
        }
        last_time = t;
    };
    if (running) 
    {
        time_period this_run(start_time,last_time);
        m_periods[this_run] = cur_period;
    }

    if (m_periods.size() > 0) m_unknown = false;

}

//// Class used internally 
template<class T>
struct time_period_contains
{
    ptime tim;
    time_period_contains(ptime t):tim(t){}
    bool operator()(const std::pair<const time_period, T >& p)const{return p.first.contains(tim);}
};

/**
    Time interval contains a time tim if   begin <= tim < end.
*/
int LogParser::period(ptime tim)const
{
    // Event file was not found, assume the instrument was constantly running and period is always 1.
    if (m_periods.size() == 0) return 1;

    std::map<time_period,int>::const_iterator p = std::find_if(m_periods.begin(),m_periods.end(),time_period_contains<int>(tim));
    if (p == m_periods.end()) return 0;

    return p->second;

}

std::vector<time_period> LogParser::getTimes(int p)const
{
    std::vector<time_period> res;
    std::map<time_period,int>::const_iterator it = m_periods.begin();
    for(;it!=m_periods.end();it++)
        if (it->second == p) res.push_back(it->first);
    return res;
}

Kernel::Property* LogParser::createLogProperty(const std::string& logFName, const std::string& name, int period)
{
    std::ifstream file(logFName.c_str());
    if (!file)
    {
        g_log.warning()<<"Cannot open log file "<<logFName<<"\n";
        return 0;
    }

    // Change times and new values read from file
    std::map<ptime,std::string> change_times;

    std::string str,old_data;
    bool isNumeric(false);
    while(std::getline(file,str))
    {
        std::string stime,sdata;
        stime = str.substr(0,19);
        sdata = str.substr(19);
        if (sdata == old_data) continue;// looking for a change in the data
        stime[10] = ' ';// make it parsable by boost::...time_from_string(...) function
        change_times[ptime(time_from_string(stime))] = sdata;
        std::istringstream istr(sdata);
        double tmp;
        istr >> tmp;
        isNumeric = !istr.fail();
        old_data = sdata;
    }

    if (change_times.size() == 0) return 0;

    Kernel::Property* logv = 0;

    if (isNumeric)
        logv = new Kernel::TimeSeriesProperty<double>(name);
    else
        logv = new Kernel::TimeSeriesProperty<std::string>(name);

    ptime start_time;
    ptime end_time;

    std::vector<time_period> run_intervals = getTimes(period);
    if (run_intervals.size() > 0)
    {

        start_time = run_intervals.front().begin();
        end_time = run_intervals.back().end();

        // Make sure that when instrument starts running the parameter has a value
        if (change_times.begin()->first > start_time)
            change_times[start_time] = change_times.begin()->second;
        else if (m_unknown && change_times.begin()->first < start_time)
        {// expand unknown running times if necessary
            start_time = change_times.begin()->first;
            run_intervals[0] = time_period(start_time,end_time);
        }

        // Make sure the life span of the parameter covers the whole running time
        if (change_times.rbegin()->first < end_time)
            change_times[end_time] = change_times.rbegin()->second;
        else if (m_unknown && change_times.rbegin()->first > end_time)
        {// expand unknown running times if necessary
            end_time = change_times.rbegin()->first;
            run_intervals[0] = time_period(start_time,end_time);
        }
    }
    else // run_intervals.size() == 0
    {
        if (period > 1) return 0;
        if (change_times.size() == 1)
        {
            start_time = change_times.begin()->first - seconds(1);
            end_time = change_times.begin()->first;
            change_times[start_time] = change_times[end_time];
            run_intervals.push_back(time_period(start_time,end_time));
        }
        else
        {
            start_time = change_times.begin()->first;
            end_time = change_times.rbegin()->first;
            run_intervals.push_back(time_period(start_time,end_time));
        }
    }

    // Construct a map of time intervals when the parameter is constant
    std::vector<time_period> change_intervals;
    std::map<ptime,std::string>::iterator it = change_times.begin();
    std::map<ptime,std::string>::iterator it1 = change_times.begin();
    it1++;
    for(;it1!=change_times.end();it++,it1++)
    {
        change_intervals.push_back(time_period(it->first,it1->first));
    }

    double lastv = 0;
    std::string lasts;
    time_period prev = time_period(ptime(),ptime());
    // Find intersections of running time intervals and intervals of constant values
    for(std::vector<time_period>::iterator r = run_intervals.begin();r!=run_intervals.end();r++)
    for(std::vector<time_period>::iterator c = change_intervals.begin();c!=change_intervals.end();c++)
    {
        if (c->begin() > r->end()) break;
        time_period inter = r->intersection(*c);
        if (!inter.is_null())
        {
            std::tm tinfo = to_tm(inter.begin());
            std::time_t t1 = std::mktime(&tinfo);
            std::time_t t2(0);
            bool mkEmpty = ! prev.end().is_not_a_date_time() && inter.begin() != prev.end();
            if (mkEmpty)
            {
                tinfo = to_tm(prev.end());
                t2 = std::mktime(&tinfo);
            }
            if (isNumeric)
            {
                std::istringstream istr(change_times[c->begin()]);
                double tmp;
                istr >> tmp;
                Kernel::TimeSeriesProperty<double>* p = static_cast<Kernel::TimeSeriesProperty<double>*>(logv);
                if (mkEmpty) p->addValue(t2,std::numeric_limits<double>::quiet_NaN());
                p->addValue(t1,tmp);
                lastv = tmp;
            }
            else
            {
                lasts = change_times[c->begin()];
                if (mkEmpty)
                    static_cast<Kernel::TimeSeriesProperty<std::string>*>(logv)->addValue(t2, lasts);
                static_cast<Kernel::TimeSeriesProperty<std::string>*>(logv)->addValue(t1, lasts);
            }
            prev = inter;
            //std::cerr<<"--"<<*r<<' '<<*c<<'\n'; 
            //std::cerr<<"Intersection: "<<inter<<' '<<c->begin()<<'\n';
        }

    }

    // Insert the last value
    std::tm tinfo = to_tm(prev.end());
    std::time_t t = std::mktime(&tinfo);
    if (isNumeric)
    {
        Kernel::TimeSeriesProperty<double>* p = static_cast<Kernel::TimeSeriesProperty<double>*>(logv);
        p->addValue(t,lastv);
    }
    else
    {
        Kernel::TimeSeriesProperty<std::string>* p = static_cast<Kernel::TimeSeriesProperty<std::string>*>(logv);
        p->addValue(t,lasts);
    }

    return logv;
}

double timeMean(const Kernel::Property* p)
{
    const Kernel::TimeSeriesProperty<double>* dp = dynamic_cast<const Kernel::TimeSeriesProperty<double>*>(p);
    if (!dp)
    {
        throw std::runtime_error("Property of a wromg type.");
    }
    
    std::map<std::time_t, double> dpmap = dp->valueAsMap();
    double res = 0.;

    std::map<std::time_t, double>::const_iterator it0=dpmap.begin();
    std::map<std::time_t, double>::const_iterator it=dpmap.begin();
    it++;
    bool skip = false;
    long total = 0;
    for(;it!=dpmap.end();it++,it0++)
    {
        if (!skip)
        {
            std::time_t dt = it->first - it0->first;
            total += dt;
            res += it0->second * dt;
        }
        if (isNaN(it->second))
        {
            skip = true;
        }
        else
            skip = false;
    }

    if (total > 0) res /= total;

    return res;
}

double firstValue(const Kernel::Property* p)
{
    const Kernel::TimeSeriesProperty<double>* dp = dynamic_cast<const Kernel::TimeSeriesProperty<double>*>(p);
    if (!dp)
    {
        throw std::runtime_error("TimeSeriesProperty of a wromg type.");
    }

    std::map<std::time_t, double> dpmap = dp->valueAsMap();

    if (dpmap.empty())
        throw std::runtime_error("TimeSeriesProperty is empty");

    return dpmap.begin()->second;
    
}

double secondValue(const Kernel::Property* p)
{
    const Kernel::TimeSeriesProperty<double>* dp = dynamic_cast<const Kernel::TimeSeriesProperty<double>*>(p);
    if (!dp)
    {
        throw std::runtime_error("TimeSeriesProperty of a wromg type.");
    }

    std::map<std::time_t, double> dpmap = dp->valueAsMap();

    if (dpmap.size() < 2)
        throw std::runtime_error("TimeSeriesProperty is empty or single valued");

    std::map<std::time_t, double>::iterator it = dpmap.begin();
    it++;
    return it->second;
}

double lastValue(const Kernel::Property* p)
{
    const Kernel::TimeSeriesProperty<double>* dp = dynamic_cast<const Kernel::TimeSeriesProperty<double>*>(p);
    if (!dp)
    {
        throw std::runtime_error("TimeSeriesProperty of a wromg type.");
    }

    std::map<std::time_t, double> dpmap = dp->valueAsMap();

    if (dpmap.empty())
        throw std::runtime_error("TimeSeriesProperty is empty");

    return dpmap.rbegin()->second;
}

double nthValue(const Kernel::Property* p, int i)
{
    const Kernel::TimeSeriesProperty<double>* dp = dynamic_cast<const Kernel::TimeSeriesProperty<double>*>(p);
    if (!dp)
    {
        throw std::runtime_error("TimeSeriesProperty of a wromg type.");
    }

    std::map<std::time_t, double> dpmap = dp->valueAsMap();
    if (dpmap.empty())
        throw std::runtime_error("TimeSeriesProperty is empty");

    std::map<std::time_t, double>::iterator it = dpmap.begin();
    for(int j=0;it!=dpmap.end();it++)
        if (!isNaN(it->second))
        {
            if (j == i) return it->second;
            j++;
        }

    return dpmap.rbegin()->second;
}


} // namespace DataHandling
} // namespace Mantid


