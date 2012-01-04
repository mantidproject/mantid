#include "MantidAlgorithms/GetTimeSeriesLogInformation.h"
#include "MantidKernel/System.h"

#include "MantidAlgorithms/FilterEventsHighFrequency.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/IEventList.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/Events.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <algorithm>
#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;


namespace Mantid
{
namespace Algorithms
{

  DECLARE_ALGORITHM(GetTimeSeriesLogInformation)
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  GetTimeSeriesLogInformation::GetTimeSeriesLogInformation()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  GetTimeSeriesLogInformation::~GetTimeSeriesLogInformation()
  {
  }
  
  void GetTimeSeriesLogInformation::initDocs(){

  }

  /*
   * Definition of all input arguments
   */
  void GetTimeSeriesLogInformation::init()
  {
    this->declareProperty(new API::WorkspaceProperty<DataObjects::EventWorkspace>("InputEventWorkspace", "", Direction::InOut),
        "Input EventWorkspace.  Each spectrum corresponds to 1 pixel");
    bool optional = true;
    this->declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("SampleEnvironmentWorkspace", "", Direction::Input, optional),
        "Input 2D workspace storing sample environment data along with absolute time");
    this->declareProperty("LogName", "", "Log's name to filter events.");

    this->declareProperty("T0", 0.0, "Earliest time of the events to be selected.  It can be absolute time (ns), relative time (second) or percentage.");
    this->declareProperty("Tf", 100.0, "Latest time of the events to be selected.  It can be absolute time (ns), relative time (second) or percentage.");
    this->declareProperty(new API::FileProperty("OutputDirectory", "", API::FileProperty::OptionalDirectory),
        "Directory of all output files");

    std::vector<std::string> timeoptions;
    timeoptions.push_back("Absolute Time (nano second)");
    timeoptions.push_back("Relative Time (second)");
    timeoptions.push_back("Percentage");
    this->declareProperty("TimeRangeOption", "Relative Time (second)", new ListValidator(timeoptions),
        "User defined time range (T0, Tf) is of absolute time (second). ");

    this->declareProperty("ExamineLog", false, "Examine the log.");

