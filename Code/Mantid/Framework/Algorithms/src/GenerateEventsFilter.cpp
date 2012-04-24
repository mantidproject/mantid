#include "MantidAlgorithms/GenerateEventsFilter.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/Column.h"

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

    // 0. Input/Output Workspaces
    declareProperty(
      new API::WorkspaceProperty<DataObjects::EventWorkspace>("InputWorkspace", "Anonymous", Direction::InOut),
      "An input event workspace" );

    declareProperty(
      new API::WorkspaceProperty<DataObjects::SplittersWorkspace>("OutputWorkspace", "", Direction::Output),
      "The name to use for the output SplittersWorkspace object, i.e., the filter." );

    declareProperty(new API::WorkspaceProperty<API::ITableWorkspace>("SplittersInformationWorkspace", "SplitterInfo", Direction::Output),
        "Optional output for the information of each splitter workspace index");

    // 1. Time
    declareProperty("StartTime", "0.0",
        "The start time, in (a) seconds, (b) nanoseconds or (c) percentage of total run time\n"
        "since the start of the run. OR (d) absolute time. \n"
        "Events before this time are filtered out.");

    declareProperty("StopTime", "-1.0",
        "The stop time, in (2) seconds, (b) nanoseconds or (c) percentage of total run time\n"
        "since the start of the run. OR (d) absolute time. \n"
        "Events at or after this time are filtered out.");

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

    declareProperty("MinimumLogValue", 0.0, "Minimum log value for which to keep events.");

    declareProperty("MaximumLogValue", -1.0, "Maximum log value for which to keep events.");

    declareProperty("LogValueInterval", -1.0,
        "Delta of log value to be sliced into from min log value and max log value.\n"
        "If not given, then only value ");

    std::vector<std::string> filteroptions;
    filteroptions.push_back("Both");
    filteroptions.push_back("Increase");
    filteroptions.push_back("Decrease");
    declareProperty("FilterLogValueByChangingDirection", "Both", boost::make_shared<Kernel::StringListValidator>(filteroptions),
        "d(log value)/dt can be positive and negative.  They can be put to different splitters.");

    declareProperty("TimeTolerance", 0.0,
        "Tolerance in time for the event times to keep. It is used in the case to filter by single value.");

    declareProperty("LogBoundary", "centre",
        "How to treat log values as being measured in the centre of time.");

    declareProperty("LogValueTolerance", -0.0,
        "Tolerance of the log value to be included in filter.  It is used in the case to filter by multiple values.");

    /* removed due to SNS hardware
    std::vector<std::string> logvalueoptions;
    logvalueoptions.push_back("StepFunction");
    logvalueoptions.push_back("LinearInterpolation");
    declareProperty("LogValueInterpolation", "StepFunction", boost::make_shared<Kernel::StringListValidator>(logvalueoptions),
        "How to treat the changing log value in multiple-value filtering.");
    */

    declareProperty("LogValueTimeSections", 1,
        "In one log value interval, it can be further divided into sections in even time slice.");

    return;
  }


  /*
   * Main execute body
   */
  void GenerateEventsFilter::exec()
  {
    // 1. Get general input and output
    mEventWS = this->getProperty("InputWorkspace");
    Kernel::DateAndTime runstart(mEventWS->run().getProperty("run_start")->value());
    std::cout << "DB9441 Run Start = " << runstart << " / " << runstart.totalNanoseconds() << std::endl;


    mSplitters =  boost::shared_ptr<DataObjects::SplittersWorkspace>(new DataObjects::SplittersWorkspace());
    mSplitters->setName("Splitter_Name");
    mSplitters->setTitle("Splitters_Title");

    // mFilterInfoWS = boost::shared_ptr<DataObjects::TableWorkspace>(new DataObjects::TableWorkspace);
    mFilterInfoWS = API::WorkspaceFactory::Instance().createTable("TableWorkspace");

    mFilterInfoWS->addColumn("int", "Workspaceindex");
    mFilterInfoWS->addColumn("str", "Title");

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
      setFilterByLogValue(logname);
    }

    /* FIXME Deletet this section before finalizing the code  *
    for (size_t i = 0; i < mSplitters->getNumberSplitters(); ++i)
    {
      Kernel::SplittingInterval split = mSplitters->getSplitter(i);
      std::cout << split.start().totalNanoseconds() << "\t\t" << split.stop().totalNanoseconds() << "\t\t" << split.index() << std::endl;
    }
    **********************************************************/

    this->setProperty("OutputWorkspace", mSplitters);
    this->setProperty("SplittersInformationWorkspace", mFilterInfoWS);

    return;
  }

  /*
   * Process the input for time.  A smart but complicated default rule
   */
  void GenerateEventsFilter::processInputTime(Kernel::DateAndTime runstarttime)
  {
    // 1. Get input
    std::string s_inpt0 = this->getProperty("StartTime");
    std::string s_inptf = this->getProperty("StopTime");

    // 2. Check if input are in double or string
    bool instringformat = true;
    if (s_inpt0.find(':') >= s_inpt0.size())
    {
      instringformat = false;
    }

    if (instringformat)
    {
      // 1. In absolute time ISO format
      Kernel::DateAndTime t0(s_inpt0);
      Kernel::DateAndTime t1(s_inptf);
      mStartTime = t0;
      mStopTime = t1;
    }
    else
    {
      // 2. In double relative time format
      std::string timetype = this->getProperty("TimeType");
      double inpt0 = atof(s_inpt0.c_str());
      double inptf = atof(s_inptf.c_str());

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
      m_convertfactor = 1.0;
      if (timetype.compare("Seconds") == 0)
      {
        // a) In unit of seconds
        m_convertfactor = 1.0E9;
      }
      else if (timetype.compare("Nanoseconds") == 0)
      {
        // b) In unit of nano-seconds
        m_convertfactor = 1.0;
      }
      else if (timetype.compare("Percent") == 0)
      {
        // c) In unit of percent of total run time
        int64_t runtime_ns = runend.totalNanoseconds()-runstarttime.totalNanoseconds();
        double runtimed_ns = static_cast<double>(runtime_ns);
        m_convertfactor = 0.01*runtimed_ns;
      }
      else
      {
        // d) Not defined
        g_log.error() << "TimeType " << timetype << " is not supported!" << std::endl;
        throw std::runtime_error("Input TimeType is not supported");
      }

      // 4. Process second round
      int64_t t0_ns = runstarttime.totalNanoseconds() + static_cast<int64_t>(inpt0*m_convertfactor);
      Kernel::DateAndTime tmpt0(t0_ns);
      mStartTime = tmpt0;

      if (inptf < inpt0)
      {
        mStopTime = runend;
      }
      else
      {
        int64_t tf_ns = runstarttime.totalNanoseconds()+static_cast<int64_t>(inptf*m_convertfactor);
        Kernel::DateAndTime tmptf(tf_ns);
        mStopTime = tmptf;
      }
    }

    std::cout << "DB8147 StartTime = " << mStartTime << ", StopTime = " << mStopTime << std::endl;

    return;
  }

  /*
   * Set splitters by time value / interval only
   */
  void GenerateEventsFilter::setFilterByTimeOnly()
  {
    double timeinterval = this->getProperty("TimeInterval");
    int wsindex = 0;

    if (timeinterval <= 0.0)
    {
      // 1. Default and thus just one interval
      Kernel::SplittingInterval ti(mStartTime, mStopTime, 0);
      mSplitters->addSplitter(ti);

      API::TableRow row = mFilterInfoWS->appendRow();
      std::stringstream ss;
      ss << "Time Interval From " << mStartTime << " to " << mStopTime;
      row << wsindex << ss.str();
    }
    else
    {
      // 2. Use N time interval
      int64_t deltatime_ns = static_cast<int64_t>(timeinterval*m_convertfactor);

      int64_t curtime_ns = mStartTime.totalNanoseconds();
      int wsindex = 0;
      while (curtime_ns < mStopTime.totalNanoseconds())
      {
        // a) Calculate next.time
        int64_t nexttime_ns = curtime_ns + deltatime_ns;
        if (nexttime_ns > mStopTime.totalNanoseconds())
          nexttime_ns = mStopTime.totalNanoseconds();

        // b) Create splitter
        Kernel::DateAndTime t0(curtime_ns);
        Kernel::DateAndTime tf(nexttime_ns);
        Kernel::SplittingInterval spiv(t0, tf, wsindex);
        mSplitters->addSplitter(spiv);

        // c) Information
        API::TableRow row = mFilterInfoWS->appendRow();
        std::stringstream ss;
        ss << "Time Interval From " << t0 << " to " << tf;
        row << wsindex << ss.str();

        // d) Update loop variable
        curtime_ns = nexttime_ns;
        wsindex ++;
      } // END-WHILE

    } // END-IF-ELSE

    return;
  }

  void GenerateEventsFilter::setFilterByLogValue(std::string logname)
  {
    // 1. Process inputs
    Kernel::TimeSeriesProperty<double>* mLog =
        dynamic_cast<Kernel::TimeSeriesProperty<double>* >(mEventWS->run().getProperty(logname));
    if (!mLog)
    {
      g_log.error() << "Log " << logname << " does not exist or is not TimeSeriesProperty in double." << std::endl;
      throw std::invalid_argument("User specified log is not correct");
    }

    /* FIXME  Delete This Section After Debugging
    std::vector<Kernel::DateAndTime> times = mLog->timesAsVector();
    std::vector<double> values = mLog->valuesAsVector();
    for (size_t i = 0; i < times.size(); ++i)
      std::cout << "DBOP " << times[i].totalNanoseconds() << "\t\t" << values[i] << std::endl;
    **********************************************/

    double minValue = this->getProperty("MinimumLogValue");
    double maxValue = this->getProperty("MaximumLogValue");
    double deltaValue = this->getProperty("LogValueInterval");

    std::string filterdirection = this->getProperty("FilterLogValueByChangingDirection");
    bool filterIncrease;
    bool filterDecrease;
    if (filterdirection.compare("Both") == 0)
    {
      filterIncrease = true;
      filterDecrease = true;
    }
    else if (filterdirection.compare("Increase") == 0)
    {
      filterIncrease = true;
      filterDecrease = false;
    }
    else
    {
      filterIncrease = false;
      filterDecrease = true;
    }

    bool toProcessSingleValueFilter = false;
    if (deltaValue <= 0)
    {
      toProcessSingleValueFilter = true;
    }

    // 2. Generate filters
    if (toProcessSingleValueFilter)
    {
      // a) Generate a filter for a single log value
      processSingleValueFilter(mLog, minValue, maxValue, filterIncrease, filterDecrease);
    }
    else
    {
      // b) Generate filters for a series of log value
      processMultipleValueFilters(mLog, minValue, maxValue, filterIncrease, filterDecrease);
    }

    return;
  }

  void GenerateEventsFilter::processSingleValueFilter(Kernel::TimeSeriesProperty<double>* mlog, double minvalue, double maxvalue,
      bool filterincrease, bool filterdecrease)
  {
    // 1. Validity & value
    if (minvalue > maxvalue)
    {
      g_log.error() << "Error: Input minimum log value " << minvalue <<
          " is larger than maximum log value " << maxvalue << std::endl;
      throw std::invalid_argument("Input minimum value is larger than maximum value");
    }

    double timetolerance = this->getProperty("TimeTolerance");
    int64_t timetolerance_ns = static_cast<int64_t>(timetolerance*m_convertfactor);

    std::string logboundary = this->getProperty("LogBoundary");
    std::transform(logboundary.begin(), logboundary.end(), logboundary.begin(), tolower);

    // 2. Generate filter
    std::vector<Kernel::SplittingInterval> splitters;
    int wsindex = 0;
    makeFilterByValue(mlog, splitters, minvalue, maxvalue, static_cast<double>(timetolerance_ns)*1.0E-9,
        logboundary.compare("centre")==0,
        filterincrease, filterdecrease, mStartTime, mStopTime, wsindex);

    // 3. Add to output
    for (size_t isp = 0; isp < splitters.size(); isp ++)
    {
      mSplitters->addSplitter(splitters[isp]);
    }

    // 4. Add information
    API::TableRow row = mFilterInfoWS->appendRow();
    std::stringstream ss;
    ss << "Log " << mlog->name() << " From " << minvalue << " To " << maxvalue;
    row << 0 << ss.str();

    return;
  }

  /*
   * Generate filters from multiple values
   */
  void GenerateEventsFilter::processMultipleValueFilters(Kernel::TimeSeriesProperty<double>* mlog, double minvalue, double maxvalue,
      bool filterincrease, bool filterdecrease)
  {
    // 1. Read more input
    double valueinterval = this->getProperty("LogValueInterval");
    if (valueinterval <= 0)
      throw std::invalid_argument("Multiple values filter must have LogValueInterval larger than ZERO.");
    double valuetolerance = this->getProperty("LogValueTolerance");
    if (valuetolerance <= 0)
      valuetolerance = 0.5*valueinterval;

    // 2. Build Map
    std::map<size_t, int> indexwsindexmap;
    std::vector<double> valueranges;
    int wsindex = 0;
    size_t index = 0;

    double curvalue = minvalue;
    while (curvalue-valuetolerance < maxvalue)
    {
      indexwsindexmap.insert(std::make_pair(index, wsindex));

      double lowbound = curvalue - valuetolerance;
      double upbound = curvalue + valuetolerance;
      valueranges.push_back(lowbound);
      valueranges.push_back(upbound);

      std::stringstream ss;
      ss << "Log " << mlog->name() << " From " << lowbound << " To " << upbound;
      API::TableRow newrow = mFilterInfoWS->appendRow();
      newrow << wsindex << ss.str();

      curvalue += valueinterval;
      wsindex ++;
      ++index;
    } // ENDWHILE

    /* FIXME Delete After Debugging *
    for (size_t i = 0; i < valueranges.size(); ++i)
    {
      std::cout << "DBOP " << mlog->firstTime().totalNanoseconds() << "\t\t" << valueranges[i] << std::endl;
      std::cout << "DBOP " << mlog->lastTime().totalNanoseconds() << "\t\t" << valueranges[i] << std::endl;
    }
    *********************************/

    // 3. Call
    Kernel::TimeSplitterType splitters;
    std::string logboundary = this->getProperty("LogBoundary");
    std::transform(logboundary.begin(), logboundary.end(), logboundary.begin(), tolower);

    makeMultipleFiltersByValues(mlog, splitters, indexwsindexmap, valueranges,
        logboundary.compare("centre") == 0,
        filterincrease, filterdecrease, mStartTime, mStopTime);

    // 4. Put to SplittersWorkspace
    for (size_t i = 0; i < splitters.size(); i ++)
      mSplitters->addSplitter(splitters[i]);

    return;
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Fill a TimeSplitterType that will filter the events by matching
   * SINGLE log values >= min and < max. Creates SplittingInterval's where
   * times match the log values, and going to index==0.
   *
   * @param split :: Splitter that will be filled.
   * @param min :: min value
   * @param max :: max value
   * @param TimeTolerance :: offset added to times in seconds.
   * @param centre :: Whether the log value time is considered centred or at the beginning.
   * @filterIncrease: as log value increase, and within (min, max), include this range in the filter
   * @filterDecrease: as log value increase, and within (min, max), include this range in the filter
   */
  void GenerateEventsFilter::makeFilterByValue(Kernel::TimeSeriesProperty<double>* mlog,
      Kernel::TimeSplitterType& split, double min, double max, double TimeTolerance, bool centre,
      bool filterIncrease, bool filterDecrease, Kernel::DateAndTime startTime, Kernel::DateAndTime stopTime,
      int wsindex)
  {
    // 1. Do nothing if the log is empty.
    if (mlog->size() == 0)
    {
      g_log.warning() << "There is no entry in this property " << this->name() << std::endl;
      return;
    }

    // 2. Do the rest
    bool lastGood = false;
    bool isGood = false;;
    time_duration tol = DateAndTime::durationFromSeconds( TimeTolerance );
    int numgood = 0;
    DateAndTime lastTime, t;
    DateAndTime start, stop;

    for (int i = 0; i < mlog->size(); i ++)
    {
      lastTime = t;
      //The new entry
      t = mlog->nthTime(i);
      double val = mlog->nthValue(i);

      // A good value?
      if (filterIncrease && filterDecrease)
      {
        // a) Including both sides
        isGood = ((val >= min) && (val < max)) && t >= startTime && t <= stopTime;
      }
      else if (filterIncrease)
      {
        if (i == 0)
          isGood = false;
        else
          isGood = ((val >= min) && (val < max)) && t >= startTime && t <= stopTime && val-mlog->nthValue(i-1) > 0;
      }
      else if (filterDecrease)
      {
        if (i == 0)
          isGood = false;
        else
          isGood = ((val >= min) && (val < max)) && t >= startTime && t <= stopTime && val-mlog->nthValue(i-1) < 0;
      }
      else
      {
        g_log.error() << "Neither increasing nor decreasing is selected.  It is empty!" << std::endl;
      }

      if (isGood)
        numgood++;

      if (isGood != lastGood)
      {
        //We switched from bad to good or good to bad

        if (isGood)
        {
          //Start of a good section
          if (centre)
            start = t - tol;
          else
            start = t;
        }
        else
        {
          //End of the good section
          if (numgood == 1)
          {
            //There was only one point with the value. Use the last time, - the tolerance, as the end time
            if (centre)
            {
              stop = t-tol;
              // stop = lastTime - tol;
            }
            else
            {
              stop = t;
              // stop = lastTime;
            }
            split.push_back( SplittingInterval(start, stop, wsindex) );
          }
          else
          {
            //At least 2 good values. Save the end time
            if (centre)
            {
              stop = t - tol;
              // stop = lastTime - tol;
            }
            else
            {
              stop = t;
              // stop = lastTime;
            }
            split.push_back( SplittingInterval(start, stop, wsindex) );
          }
          //Reset the number of good ones, for next time
          numgood = 0;
        }
        lastGood = isGood;
      }
    } // ENDFOR

    if (numgood > 0)
    {
      //The log ended on "good" so we need to close it using the last time we found
      if (centre)
        stop = t - tol;
      else
        stop = t;
      split.push_back( SplittingInterval(start, stop, wsindex) );
      numgood = 0;
    }

    return;
  }

  //-----------------------------------------------------------------------------------------------
  /**
   * * Fill a TimeSplitterType that will filter the events by matching
   * SINGLE log values >= min and < max. Creates SplittingInterval's where
   * times match the log values, and going to index==0.
   *
   * @param split :: Splitter that will be filled.
   * @param min :: min value
   * @param max :: max value
   * @param centre :: Whether the log value time is considered centred or at the beginning.
   * @filterIncrease: as log value increase, and within (min, max), include this range in the filter
   * @filterDecrease: as log value increase, and within (min, max), include this range in the filter
   */
  void GenerateEventsFilter::makeMultipleFiltersByValues(Kernel::TimeSeriesProperty<double>* mlog,
      Kernel::TimeSplitterType& split, std::map<size_t, int> indexwsindexmap, std::vector<double> valueranges,
      bool centre, bool filterIncrease, bool filterDecrease, Kernel::DateAndTime startTime, Kernel::DateAndTime stopTime)
  {
    // 0. Set up
    double timetolerance = 0.0;
    if (centre)
    {
      timetolerance = this->getProperty("TimeTolerance");
    }
    time_duration tol = DateAndTime::durationFromSeconds( timetolerance );

    // 1. Do nothing if the log is empty.
    if (mlog->size() == 0)
    {
      g_log.warning() << "There is no entry in this property " << mlog->name() << std::endl;
      return;
    }

    // 2. Do the rest
    Kernel::DateAndTime ZeroTime(0);
    int lastindex = -1;
    int currindex = -1;
    DateAndTime lastTime, currTime;
    DateAndTime start, stop;
    double lastValue, currValue;

    for (int i = 0; i < mlog->size(); i ++)
    {
      // a) Init status flags and new entry
      lastTime = currTime;
      lastValue = currValue;
      bool breakloop = false;
      bool completehalf = false;
      bool newsplitter = false;

      currTime = mlog->nthTime(i);
      currValue = mlog->nthValue(i);

      // b) Filter out by time and direction (optional)
      bool intime = false;
      if (currTime < startTime)
      {
        // b1) Too early, do nothing
        completehalf = false;
      }
      else if (currTime > stopTime)
      {
        // b2) Too later.  Put to splitter if half of splitter is done.  But still within range
        breakloop = true;
        stop = currTime;
        if (start.totalNanoseconds() > 0)
        {
          completehalf = true;
        }
      }
      else
      {
        intime = true;
      }

      // c) Filter in time
      if (intime)
      {
        // c1) Direction
        bool correctdir = true;

        if (filterIncrease && filterDecrease)
        {
          // Both direction is fine
          correctdir = true;
        }
        else
        {
          // Filter out one direction
          int direction = 0;
          if ( mlog->nthValue(i)-mlog->nthValue(i-1) > 0)
            direction = 1;
          else
            direction = -1;
          if (filterIncrease && direction > 0)
            correctdir = true;
          else if (filterDecrease && direction < 0)
            correctdir = true;
          else
            correctdir = false;

          // Condition to generate a Splitter (close parenthesis)
          if (!correctdir && start.totalNanoseconds() > 0)
          {
            stop = currTime;
            completehalf = true;
          }
        } // END-IF-ELSE: Direction

        // c2) See whether this value falls into any range
        if (correctdir)
        {
          size_t index = searchValue(valueranges, currValue);
          std::cout << "DBOP Log Index " << i << " Data Range Index = " << index << "  WS Index = " << indexwsindexmap[index/2] << std::endl;

          if (index%2 == 0)
          {
            // c1) Falls in the interval
            currindex = indexwsindexmap[index/2];

            if (currindex != lastindex && start.totalNanoseconds() == 0)
            {
              // i.   A new region!
              newsplitter = true;
            }
            else if (currindex != lastindex && start.totalNanoseconds() > 0)
            {
              // ii.  Time to close a region and new a region
              stop = currTime;
              completehalf = true;
              newsplitter = true;
            }
            else if (currindex == lastindex && start.totalNanoseconds() > 0)
            {
              // iii. Still in the same zone
              ;
            }
            else
            {
              // iv.  It is impossible
              throw std::runtime_error("Impossible to have currindex == lastindex, while start is not init");
            }
          }
          else
          {
            if (start.totalNanoseconds() > 0)
            {
              stop = currTime;
              completehalf = true;
            }

            // c2) Fall out of interval
            std::cout << "DBOP Log Index " << i << "  Falls Out b/c value range... " << std::endl;
          }
        } // ENDIF NO breakloop AND Correction Direction
        else
        {
          std::cout << "DBOP Log Index " << i << " Falls out b/c out of wrong direction" << std::endl;
        }
      }
      else
      {
        std::cout << "DBOP Log Index " << i << "  Falls Out b/c out of time range... " << std::endl;
      }

      // d) Create Splitter
      if (completehalf)
      {
        if (centre)
        {
          split.push_back( SplittingInterval(start-tol, stop-tol, lastindex) );
        }
        else
        {
          split.push_back( SplittingInterval(start, stop, lastindex) );
        }
        std::cout << "DBOP ...  Add splitter " << split.size()-1 << ":  " << start.totalNanoseconds() << ", "
            << stop.totalNanoseconds() << " ... WSIndex = " << lastindex << std::endl;

        // reset
        start = ZeroTime;
      }

      // e) Start new splitter: have to be here due to start cannot be updated before a possible splitter generated
      if (newsplitter)
      {
        start = currTime;
      }

      // f) Break
      if (breakloop)
        break;

      // e) Update loop variable
      lastindex = currindex;
    } // For each log value

    return;

  }

  /*
   * Do a binary search in the following list
   * Warning: if the vector is not sorted, the error will happen.
   * This algorithm won't guarantee for it
   *
   * return:  if value is out of range, then return datarange.size() + 1
   */
  size_t GenerateEventsFilter::searchValue(std::vector<double> dataranges, double value)
  {
    size_t outrange = dataranges.size()+1;

    // std::cout << "DB450  Search Value " << value << std::endl;

    // 1. Extreme case
    if (value < dataranges[0] || value > dataranges.back())
      return outrange;
    if (dataranges.size() == 0)
      return outrange;

    // 2. Binary search
    bool found = false;
    size_t start = 0;
    size_t stop = dataranges.size()-1;

    while (!found)
    {
      if (start == stop || start+1 == stop)
      {
        // a) Found
        if (value == dataranges[stop])
        {
          // std::cout << "DB450  Found @ A " << dataranges[stop] << "  Index = " << stop << std::endl;
          return stop;
        }
        else
        {
          // std::cout << "DB450  Found @ B " << dataranges[start] << "  Index = " << start << std::endl;
          return start;
        }
      }

      size_t mid = (start+stop)/2;
      if (value < dataranges[mid])
      {
        stop = mid;
      }
      else
      {
        start = mid;
      }
    }

    return 0;
  }

} // namespace Mantid
} // namespace Algorithms
