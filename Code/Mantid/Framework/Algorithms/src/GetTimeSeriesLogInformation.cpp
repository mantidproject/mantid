#include "MantidAlgorithms/GetTimeSeriesLogInformation.h"
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

    std::vector<std::string> funcoptions;
    funcoptions.push_back("Overall Statistic");
    funcoptions.push_back("Time Range Info");
    funcoptions.push_back("Generate Calibration File");
    this->declareProperty("Function", "Time Range Info", new ListValidator(funcoptions),
        "Options for functionalities.");

    this->declareProperty("LogName", "", "Log's name to filter events.");
    bool optional = true;
    this->declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("SampleEnvironmentWorkspace", "", Direction::Input, optional),
        "Input 2D workspace storing sample environment data along with absolute time");

    std::vector<std::string> timeoptions;
    timeoptions.push_back("Absolute Time (nano second)");
    timeoptions.push_back("Relative Time (second)");
    timeoptions.push_back("Percentage");
    this->declareProperty("TimeRangeOption", "Relative Time (second)", new ListValidator(timeoptions),
        "User defined time range (T0, Tf) is of absolute time (second). ");
    this->declareProperty("T0", 0.0, "Earliest time of the events to be selected.  It can be absolute time (ns), relative time (second) or percentage.");
    this->declareProperty("Tf", 100.0, "Latest time of the events to be selected.  It can be absolute time (ns), relative time (second) or percentage.");

    bool optionalopws = true;
    this->declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("TimeOutputWorkspace", "TimeStat", Direction::Output, optionalopws),
        "Output Workspace as the statistic on time (second).");
    this->declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("PercentOutputWorkspace", "PercentStat", Direction::Output, optionalopws),
        "Output Workspace as the statistic on percentage time.");

    this->declareProperty("Resolution", 10, "Resolution of statistic workspace");

    this->declareProperty("DetectorOffset", 1.0, "Unified offset value of detectors.");
    this->declareProperty(new API::FileProperty("CalibrationFile", "", API::FileProperty::OptionalSave),
        "Name of the output calibration file.");

    this->declareProperty(new API::FileProperty("OutputDirectory", "", API::FileProperty::OptionalDirectory),
        "Directory where the information file will be written to.");

    return;
  }

  /*
   * Main execution
   */
  void GetTimeSeriesLogInformation::exec()
  {
    // 1. Get property
    eventWS = this->getProperty("InputEventWorkspace");

    std::string funcoption = this->getProperty("Function");

    // 2. Do the work
    if (funcoption.compare("Time Range Info") == 0){
      // 2a) Calculate and output time range information
      this->doTimeRangeInformation();
    }
    else if (funcoption.compare("Overall Statistic") == 0)
    {
      this->doStatistic();
    }
    else if (funcoption.compare("Generate Calibration File") == 0){
      // 2c) Generate calibration file
      this->generateCalibrationFile();
    }
    else{
      g_log.error() << "Functionality option " << funcoption << " is not supported!" << std::endl;
    }

    return;
  }

  /*
   * Generate calibration file
   */
  void GetTimeSeriesLogInformation::generateCalibrationFile(){

    g_log.notice() << "Generating calibration file" << std::endl;

    double offset = this->getProperty("DetectorOffset");

    // 1. Open file
    std::string calfilename = this->getProperty("CalibrationFile");
    std::ofstream ofs;
    ofs.open(calfilename.c_str(), std::ios::out);

    // 2. Write file
    for (size_t i = 0; i < eventWS->getNumberHistograms(); i ++){

      // a) Find detector of the spectrum
      DataObjects::EventList* events = eventWS->getEventListPtr(i);
      std::set<detid_t> detids = events->getDetectorIDs();

      if (detids.size() != 1){
        g_log.error() << "Spectrum (workspace) " << i << " has " << detids.size() << " detectors.  It is not allowed!" << std::endl;
        break;
      }

      detid_t detid = 0;
      for (std::set<detid_t>::iterator it=detids.begin(); it!=detids.end(); ++it)
        detid = *it;

      // b) Write
      ofs << std::setw(10) << detid << std::setprecision(5) << std::setw(15) << offset << std::endl;

    }


    // 3. Close file
    ofs.close();

    return;
  }

  /*
   * Do statistic on user proposed range and examine the log
   * inside the given time range.
   */
  void GetTimeSeriesLogInformation::doTimeRangeInformation()
  {

    // 1.  Get log, time, and etc.
    // 1.1 General
    std::string logname = getProperty("LogName");
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
      throw std::invalid_argument("Log must be given!");
    }

    // 1.2 Time
    double t0r = this->getProperty("T0");
    double tfr = this->getProperty("Tf");
    if (t0r >= tfr){
      g_log.error() << "User defined filter starting time (T0 = " << t0r
          << ") is later than ending time (Tf = " << tfr << ")" << std::endl;
      throw std::invalid_argument("User input T0 and Tf error!");
    }
    std::string timeoption = this->getProperty("TimeRangeOption");

    // 2) Set up data structures
    const API::Run& runlog = eventWS->run();
    std::string runstartstr = runlog.getProperty("run_start")->value();
    Kernel::DateAndTime runstart(runstartstr);
    mRunStartTime = runstart;

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

      int64_t ts = times[0].totalNanoseconds();
      int64_t te = times[times.size()-1].totalNanoseconds();
      mFilterT0 = times[0] + static_cast<int64_t>(static_cast<double>(te-ts)*t0r*0.01);
      mFilterTf = times[0] + static_cast<int64_t>(static_cast<double>(te-ts)*tfr*0.01);
    } // end-if-else

    // 2. Print out some information about log and run
    std::stringstream loginfoss;
    int64_t dt0_ns = times[0].totalNanoseconds()-runstart.totalNanoseconds();
    int64_t dtf_ns = times[times.size()-1].totalNanoseconds()-runstart.totalNanoseconds();
    loginfoss << "Run Start Time = " << runstart << std::endl <<
        "                 " << runstart.totalNanoseconds() << std::endl;
    loginfoss << "Log Start Time = " << times[0] << std::endl <<
        "                 " << times[0].totalNanoseconds() << "nano-second = "<<
        (static_cast<double>(times[0].totalNanoseconds())*1.0E-9) << " seconds " << std::endl <<
        "  To Run Start dT = " << dt0_ns << " ns" << std::endl <<
        "                    " << static_cast<double>(dt0_ns)/static_cast<double>(dtf_ns) << std::endl;
    loginfoss << "Log End   Time = " << times[times.size()-1] << std::endl <<
        "                 " << times[times.size()-1].totalNanoseconds() << " nano-second  =  " <<
        (static_cast<double>(times[times.size()-1].totalNanoseconds())*1.0E-9) << " seconds " << std::endl <<
        "  To Run Start dT = " << dtf_ns << " ns" << std::endl <<
        "                    " << static_cast<double>(dtf_ns)/static_cast<double>(dtf_ns)*100 << " percent" << std::endl;;

    loginfoss << "User Filter:  T0 = " << mFilterT0 << std::endl <<
        "                   " << mFilterT0.totalNanoseconds() << std::endl;
    loginfoss << "              Tf = " << mFilterTf << std::endl <<
        "                   " << mFilterTf.totalNanoseconds() << std::endl;

    g_log.notice(loginfoss.str());

    // 4. Do more staticts (examine)
    std::string outputdir = this->getProperty("OutputDirectory");
    examLog(logname, outputdir);

    return;

  } // END

  /*
   * Examine the log.  Output relative information.  It is a simulation to real filtering
   * Output the error statistic to workspace
   * NOTE: All record about time are in unit as nano-second
   *
   * @param resolution:  int, 1, 10, 100... as the resolution for statistic
   */
  void GetTimeSeriesLogInformation::examLog(std::string logname, std::string outputdir){

    std::vector<Kernel::DateAndTime> times;
    std::vector<double> values;

    // 1. Get vector of all times
    Kernel::TimeSeriesProperty<double> *prop = dynamic_cast<Kernel::TimeSeriesProperty<double>* >(
        eventWS->run().getProperty(logname));
    bool logerror = false;
    if (!prop){
      logerror = true;
    } else {
      times = prop->timesAsVector();
    }

    if (logerror)
    {
      g_log.error() << "Log " << logname << " is not a TimeSeriesProperty log" << std::endl;
      throw std::invalid_argument("Input log is not a TimeSeriesProperty log");
    }

    // 2. Calculate the frequency and standard deviation of DELTA-TIME
    std::stringstream msgss;
    msgss << "\n";

    double sumdeltatime1 = 0.0;
    double sumdeltatime2 = 0.0;
    size_t numpoints = 0;
    size_t numzerodt = 0;

    numpoints = times.size();
    if (numpoints == 0)
    {
      g_log.error() << "Zero entries in times array!  Cannot be true!" << std::endl;
      throw std::invalid_argument("Log has zero entries!");
    } else
    {
      g_log.notice() << "Log has " << numpoints << " entries" << std::endl;
    }

    for (size_t i = 0; i < times.size(); i ++)
    {
      values.push_back(prop->getSingleValue(times[i]));
    }


    // 2 (i)   Check whether times is an ordered (ascending) vector
    //   (ii)  Do statistic
    size_t numevents = 0;
    size_t numoutrange = 0;
    for (size_t i = 1; i < times.size(); i++)
    {
      // (a) Filter out the time out of range
      if (times[i] < mFilterT0 || times[i] > mFilterTf)
      {
        numoutrange ++;
        continue;
      }

      int64_t dtns = times[i].totalNanoseconds() - times[i-1].totalNanoseconds();

      // (b) Check not-allowed situations
      if (dtns < 0)
      {
        g_log.error() << "Vector time is not ordered!" << std::endl;
        throw std::runtime_error("Vector times (absolute time) is not ordered!");
      }
      else if (dtns == 0)
      {
        numzerodt ++;
      }
      else {
        numevents ++;
      }

      // (b) Statistics
      sumdeltatime1 += static_cast<double> (dtns);
      sumdeltatime2 += static_cast<double> (dtns) * static_cast<double> (dtns);

    } // FOR: i

    // 3. Output for delta(T) on time stamps within selected range
    double avgdeltatime = sumdeltatime1 / static_cast<double> (numpoints);
    double stddeltatime = sqrt(sumdeltatime2 / static_cast<double> (numpoints) - avgdeltatime
        * avgdeltatime);

    msgss << "Time Range            = " << mFilterT0 << ", " << mFilterTf << ".  Delta(T) = " << mFilterTf-mFilterT0 << std::endl;
    msgss << "Number of Points In   = " << numevents << "(" << numpoints << "), Same Points = " << numzerodt << std::endl;
    msgss << "NUmber of Points Out  = " << numoutrange << std::endl;
    msgss << "Min.             Time = " << times[0] << " / " << times[0].totalNanoseconds()
        << "  ns " << std::endl;
    msgss << "Max.             Time = " << times[times.size() - 1] << " / " << times[times.size()
        - 1].totalNanoseconds() << " ns" << std::endl;
    msgss << "Average       Delta T = " << avgdeltatime << "  ns,  Standard Deviation = "
        << stddeltatime << std::endl;

    // 4.  Record values out of standard deviation
    std::vector<size_t> oddindicies;
    std::vector<Kernel::DateAndTime> oddtimes;
    std::vector<double> oddvalues;
    for (size_t i = 1; i < times.size(); i++)
    {

      // (0) Filter out the time out of range
      if (times[i] < mFilterT0 || times[i] > mFilterTf){
        continue;
      }

      int64_t dtns = times[i].totalNanoseconds() - times[i - 1].totalNanoseconds();

      if (fabs(static_cast<double> (dtns) - avgdeltatime) > stddeltatime)
      {
        oddindicies.push_back(i);
        oddtimes.push_back(times[i]);
        oddvalues.push_back(static_cast<double> (dtns) - avgdeltatime);
      }
    }

    // 4.2 Write out the record for all DeltaT out of 1 \sigma, if file size is not too large
    std::ofstream sigfs;
    std::string sigfilename = outputdir + "/outofsigma.txt";

    size_t numwriteout = 1000;
    if (oddvalues.size() < numwriteout)
      numwriteout = oddvalues.size();

    g_log.notice() << "Writing " << numwriteout << " events outside 1 sigma out of " << oddvalues.size() <<
        " to file " << sigfilename << std::endl;

    int64_t timespan_ns = times[times.size() - 1].totalNanoseconds() - times[0].totalNanoseconds();

    sigfs.open(sigfilename.c_str(), std::ios::out);
    sigfs << "Index" << std::setw(30) << "Time (ns)" << std::setw(30) << "Delta T (ns)"
        << std::setw(30) << "DeltaT - Avg(DeltaT) " << std::setw(30) << "Percent of Time"
        << std::endl;
    for (size_t i = 0; i < oddvalues.size(); i++)
    {
      int64_t temets_ns = oddtimes[i].totalNanoseconds() - times[0].totalNanoseconds();
      double timepercent = static_cast<double> (temets_ns) / static_cast<double> (timespan_ns) * 100;
      sigfs << oddindicies[i] << std::setw(30) << oddtimes[i].totalNanoseconds() << std::setw(30)
          << std::setprecision(10) << (oddvalues[i] + avgdeltatime) << std::setw(30)
          << std::setprecision(10) << oddvalues[i] << std::setw(30) << timepercent << std::endl;
    }
    sigfs.close();


    // 5. Output statistic of bad time stamps to Matrixworkspace
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

    double ddt0f_ns = static_cast<double>(timespan_ns);

    // 5.1 set up x-axis
    double deltatns = ddt0f_ns/static_cast<double>(resolution);
    for (size_t i = 0; i < resolution; i ++){
      double xp = static_cast<double>(i)/static_cast<double>(resolution);
      double xt = static_cast<double>(mFilterT0.totalNanoseconds())+static_cast<double>(i)*deltatns;
      timestatws->dataX(0)[i] = xt;
      timestatws->dataY(0)[i] = 0.0;
      percentstatws->dataX(0)[i] = xp;
      percentstatws->dataY(0)[i] = 0.0;
    }

    // 5.2 go through all weird data points
    for (size_t i = 0; i < oddtimes.size(); i ++)
    {
      double roughpos = static_cast<double>(oddtimes[i].totalNanoseconds()-mFilterT0.totalNanoseconds())/ddt0f_ns;
      size_t pos = static_cast<size_t>(roughpos*static_cast<double>(resolution));

      if (pos == resolution)
      {
        g_log.warning() << "Slightly out of bound for time stamp " << oddtimes[i] << std::endl;
        pos --;
      } else if (pos > resolution)
      {
        g_log.error() << "Programming error!" << std::endl;
        throw std::invalid_argument("Programming logic error!");
      }

      timestatws->dataY(0)[pos] += 1.0;
      percentstatws->dataY(0)[pos] += 1.0;
    }

    // 5.3 Output largest 20% Delta T (original order is destroyed)
    /*
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
    */

    // 5.4 Find Max Delta(T)
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

    /*
    std::sort(oddvalues.begin(), oddvalues.end());
    for (size_t i = oddvalues.size() - 1; i >= oddvalues.size() - 10; i--)
    {
      msgss << "Index = " << i << "   Delta T = " << oddvalues[i] << std::endl;
    }
    */

    std::string msgstr = msgss.str();

    if (msgstr.size() > 0){
      g_log.notice(msgstr);
    }

    // 6. Print out part result
    /*
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
      ofs << times[i].totalNanoseconds()-1 << "   " << 0 << std::endl;
      ofs << times[i].totalNanoseconds() << "   " << 1 << std::endl;
      ofs << times[i].totalNanoseconds()+1 << "   " << 0 << std::endl;
    } // ENDFOR
    ofs << std::endl;
    for (size_t i = times.size()/2-numput/2; i < times.size()/2+numput/2; i ++){
      ofs << times[i].totalNanoseconds()-1 << "   " << 0 << std::endl;
      ofs << times[i].totalNanoseconds() << "   " << 1 << std::endl;
      ofs << times[i].totalNanoseconds()+1 << "   " << 0 << std::endl;
    } // ENDFOR
    ofs << std::endl;
    for (size_t i = times.size()-1-numput; i < times.size(); i ++){
      ofs << times[i].totalNanoseconds()-1 << "   " << 0 << std::endl;
      ofs << times[i].totalNanoseconds() << "   " << 1 << std::endl;
      ofs << times[i].totalNanoseconds()+1 << "   " << 0 << std::endl;
    } //ENDFOR

    ofs.close();

    std::string opfname2 = outputdir+"/"+"partial_log.dat";
    std::ofstream ofs2;
    ofs2.open(opfname2.c_str(), std::ios::out);
    for (size_t i = 0; i < numput; i ++){
      ofs2 << times[i].totalNanoseconds() << "   " << values[i] << std::endl;
    } // ENDFOR
    ofs2 << std::endl;
    for (size_t i = times.size()/2-numput/2; i < times.size()/2+numput/2; i ++){
      ofs2 << times[i].totalNanoseconds() << "   " << values[i] << std::endl;
    } // ENDFOR
    ofs << std::endl;
    for (size_t i = times.size()-1-numput; i < times.size(); i ++){
      ofs2 << times[i].totalNanoseconds() << "   " << values[i] << std::endl;
    } //ENDFOR

    ofs2.close();
     */

    return;
  }

  /*
   *  Perform statistic to log including its distribution and etc.
   *  Code is migrated from
   */
  void GetTimeSeriesLogInformation::doStatistic()
  {
    // 1. Get time series values and vector
    std::string logname = getProperty("LogName");
    std::vector<double> values;
    std::vector<Kernel::DateAndTime> timevec;
    if (!logname.empty())
    {
      Kernel::TimeSeriesProperty<double>* tlog = dynamic_cast<Kernel::TimeSeriesProperty<double>* >(
          eventWS->run().getProperty(logname));
      if (!tlog){
        g_log.error() << "TimeSeriesProperty Log " << logname << " does not exist in workspace " <<
            eventWS->getName() << std::endl;
        throw std::invalid_argument("TimeSeriesProperty log cannot be found");
      }
      timevec = tlog->timesAsVector();
      values = tlog->valuesAsVector();
    }
    else
    {
      throw std::invalid_argument("Log name must be given!");
    }

    // 1. Check whether the log is alternating
    this->checkLogBasicInforamtion(eventWS, logname);
    checkLogAlternating(timevec, values, 0.1);

    // 2. Do some static on time stamps
    g_log.notice() << "Vector size = " << timevec.size() << std::endl;
    double sum1dtms = 0.0; // sum(dt^2)
    double sum2dtms = 0.0; // sum(dt^2)
    size_t numinvert = 0;
    size_t numsame = 0;
    size_t numnormal = 0;
    double maxdtms = 0;
    double mindtms = 1.0E20;
    size_t numdtabove10p = 0;
    size_t numdtbelow10p = 0;

    double sampledtms = 0.00832646*1.0E6;
    double dtmsA10p = sampledtms*1.1;
    double dtmsB10p = sampledtms/1.0;

    for (size_t i = 1; i < timevec.size(); i ++)
    {
      int64_t dtns = timevec[i].totalNanoseconds()-timevec[i-1].totalNanoseconds();
      double dtms = static_cast<double>(dtns)*1.0E-3;

      sum1dtms += dtms;
      sum2dtms += dtms*dtms;
      if (dtns == 0)
        numsame ++;
      else if (dtns < 0)
        numinvert ++;
      else
        numnormal ++;

      if (dtms > maxdtms)
        maxdtms = dtms;
      if (dtms < mindtms)
        mindtms = dtms;

      if (dtms > dtmsA10p)
        numdtabove10p ++;
      else if (dtms < dtmsB10p)
        numdtbelow10p ++;

    } // ENDFOR

    double dt = sum1dtms/static_cast<double>(timevec.size())*1.0E-6;
    double stddt = sqrt(sum2dtms/static_cast<double>(timevec.size())*1.0E-12 - dt*dt);

    g_log.notice() << "Normal   dt = " << numnormal << std::endl;
    g_log.notice() << "Zero     dt = " << numsame << std::endl;
    g_log.notice() << "Negative dt = " << numinvert << std::endl;
    g_log.notice() << "Avg d(T) = " << dt << " seconds +/- " << stddt << ",  Frequency = " << 1.0/dt << std::endl;
    g_log.notice() << "d(T) (unit ms) is in range [" << mindtms << ", " << maxdtms << "]"<< std::endl;
    g_log.notice() << "Number of d(T) 10% larger than average  = " << numdtabove10p << std::endl;
    g_log.notice() << "Number of d(T) 10% smaller than average = " << numdtbelow10p << std::endl;

    g_log.notice() << "Size of timevec = " << timevec.size() << std::endl;

    exportErrorLog(eventWS, timevec, 1/(240.1));
    calDistributions(timevec, 1/(240.1));

  }

  /*
   * Export time stamps looking erroreous
   * @param dts: standard delta T in second
   */
  void GetTimeSeriesLogInformation::exportErrorLog(API::MatrixWorkspace_sptr ws, std::vector<Kernel::DateAndTime> abstimevec, double dts)
  {
    std::string outputdir = getProperty("OutputDirectory");
    if (outputdir[outputdir.size()-1] != '/')
      outputdir += "/";

    std::string ofilename = outputdir + "errordeltatime.txt";
    g_log.notice() << ofilename << std::endl;
    std::ofstream ofs;
    ofs.open(ofilename.c_str(), std::ios::out);

    size_t numbaddt = 0;
    Kernel::DateAndTime t0(ws->run().getProperty("run_start")->value());

    for (size_t i = 1; i < abstimevec.size(); i++)
    {
      double tempdts = static_cast<double>(abstimevec[i].totalNanoseconds()-abstimevec[i-1].totalNanoseconds())*1.0E-9;
      double dev = (tempdts-dts)/dts;
      bool baddt = false;
      if (fabs(dev) > 0.5)
        baddt = true;

      if (baddt)
      {
        numbaddt ++;
        double deltapulsetimeSec1 = static_cast<double>(abstimevec[i-1].totalNanoseconds()-t0.totalNanoseconds())*1.0E-9;
        double deltapulsetimeSec2 = static_cast<double>(abstimevec[i].totalNanoseconds()-t0.totalNanoseconds())*1.0E-9;
        int index1 = static_cast<int>(deltapulsetimeSec1*60);
        int index2 = static_cast<int>(deltapulsetimeSec2*60);

        ofs << "Error d(T) = " << tempdts << "   vs   Correct d(T) = " << dts << std::endl;
        ofs << index1 << "\t\t" << abstimevec[i-1].totalNanoseconds()  <<
            "\t\t" << index2 << "\t\t" << abstimevec[i].totalNanoseconds() << std::endl;

      }
    }

    ofs.close();

  }

  /*
   * Output distributions in order for a better understanding of the log
   * @param dts: d(T) in second
   */
  void GetTimeSeriesLogInformation::calDistributions(std::vector<Kernel::DateAndTime> timevec, double dts)
  {
    // 1. Calculate percent deviation vs. number of cases
    std::vector<double> x1, y1;
    for (int i=-99; i < 100; i++)
    {
      x1.push_back(static_cast<double>(i));
      y1.push_back(0);
    }

    for (size_t i = 1; i < timevec.size(); i ++)
    {
      double tempdts = static_cast<double>(timevec[i].totalNanoseconds()-timevec[i-1].totalNanoseconds())*1.0E-9;
      int index = static_cast<int>((tempdts-dts)/dts*100)+99;
      if (index < 0)
        index = 0;
      else if (index > 199)
        index = 19;
      y1[static_cast<size_t>(index)]++;
    }

    /* Skip output */
    for (size_t i = 0; i < x1.size(); i ++)
      g_log.notice() << i << "\t\t" << x1[i] << "\t\t" << y1[i] << std::endl;
     /**/

    // 2. Calculate space distribution on error cases
    std::vector<double> x2s;
    std::vector<size_t> y2;

    size_t numperiods = 100;
    int64_t spanns = timevec[timevec.size()-1].totalNanoseconds()-timevec[0].totalNanoseconds();
    double timestepsec = static_cast<double>(spanns)*1.0E-9/static_cast<double>(numperiods);

    for (size_t i = 0; i < numperiods; i++)
    {
      x2s.push_back(static_cast<double>(i)*timestepsec);
      y2.push_back(0);
    }

    size_t numbaddt = 0;
    for (size_t i = 1; i < timevec.size(); i ++)
    {
      double tempdts = static_cast<double>(timevec[i].totalNanoseconds()-timevec[i-1].totalNanoseconds())*1.0E-9;
      double dev = (tempdts-dts)/dts;
      bool baddt = false;
      if (fabs(dev) > 0.5)
        baddt = true;

      if (baddt)
      {
        numbaddt ++;
        int index = static_cast<int>(static_cast<double>(timevec[i].totalNanoseconds()-timevec[0].totalNanoseconds())*1.0E-9/timestepsec);
        if (index < 0)
          throw std::runtime_error("Impossible to have index less than 0");
        if (index >= static_cast<int>(numperiods))
        {
          g_log.error() << "Logic error X" << std::endl;
          index = static_cast<int>(numperiods)-1;
        }
        y2[static_cast<size_t>(index)] ++;
      }
    } // ENDFOR

    /* Skip
    for (size_t i = 0; i < x2s.size(); i ++)
      g_log.notice() << i << "\t\t" << x2s[i] << "\t\t" << y2[i] << std::endl;
      */
    g_log.notice() << "total number of wrong dt = " << numbaddt << std::endl;

    return;
  }


  /*
   * Check log in workspace
   */
  void GetTimeSeriesLogInformation::checkLogBasicInforamtion(API::MatrixWorkspace_sptr ws, std::string logname)
  {
    // 1. Get log
    Kernel::Property* log = ws->run().getProperty(logname);
    if (!log)
    {
      g_log.error() << "Log " << logname << " does not exist!" << std::endl;
      throw std::invalid_argument("Non-exising log name");
    }
    Kernel::TimeSeriesProperty<double>* tslog = dynamic_cast<Kernel::TimeSeriesProperty<double>* >(log);
    if (!tslog)
    {
      g_log.error() << "Log " << logname << " is not time series log" << std::endl;
      throw std::invalid_argument("Log type error!");
    }

    // 2. Survey
    std::vector<Kernel::DateAndTime> times = tslog->timesAsVector();
    g_log.information() << "Entries of times = " << times.size() << std::endl;
    size_t countsame = 0;
    size_t countinverse = 0;
    for (size_t i=1; i<times.size(); i++)
    {
      Kernel::DateAndTime tprev = times[i-1];
      Kernel::DateAndTime tpres = times[i];
      if (tprev == tpres)
        countsame ++;
      else if (tprev > tpres)
        countinverse ++;
    }

    // 3. Output
    Kernel::DateAndTime t0(ws->run().getProperty("run_start")->value());
    Kernel::time_duration dts = times[0]-t0;
    Kernel::time_duration dtf = times[times.size()-1]-t0;
    size_t f = times.size()-1;

    g_log.information() << "Number of Equal Time Stamps    = " << countsame << std::endl;
    g_log.information() << "Number of Inverted Time Stamps = " << countinverse << std::endl;
    g_log.information() << "Run Start = " << t0.totalNanoseconds() << std::endl;
    g_log.information() << "First Log (Absolute Time, Relative Time): " << times[0].totalNanoseconds() << ", "
        << Kernel::DateAndTime::nanosecondsFromDuration(dts) << std::endl;
    g_log.information() << "Last  Log (Absolute Time, Relative Time): " << times[f].totalNanoseconds() << ", "
        << Kernel::DateAndTime::nanosecondsFromDuration(dtf) << std::endl;

    return;
  }

  /*
   * Check whether log values are alternating
   * @param delta: if adjacent log values differs less than this number, then it is not considered as alternating
   */
  void GetTimeSeriesLogInformation::checkLogAlternating(std::vector<Kernel::DateAndTime>timevec, std::vector<double> values,
      double delta)
  {
    std::stringstream ss;

    ss << "Alternating Threashold = " << delta << std::endl;

    size_t numocc = 0;

    for (size_t i = 1; i < values.size(); i ++)
    {
      double tempdelta = values[i]-values[i-1];
      if (fabs(tempdelta) < delta)
      {
        // Value are 'same'
        numocc ++;

        // An error message
        ss << "@ " << i << "\tDelta = " << tempdelta << "\t\tTime From " << timevec[i-1].totalNanoseconds()
            << " to " << timevec[i].totalNanoseconds() << std::endl;
      }
    }

    g_log.warning() << "Total non-alternating time stamps = " << numocc << " among " << timevec.size() << " cases. " << std::endl;
    g_log.warning() << ss.str();

    return;
  }


} // namespace Mantid
} // namespace Algorithms




