    bool optionalopws = true;
    this->declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("TimeOutputWorkspace", "TimeStat", Direction::Output, optionalopws),
        "Output Workspace as the statistic on time (second).");
    this->declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("PercentOutputWorkspace", "PercentStat", Direction::Output, optionalopws),
        "Output Workspace as the statistic on percentage time.");

    this->declareProperty("Resolution", 10, "Resolution of statistic workspace");

    return;
  }

  void GetTimeSeriesLogInformation::exec()
  {
    // 1. Get property
    eventWS = this->getProperty("InputEventWorkspace");
    const std::string outputdir = this->getProperty("OutputDirectory");
    seWS = this->getProperty("SampleEnvironmentWorkspace");

    std::string logname = this->getProperty("LogName");

    double t0r = this->getProperty("T0");
    double tfr = this->getProperty("Tf");
    if (t0r >= tfr){
      g_log.error() << "User defined filter starting time (T0 = " << t0r << ") is later than ending time (Tf = " << tfr << ")" << std::endl;
      throw std::invalid_argument("User input T0 and Tf error!");
    }
    std::string timeoption = this->getProperty("TimeRangeOption");

    bool examlog = this->getProperty("ExamineLog");

    // 2) Set up data structures
    const API::Run& runlog = eventWS->run();
    std::string runstartstr = runlog.getProperty("run_start")->value();
    Kernel::DateAndTime runstart(runstartstr);
    mRunStartTime = runstart;

    std::vector<Kernel::DateAndTime> times;

    if (logname.size() > 0)
    {
      // Log
      Kernel::TimeSeriesProperty<double>* tlog = dynamic_cast<Kernel::TimeSeriesProperty<double>* >(
          eventWS->run().getProperty(logname));
      if (!tlog){
        g_log.error() << "TimeSeriesProperty Log " << logname << " does not exist in workspace " <<
            eventWS->getName() << std::endl;
        throw std::invalid_argument("TimeSeriesProperty log cannot be found");
      }
      times = tlog->timesAsVector();
    }
    else
    {
      // Workspace
      for (size_t i=0; i<seWS->dataX(0).size(); i ++){
        Kernel::DateAndTime tt(static_cast<int64_t>(seWS->dataX(0)[i]));
        times.push_back(tt);
      }
    }

    // 3) Print out some information about log and run
    std::stringstream loginfoss;
    int64_t dt0_ns = times[0].total_nanoseconds()-runstart.total_nanoseconds();
    int64_t dtf_ns = times[times.size()-1].total_nanoseconds()-runstart.total_nanoseconds();
    loginfoss << "Run Start Time = " << runstart << std::endl <<
        "                 " << runstart.total_nanoseconds() << std::endl;
    loginfoss << "Log Start Time = " << times[0] << std::endl <<
        "                 " << times[0].total_nanoseconds() << "nano-second = "<<
        (static_cast<double>(times[0].total_nanoseconds())*1.0E-9) << " seconds " << std::endl <<
        "  To Run Start dT = " << dt0_ns << " ns" << std::endl <<
        "                    " << static_cast<double>(dt0_ns)/static_cast<double>(dtf_ns) << std::endl;
    loginfoss << "Log End   Time = " << times[times.size()-1] << std::endl <<
        "                 " << times[times.size()-1].total_nanoseconds() << " nano-second  =  " <<
        (static_cast<double>(times[times.size()-1].total_nanoseconds())*1.0E-9) << " seconds " << std::endl <<
        "  To Run Start dT = " << dtf_ns << " ns" << std::endl <<
        "                    " << static_cast<double>(dtf_ns)/static_cast<double>(dtf_ns)*100 << " percent" << std::endl;;

    // 4) Determine Time
    if (timeoption.compare("Absolute Time (nano second)")==0){
      // i. absolute time
      mFilterT0 = Kernel::DateAndTime(static_cast<int64_t>(t0r));
      mFilterTf = Kernel::DateAndTime(static_cast<int64_t>(tfr));
    }
    else if (timeoption.compare("Relative Time (second)") == 0){
      // ii. relative time
      mFilterT0 = runstart + t0r;
      mFilterTf = runstart + tfr;
    }
    else{
      // iii. percentage
      if (t0r < 0.0){
        t0r = 0.0;
        g_log.warning() << "For percentage T0 cannot be less than 0.  Auto-reset to 0.0 percent." << std::endl;
      }
      if (tfr > 100.0){
        tfr = 100.0;
        g_log.warning() << "For percentage Tf cannot be larger than 100.  Auto-reset to 100 percent." << std::endl;
      }

      int64_t ts = times[0].total_nanoseconds();
      int64_t te = times[times.size()-1].total_nanoseconds();
      mFilterT0 = times[0] + static_cast<int64_t>(static_cast<double>(te-ts)*t0r*0.01);
      mFilterTf = times[0] + static_cast<int64_t>(static_cast<double>(te-ts)*tfr*0.01);
    } // end-if-else

    loginfoss << "User Filter:  T0 = " << mFilterT0 << std::endl <<
        "                   " << mFilterT0.total_nanoseconds() << std::endl;
    loginfoss << "              Tf = " << mFilterTf << std::endl <<
        "                   " << mFilterTf.total_nanoseconds() << std::endl;

    g_log.notice(loginfoss.str());

    // 4) Exam log
    if (examlog){
      // b) Examine the log by printing out some important message
      if (logname.size() > 0){
        examLog(logname, outputdir, true, true);
      } else {
        examLog(logname, outputdir, false, true);
      }
    }

    return;

  } // END

  /*
   * Examine the log.  Output relative information
   * Output the error statistic to workspace
   *
   * @param uselog: log comes from Workspace. but not from input Sample Environment workspace
   * @param resolution:  int, 1, 10, 100... as the resolution for statistic
   */
  void GetTimeSeriesLogInformation::examLog(std::string logname, std::string outputdir, bool uselog, bool checkvaluealter){

    // 1. (Optional) input Workspace dataX in ascending order?
    if (!uselog){
      bool noinorder = false;
      size_t  errcounts = 0;
      for (size_t i = 1; i < seWS->dataX(0).size(); i++){
        if (seWS->dataX(0)[i] <= seWS->dataX(0)[i-1]){
          g_log.error() << "Sample E. Workspace: data " << i << " = " << seWS->dataX(0)[i] <<
              " < data " << i-1 << " = " << seWS->dataX(0)[i-1] << "   ";
          g_log.error() << "dT = " << (seWS->dataX(0)[i]-seWS->dataX(0)[i-1]) <<
              " in total " << seWS->dataX(0).size() << " points" << std::endl;
          noinorder = true;
          errcounts ++;
        }
      }
      if (noinorder){
        g_log.error() << "Total " << errcounts << " pionts are not in order!" << std::endl;
        throw std::invalid_argument("Input Sample E. Workspace is not in ascending order!");
      } else {
        g_log.notice() << "Working space " << seWS->getName() << " is in ascending order" << std::endl;
      }
    } // IF: uselog

    // 2. Calculate the frequency and standard deviation of DELTA-TIME
    std::stringstream msgss;
    std::stringstream errss;

    double sumdeltatime1 = 0.0;
    double sumdeltatime2 = 0.0;
    size_t numpoints = 0;
    size_t numzerodt = 0;

    std::vector<Kernel::DateAndTime> times;
    std::vector<double> values;
    bool logerror = false;

    // NOTE: All record about time are in unit as nano-second

    // i) Get vector of all times
    if (uselog){
      // a) From log
      Kernel::TimeSeriesProperty<double> *prop = dynamic_cast<Kernel::TimeSeriesProperty<double>* >(
          eventWS->run().getProperty(logname));
      if (!prop){
        errss << "Log " << logname << " does not exist or it is not a TimeSeriesProperty" << std::endl;
        logerror = true;
      } else {
        times = prop->timesAsVector();
      }
    } else {
      // b) From Workspace
      const std::vector<double> xvalues = seWS->dataX(0);
      g_log.notice() << "VZ-Debug  Size of X = " << xvalues.size() << std::endl;

      // i) Translate all x-value to DataAndTime
      for (size_t i = 0; i < xvalues.size(); i ++){
        int64_t timens = static_cast<int64_t>(xvalues[i]);
        Kernel::DateAndTime dtime(timens);
        times.push_back(dtime);
      }
    } // IF-ELSE: uselog

    if (!logerror)
    {
      numpoints = times.size();
      if (numpoints == 0){
        g_log.error() << "Zero entries in times array!  Cannot be true!" << std::endl;
        throw;
      }

      // iiA)  Build the value vector if required
      if (uselog){
        Kernel::TimeSeriesProperty<double> *prop = dynamic_cast<Kernel::TimeSeriesProperty<double>* >(
            eventWS->run().getProperty(logname));
        for (size_t i = 0; i < times.size(); i ++){
          values.push_back(prop->getSingleValue(times[i]));
        }
      } else {
        for (size_t i = 0; i < seWS->dataY(0).size(); i ++){
          values.push_back(seWS->dataY(0)[i]);
        }
      }

      // ii) (1) Check whether times is an ordered (ascending) vector
      //     (2) Do statistic
      //     (3) Check alternate values
      double vm1, vm2, vm0;
      size_t numnotalter = 0;
      size_t numevents = 0;
      for (size_t i = 1; i < times.size(); i++)
      {

        // (0) Filter out the time out of range
        if (times[i] < mFilterT0 || times[i] > mFilterTf){
          continue;
        }

        int64_t dtns = times[i].total_nanoseconds() - times[i-1].total_nanoseconds();

        // (1) Check not-allowed situations
        if (dtns < 0)
        {
          g_log.error() << "Vector time is not ordered!" << std::endl;
          throw;
        } else if (dtns == 0)
        {
          numzerodt ++;
        } else {
          numevents ++;
        }

        // (2) Statistics
        sumdeltatime1 += static_cast<double> (dtns);
        sumdeltatime2 += static_cast<double> (dtns) * static_cast<double> (dtns);

        // (3) Optionally alternative value
        if (checkvaluealter && i >= 2){
          vm0 = values[i];
          vm1 = values[i-1];
          vm2 = values[i-2];
          if (fabs(vm0-vm2)*10.0 >= fabs(vm0-vm1)){
            numnotalter ++;
          }
        }
      } // FOR: i

      double avgdeltatime = sumdeltatime1 / static_cast<double> (numpoints);
      double stddeltatime = sqrt(sumdeltatime2 / static_cast<double> (numpoints) - avgdeltatime
          * avgdeltatime);

      msgss << "Number of Time Points = " << numevents << "(" << numpoints << "), Same Points = " << numzerodt << std::endl;
      msgss << "Min.             Time = " << times[0] << " / " << times[0].total_nanoseconds()
          << "  ns " << std::endl;
      msgss << "Max.             Time = " << times[times.size() - 1] << " / " << times[times.size()
          - 1].total_nanoseconds() << " ns" << std::endl;
      msgss << "Average       Delta T = " << avgdeltatime << "  ns,  Standard Deviation = "
          << stddeltatime << std::endl;
      msgss << "Number of points no alter = " << numnotalter << std::endl;

      // ii)  Record values out of standard deviation
      std::vector<size_t> oddindicies;
      std::vector<Kernel::DateAndTime> oddtimes;
      std::vector<double> oddvalues;
      for (size_t i = 1; i < times.size(); i++)
      {

        // (0) Filter out the time out of range
        if (times[i] < mFilterT0 || times[i] > mFilterTf){
          continue;
        }

        int64_t dtns = times[i].total_nanoseconds() - times[i - 1].total_nanoseconds();

        if (fabs(static_cast<double> (dtns) - avgdeltatime) > stddeltatime)
        {
          oddindicies.push_back(i);
          oddtimes.push_back(times[i]);
          oddvalues.push_back(static_cast<double> (dtns) - avgdeltatime);
        }
      }

      // iii) Write out the record for all DeltaT out of 1-sigma
      if (false)
      {
        std::ofstream sigfs;
        std::string sigfilename = outputdir + "/outofsigma.txt";
        sigfs.open(sigfilename.c_str(), std::ios::out);
        int64_t timespan_ns = times[times.size() - 1].total_nanoseconds() - times[0].total_nanoseconds();

        sigfs << "Index" << std::setw(30) << "Time (ns)" << std::setw(30) << "Delta T (ns)"
            << std::setw(30) << "DeltaT - Avg(DeltaT) " << std::setw(30) << "Percent of Time"
            << std::endl;
        for (size_t i = 0; i < oddvalues.size(); i++)
        {
          int64_t temets_ns = oddtimes[i].total_nanoseconds() - times[0].total_nanoseconds();
          double timepercent = static_cast<double> (temets_ns) / static_cast<double> (timespan_ns) * 100;
          sigfs << oddindicies[i] << std::setw(30) << oddtimes[i].total_nanoseconds() << std::setw(30)
              << std::setprecision(10) << (oddvalues[i] + avgdeltatime) << std::setw(30)
              << std::setprecision(10) << oddvalues[i] << std::setw(30) << timepercent << std::endl;
        }
        sigfs.close();
      } else
      {
        g_log.error() << "Temporarily disable writing bad time stamps" << std::endl;
      }

      // iv) Output statistic to
      int resin = this->getProperty("Resolution");
      if (resin <= 0){
        throw std::invalid_argument("Resolution cannot be equal or less than 0");
      }
      size_t resolution = static_cast<size_t>(resin);

      DataObjects::Workspace2D_sptr timestatws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          API::WorkspaceFactory::Instance().create("Workspace2D", 1, resolution, resolution));
      DataObjects::Workspace2D_sptr percentstatws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          API::WorkspaceFactory::Instance().create("Workspace2D", 1, resolution, resolution));
      this->setProperty("TimeOutputWorkspace", timestatws);
      this->setProperty("PercentOutputWorkspace", percentstatws);

      int64_t dt0f_ns = times[times.size()-1].total_nanoseconds() - times[0].total_nanoseconds();
      double ddt0f_ns = static_cast<double>(dt0f_ns);

      // set up x-axis
      double deltatns = ddt0f_ns/static_cast<double>(resolution);
      for (size_t i = 0; i < resolution; i ++){
        double xp = static_cast<double>(i)/static_cast<double>(resolution);
        double xt = static_cast<double>(mFilterT0.total_nanoseconds())+static_cast<double>(i)*deltatns;
        timestatws->dataX(0)[i] = xt;
        timestatws->dataY(0)[i] = 0.0;
        percentstatws->dataX(0)[i] = xp;
        percentstatws->dataY(0)[i] = 0.0;
      }

      // go through all weird data points
      for (size_t i = 0; i < oddtimes.size(); i ++){
        double roughpos = static_cast<double>(oddtimes[i].total_nanoseconds()-mFilterT0.total_nanoseconds())/ddt0f_ns;
        size_t pos = static_cast<size_t>(roughpos*static_cast<double>(resolution));

        if (pos == resolution){
          g_log.warning() << "Slightly out of bound for time stamp " << oddtimes[i] << std::endl;
          pos --;
        } else if (pos > resolution){
          g_log.error() << "Programming error!" << std::endl;
          throw std::invalid_argument("Programming logic error!");
        }

        timestatws->dataY(0)[pos] += 1.0;
        percentstatws->dataY(0)[pos] += 1.0;
      }

      // v) Output largest 20% Delta T (original order is destroyed)
      msgss << "Delta(T) larger than standard deviation:  Number = " << oddindicies.size()
          << std::endl;
      if (oddindicies.size() >= 2)
      {
        msgss << "First Odd @ " << oddindicies[0] << ", Time = " << oddtimes[0] << ",  Delta(T) = "
            << oddvalues[0] << std::endl;
        size_t li = oddindicies.size() - 1;
        msgss << "Last  Odd @ " << oddindicies[li] << ", Time = " << oddtimes[li] << ",  Delta(T) = "
            << oddvalues[li] << std::endl;
      }
      size_t imaxdt = 0;
      double maxdt = -1;
      for (size_t i = 0; i < oddindicies.size(); i++)
      {
        if (oddvalues[i] > maxdt)
        {
          imaxdt = oddindicies[i];
          maxdt = oddvalues[i];
        }
      }
      msgss << "Max Delta(T) @ " << imaxdt << " = " << maxdt << std::endl;

      std::sort(oddvalues.begin(), oddvalues.end());
      for (size_t i = oddvalues.size() - 1; i >= oddvalues.size() - 10; i--)
      {
        msgss << "Index = " << i << "   Delta T = " << oddvalues[i] << std::endl;
      }

    } // ENDIF: logerror

    std::string errstr = errss.str();
    std::string msgstr = msgss.str();

    if (msgstr.size() > 0){
      g_log.notice(msgstr);
    }
    if (errstr.size() > 0){
      g_log.error(errstr);
      return;
    }

    // 3. Print out part result
    size_t numput = 1000;

    // a) Redefine numput if out of range
    if (times.size() < numput*3){
      numput = times.size()/3;
    }

    // b) Output to buffer: first, middle and last
    std::string opfname = outputdir+"/"+"pulse.dat";
    std::ofstream ofs;
    ofs.open(opfname.c_str(), std::ios::out);

    for (size_t i = 0; i < numput; i ++){
      ofs << times[i].total_nanoseconds()-1 << "   " << 0 << std::endl;
      ofs << times[i].total_nanoseconds() << "   " << 1 << std::endl;
      ofs << times[i].total_nanoseconds()+1 << "   " << 0 << std::endl;
    } // ENDFOR
    ofs << std::endl;
    for (size_t i = times.size()/2-numput/2; i < times.size()/2+numput/2; i ++){
      ofs << times[i].total_nanoseconds()-1 << "   " << 0 << std::endl;
      ofs << times[i].total_nanoseconds() << "   " << 1 << std::endl;
      ofs << times[i].total_nanoseconds()+1 << "   " << 0 << std::endl;
    } // ENDFOR
    ofs << std::endl;
    for (size_t i = times.size()-1-numput; i < times.size(); i ++){
      ofs << times[i].total_nanoseconds()-1 << "   " << 0 << std::endl;
      ofs << times[i].total_nanoseconds() << "   " << 1 << std::endl;
      ofs << times[i].total_nanoseconds()+1 << "   " << 0 << std::endl;
    } //ENDFOR

    ofs.close();

    std::string opfname2 = outputdir+"/"+"partial_log.dat";
    std::ofstream ofs2;
    ofs2.open(opfname2.c_str(), std::ios::out);
    for (size_t i = 0; i < numput; i ++){
      ofs2 << times[i].total_nanoseconds() << "   " << values[i] << std::endl;
    } // ENDFOR
    ofs2 << std::endl;
    for (size_t i = times.size()/2-numput/2; i < times.size()/2+numput/2; i ++){
      ofs2 << times[i].total_nanoseconds() << "   " << values[i] << std::endl;
    } // ENDFOR
    ofs << std::endl;
    for (size_t i = times.size()-1-numput; i < times.size(); i ++){
      ofs2 << times[i].total_nanoseconds() << "   " << values[i] << std::endl;
    } //ENDFOR

    ofs2.close();

    return;
  }


} // namespace Mantid
} // namespace Algorithms




























