#include "MantidAlgorithms/GenerateEventsFilter.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{
  DECLARE_ALGORITHM(GenerateEventsFilter)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  GenerateEventsFilter::GenerateEventsFilter()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  GenerateEventsFilter::~GenerateEventsFilter()
  {
  }
  
  void GenerateEventsFilter::initDocs()
  {

  }

  /*
   * Define input
   */
  void GenerateEventsFilter::init()
  {
    declareProperty(
      new WorkspaceProperty<DataObjects::EventWorkspace>("InputWorkspace","",Direction::InOut),
      "An input event workspace" );

    declareProperty(
      new WorkspaceProperty<DataObjects::SplittersWorkspace>("OutputWorkspace","",Direction::Output),
      "The name to use for the output SplittersWorkspace object, i.e., the filter." );

    // 1. Time
    declareProperty("StartTime", 0.0,
        "The start time, in (a) seconds, (b) nanoseconds or (c) percentage of total run time\n"
        "since the start of the run. Events before this time are filtered out. \n"
        "run_start is used as the zero. ");

    declareProperty("StopTime", -1.0,
        "The stop time, in (2) seconds, (b) nanoseconds or (c) percentage of total run time\n"
        "since the start of the run. Events at or after this time are filtered out. \n"
        "run_start is used as the zero. ");

    declareProperty("TimeInterval", -1.0,
        "Length of the time splices if filtered in time only.");

    std::vector<std::string> timeoptions;
    timeoptions.push_back("Seconds");
    timeoptions.push_back("Nanoseconds");
    timeoptions.push_back("Percent");
    declareProperty("TimeType", "Seconds", boost::make_shared<Kernel::StringListValidator>(timeoptions),
        "Type to define StartTime, StopTime and DeltaTime");

    // 2. Log value
    declareProperty("LogName", "",
        "Name of the sample log to use to filter.\n"
        "For example, the pulse charge is recorded in 'ProtonCharge'.");

    declareProperty("MinimumValue", 0.0, "Minimum log value for which to keep events.");

    declareProperty("MaximumValue", -1.0, "Maximum log value for which to keep events.");

    declareProperty("DeltaValue", -1.0,
        "Delta of log value to be sliced into from min log value and max log value.\n"
        "If not given, then only value ");

    declareProperty("SeparateLogValueChangeDirection", false,
        "d(log value)/dt can be positive and negative.  They can be put to different splitters.");

    declareProperty("LogValueTimeSections", 1,
        "In one log value interval, it can be further divided into sections in even time slice.");
  }

  /*
   * Main execute body
   */
  void GenerateEventsFilter::exec()
  {
    // 1. Get general input and output
    mEventWS = this->getProperty("InputWorkspace");
    Kernel::DateAndTime runstart(mEventWS->run().getProperty("run_start")->value());

    mSplitters = boost::shared_ptr<DataObjects::SplittersWorkspace>(new DataObjects::SplittersWorkspace);
    this->setProperty("OutputWorkspace", mSplitters);

    std::cout << "DB9441 Run Start = " << runstart << " / " << runstart.totalNanoseconds() << std::endl;

    // 2. Get Time
    processInputTime(runstart);

    // 3. Get Log
    std::string logname = this->getProperty("LogName");
    if (logname.empty())
    {
      // a) Set filter by time only
      setFilterByTimeOnly();
    }
    else
    {
      // b) Set filter by time and log
      setFilterByValue();
    }



  }

  /*
   * Process the input for time.  A smart but complicated default rule
   */
  void GenerateEventsFilter::processInputTime(Kernel::DateAndTime runstarttime)
  {
    // 1. Get input
    double inpt0 = this->getProperty("StartTime");
    double inptf = this->getProperty("StopTime");
    std::string timetype = this->getProperty("TimeType");

    if (inpt0 < 0)
    {
      throw std::invalid_argument("Input StartTime cannot be negative!");
    }

    // 2. Find maximum time by proton charge
    // FIXME Use this simple method may miss the events in the last pulse
    Kernel::TimeSeriesProperty<double>* protonchargelog =
        dynamic_cast<Kernel::TimeSeriesProperty<double> *>(mEventWS->run().getProperty("proton_charge"));
    Kernel::DateAndTime runend = protonchargelog->lastTime();

    // 3. Set up time-convert unit
    double convertfactor = 1.0;
    if (timetype.compare("Seconds") == 0)
    {
      // a) In unit of seconds
      convertfactor = 1.0E9;
    }
    else if (timetype.compare("Nanoseconds") == 0)
    {
      // b) In unit of nano-seconds
      convertfactor = 1.0;
    }
    else if (timetype.compare("Percent") == 0)
    {
      // c) In unit of percent of total run time
      int64_t runtime_ns = runend.totalNanoseconds()-runstarttime.totalNanoseconds();
      double runtimed_ns = static_cast<double>(runtime_ns);
      convertfactor = 0.01*runtimed_ns;
    }
    else
    {
      // d) Not defined
      g_log.error() << "TimeType " << timetype << " is not supported!" << std::endl;
      throw std::runtime_error("Input TimeType is not supported");
    }

    // 4. Process second round
    int64_t t0_ns = runstarttime.totalNanoseconds() + static_cast<int64_t>(inpt0*convertfactor);
    Kernel::DateAndTime tmpt0(t0_ns);
    mStartTime = tmpt0;

    if (inptf < inpt0)
    {
      mStopTime = runend;
    }
    else
    {
      int64_t tf_ns = runstarttime.totalNanoseconds()+static_cast<int64_t>(inptf*convertfactor);
      Kernel::DateAndTime tmptf(tf_ns);
      mStopTime = tmptf;
    }

    return;
  }

  /*
   * Set splitters by time value / interval only
   */
  void GenerateEventsFilter::setFilterByTimeOnly()
  {
    double timeinterval = this->getProperty("TimeInterval");
    if (timeinterval <= 0.0)
    {
      // 1. Default and thus just one interval
    }

  }

  void GenerateEventsFilter::setFilterByValue()
  {
    // Kernel::TimeSeriesProperty<double>* mLog =
  }


} // namespace Mantid
} // namespace Algorithms
