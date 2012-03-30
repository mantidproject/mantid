#include "MantidAlgorithms/CountEventsInPulses.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/Events.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/UnitFactory.h"

#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{

  DECLARE_ALGORITHM(CountEventsInPulses)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  CountEventsInPulses::CountEventsInPulses()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CountEventsInPulses::~CountEventsInPulses()
  {
  }

  void CountEventsInPulses::initDocs()
  {
    return;
  }

  void CountEventsInPulses::init()
  {
    this->declareProperty(new API::WorkspaceProperty<DataObjects::EventWorkspace>("InputWorkspace", "", Direction::Input),
        "Input EventWorkspace to count events");
    this->declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace", "", Direction::Output),
        "Output Workspace2D for events count along run time.");

    this->declareProperty("PulsesPerBin", 1000, "Number of pulses per bin.");
    this->declareProperty("SumSpectra", true, "Whether to sum up all spectra.");

    std::vector<std::string> timeunit;
    timeunit.push_back("second");
    timeunit.push_back("millisecond");
    timeunit.push_back("microsecond");
    this->declareProperty("Unit", "second", boost::make_shared<Kernel::StringListValidator>(timeunit),
        "Unit of time in output Workspace2D.");

    this->declareProperty("PreserveEvents", false,
        "Option to preserve the events in the output workspace.");

    return;
  }

  /*
   * Execute main body
   */
  void CountEventsInPulses::exec()
  {
    // 1. Get input
    inpWS = this->getProperty("InputWorkspace");
    int binsize = this->getProperty("PulsesPerBin");
    mSumSpectra = this->getProperty("SumSpectra");
    std::string timeunit = this->getProperty("Unit");

    if (binsize <= 0)
    {
      g_log.error() << "Input bin size = " << binsize << "  is not allowed to be less than 1" << std::endl;
      throw std::invalid_argument("Input bin size is not allowed");
    }
    mBinSize = static_cast<size_t>(binsize);

    mUnitFactor = 1.0;
    if (timeunit.compare("millisecond") == 0)
      mUnitFactor = 1000.0;
    else if (timeunit.compare("microsecond")==0)
      mUnitFactor = 1.0E6;

    // 2. Some survey of
    Kernel::TimeSeriesProperty<double>* protonchargelog =
        dynamic_cast<Kernel::TimeSeriesProperty<double>* >(inpWS->run().getProperty("proton_charge"));
    mTimesInSecond = protonchargelog->timesAsVectorSeconds();
    mTimes = protonchargelog->timesAsVector();

    double dt1 = 0.0;
    double dt2 = 0.0;
    for (size_t i = 1; i < mTimesInSecond.size(); i ++)
    {
      double dt = mTimesInSecond[i]-mTimesInSecond[i-1];
      dt1 += dt;
      dt2 += dt*dt;
      if (dt > 1.5/60.0)
      {
        g_log.error() << "dt = " << dt << "  : some pulse must be skipped" << std::endl;
      }
    }
    mPulseLength = dt1/static_cast<double>(mTimesInSecond.size());
    double stddev = sqrt(dt2/static_cast<double>(mTimesInSecond.size())-mPulseLength*mPulseLength);
    g_log.notice() << "Delta T = " << mPulseLength << "  standard deviation = " << stddev << std::endl;

    // 3. Count!
    bool preserveevents = this->getProperty("PreserveEvents");
    if (!preserveevents)
    {
      this->countInWorkspace2D();
    }
    else
    {
      this->countInEventWorkspace();
    }

    return;
  }

  /*
   * Count events and output in event workspace
   */
  void CountEventsInPulses::countInEventWorkspace()
  {
    // 1. Create and output EventWorkspace
    DataObjects::EventWorkspace_sptr outWS = this->createEventWorkspace(inpWS);
    API::MatrixWorkspace_sptr matWS = boost::dynamic_pointer_cast<API::MatrixWorkspace>(outWS);
    if (!matWS)
    {
      throw std::runtime_error("Output EventWorkspace cannot be casted to MatrixWorkspace.");
    }
    this->setProperty("OutputWorkspace", matWS);

    // 2. Switch Event's TOF and Pulse Time
    this->convertEvents(outWS);

    // 3. Rebin on pulses/bin
    double binstep = mPulseLength*static_cast<double>(mBinSize)*1.0E6;
    double t0 = mTimesInSecond[0]*1.0E6 + binstep*0.5;
    double tf = mTimesInSecond.back()*1.0E6 - binstep*0.5;
    std::stringstream ss;
    ss << t0 << "," << binstep << ", " << tf;
    std::string rebinpar = ss.str();

    IAlgorithm_sptr rebin = createSubAlgorithm("Rebin");
    g_log.debug() << "Rebin Workspace = " << outWS->getName() << " and parameter = " << rebinpar << std::endl;
    rebin->setProperty("InputWorkspace", outWS);
    rebin->setProperty("OutputWorkspace", outWS);
    rebin->setProperty("Params", rebinpar);
    rebin->setProperty("PreserveEvents", true);

    bool rebinsuccess = rebin->execute();
    if (!rebinsuccess)
      g_log.error() << "Rebin EventWorkspace " << outWS->getName() << " by " << rebinpar << "  failed!" << std::endl;

    return;
  }

  /*
   * Count events and output in workspace2D
   */
  void CountEventsInPulses::countInWorkspace2D()
  {
    // 3. Create and set output Workspace
    DataObjects::Workspace2D_sptr outWS = DataObjects::Workspace2D_sptr(new DataObjects::Workspace2D);
    size_t outsize = mTimesInSecond.size()/mBinSize;
    if (mTimesInSecond.size()%mBinSize != 0)
      outsize ++;

    if (mSumSpectra)
    {
      // a) Output is 1 spectrum
      outWS->initialize(1, outsize, outsize-1);
    }
    else
    {
      // b) Output is N spectra
      outWS->initialize(inpWS->getNumberHistograms(), outsize, outsize-1);
    }

    API::MatrixWorkspace_sptr matWS =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(outWS);
    this->setProperty("OutputWorkspace", outWS);

    // 4. Set X and Reset Y
    g_log.debug() << "Setting X values and Resetting Y values" << std::endl;
    std::vector<double> tempx;
    std::vector<double> tempy;
    tempx.reserve(outsize);
    tempy.reserve(outsize-1);
    mBinTimes.reserve(outsize);
    for (size_t ix = 0; ix < outsize; ix ++)
    {
      size_t tindex = ix*mBinSize;
      if (tindex >= mTimesInSecond.size())
        tindex = mTimesInSecond.size()-1;
      tempx.push_back(mTimesInSecond[tindex]*mUnitFactor);
      mBinTimes.push_back(mTimes[tindex]);

    }
    for (size_t iy = 0; iy < outsize-1; iy ++)
      tempy.push_back(0);
    for (size_t ih = 0; ih < outWS->getNumberHistograms(); ih ++)
    {
      MantidVec& vecX = outWS->dataX(ih);
      std::copy(tempx.begin(), tempx.end(), vecX.begin());
      MantidVec& vecY = outWS->dataY(ih);
      std::copy(tempy.begin(), tempy.end(), vecY.begin());
    }

    // 5. Sort Events
    IAlgorithm_sptr sortalg = createSubAlgorithm("SortEvents");
    sortalg->setPropertyValue("InputWorkspace", inpWS->name());
    sortalg->setProperty("SortBy", "Pulse Time");
    bool sortsuccess = sortalg->execute();
    if (!sortsuccess)
      throw std::runtime_error("Failed to sort event workspace by pulse time");

    // 6. Sum
    for (size_t ih = 0; ih < inpWS->getNumberHistograms(); ih ++)
    {
      if (mSumSpectra)
      {
        // a. Get hold of the vector to sum up to
        MantidVec& vecX = outWS->dataX(0);
        MantidVec& vecY = outWS->dataY(0);
        // b. Sum over
        this->countEventsOnSpectrumParallel(ih, vecX, vecY);
      }
      else
      {
        // a. Get hold of the vector to sum up to
        MantidVec& vecX = outWS->dataX(ih);
        MantidVec& vecY = outWS->dataY(ih);
        // b. Sum over
        this->countEventsOnSpectrumParallel(ih, vecX, vecY);
      }
    }

    return;
  }

  /*
   * Count events for a single spectrum
   */
  void CountEventsInPulses::countEventsOnSpectrum(size_t wsindex, MantidVec& vecX, MantidVec& vecY)
  {
    DataObjects::EventList events = inpWS->getEventList(wsindex);

    bool dobinsearch = true;
    size_t currindex = 0;

    for (size_t ie = 0; ie < events.getNumberEvents(); ie ++)
    {
      DataObjects::WeightedEvent event = events.getEvent(ie);
      Kernel::DateAndTime pulsetime = event.pulseTime();

      // a) Do limited linear search
      if (!dobinsearch)
      {
        if (pulsetime < mBinTimes[currindex+1] || currindex == vecY.size()-1)
        {
          // i.   event in the same pulse as last event OR already in the end of events
          vecY[currindex] ++;
        }
        else if (pulsetime < mBinTimes[currindex+2])
        {
          // ii.  event in the next pulse
          currindex ++;
          vecY[currindex] ++;
        }
        else
        {
          // iii. use binary search to determine
          dobinsearch = true;
        }
      }

      // b) do binary search if limited linear search failed
      if (dobinsearch)
      {
        std::vector<Kernel::DateAndTime>::iterator it = std::lower_bound(mTimes.begin()+currindex, mTimes.end(), pulsetime);
        size_t tindex = size_t(it-mTimes.begin());
        if (*it != pulsetime && it!=mTimes.begin())
        {
          tindex --;
        }
        currindex = tindex/mBinSize;
        if (currindex >= vecX.size()-1)
          currindex = vecX.size()-2;

        vecY[currindex]++;
      }

    } // FOR Events

    return;
  }

  /*
   * Count events for a single spectrum In parallel
   */
  void CountEventsInPulses::countEventsOnSpectrumParallel(size_t wsindex, MantidVec& vecX, MantidVec& vecY)
  {
    DataObjects::EventList events = inpWS->getEventList(wsindex);

    bool dobinsearch = true;
    size_t currindex = 0;

    std::vector<double> sum;
    sum.reserve(100);
    for (size_t i = 0; i < 100; i ++)
      sum.push_back(0);

    size_t numevents = events.getNumberEvents();
    size_t counts = 0;
    // PARALLEL_FOR_NO_WSP_CHECK()

    // #pragma omp parallel for firstprivate(currindex, dobinsearch)

    //PARALLEL_FOR_NOWS_CHECK_FIRSTPRIVATE2(currindex, dobinsearch)
    for (int ie = 0; ie < static_cast<int>(numevents); ie ++)
    {
      PARALLEL_START_INTERUPT_REGION

      PARALLEL_ATOMIC
      counts ++;

      DataObjects::WeightedEvent event = events.getEvent(ie);
      Kernel::DateAndTime pulsetime = event.pulseTime();

      // a) Do limited linear search
      if (!dobinsearch)
      {
        if (pulsetime < mBinTimes[currindex+1] || currindex == vecY.size()-1)
        {
          // i.   event in the same pulse as last event OR already in the end of events
          PARALLEL_ATOMIC
          vecY[currindex] ++;
        }
        else if (pulsetime < mBinTimes[currindex+2])
        {
          // ii.  event in the next pulse
          currindex ++;
          PARALLEL_ATOMIC
          vecY[currindex] ++;
        }
        else
        {
          // iii. use binary search to determine
          dobinsearch = true;
        }
      }

      // b) do binary search if limited linear search failed
      if (dobinsearch)
      {
        std::vector<Kernel::DateAndTime>::iterator it = std::lower_bound(mTimes.begin()+currindex, mTimes.end(), pulsetime);
        size_t tindex = size_t(it-mTimes.begin());
        if (*it != pulsetime && it!=mTimes.begin())
        {
          tindex --;
        }
        currindex = tindex/mBinSize;
        if (currindex >= vecX.size()-1)
          currindex = vecX.size()-2;

        PARALLEL_ATOMIC
        vecY[currindex]++;
      }

      PARALLEL_END_INTERUPT_REGION
    } // FOR Events
    PARALLEL_CHECK_INTERUPT_REGION

    // g_log.notice() << "Counts = " << counts << ",  Events = " << numevents << std::endl;

    return;
  }

  /*
   * Create an output EventWorkspace w/o any events
   */
  DataObjects::EventWorkspace_sptr CountEventsInPulses::createEventWorkspace(DataObjects::EventWorkspace_const_sptr parentws)
  {
    // 1. Initialize:use dummy numbers for arguments, for event workspace it doesn't matter
    DataObjects::EventWorkspace_sptr outputWS =
        DataObjects::EventWorkspace_sptr(new DataObjects::EventWorkspace());
    outputWS->setName("NewEventWorkspace");
    outputWS->initialize(1,1,1);

    // 2. Set the units
    outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    outputWS->setYUnit("Counts");
    outputWS->setTitle("EmptyTitle");

    // 3. Add the run_start property:
    int runnumber = parentws->getRunNumber();
    outputWS->mutableRun().addProperty("run_number", runnumber);

    std::string runstartstr = parentws->run().getProperty("run_start")->value();
    outputWS->mutableRun().addProperty("run_start", runstartstr);

    // 4. Instrument
    IAlgorithm_sptr loadInst= createSubAlgorithm("LoadInstrument");
    // Now execute the sub-algorithm. Catch and log any error, but don't stop.
    loadInst->setPropertyValue("InstrumentName", parentws->getInstrument()->getName());
    loadInst->setProperty<MatrixWorkspace_sptr> ("Workspace", outputWS);
    loadInst->setProperty("RewriteSpectraMap", true);
    loadInst->executeAsSubAlg();
    // Populate the instrument parameters in this workspace - this works around a bug
    outputWS->populateInstrumentParameters();

    // 5. ??? Is pixel mapping file essential???

    // 6. Build spectrum and event list
    // a) We want to pad out empty pixels.
    detid2det_map detector_map;
    outputWS->getInstrument()->getDetectors(detector_map);

    g_log.debug() << "VZ: 6a) detector map size = " << detector_map.size() << std::endl;

    // b) determine maximum pixel id
    detid2det_map::iterator it;
    detid_t detid_max = 0; // seems like a safe lower bound
    for (it = detector_map.begin(); it != detector_map.end(); ++it)
      if (it->first > detid_max)
        detid_max = it->first;

    // c) Pad all the pixels and Set to zero
    std::vector<std::size_t> pixel_to_wkspindex;
    pixel_to_wkspindex.reserve(detid_max+1); //starting at zero up to and including detid_max
    pixel_to_wkspindex.assign(detid_max+1, 0);
    size_t workspaceIndex = 0;
    for (it = detector_map.begin(); it != detector_map.end(); ++it)
    {
      if (!it->second->isMonitor())
      {
        pixel_to_wkspindex[it->first] = workspaceIndex;
        DataObjects::EventList & spec = outputWS->getOrAddEventList(workspaceIndex);
        spec.addDetectorID(it->first);
        // Start the spectrum number at 1
        spec.setSpectrumNo(specid_t(workspaceIndex+1));
        workspaceIndex += 1;
      }
    }
    outputWS->doneAddingEventLists();

    // Clear
    pixel_to_wkspindex.clear();

    g_log.debug() << "VZ (End of createEventWorkspace): Total spectrum number = " << outputWS->getNumberHistograms() << std::endl;

    return outputWS;

  }

  /*
   * Convert events to "fake" events as counts in outWS
   */
  void CountEventsInPulses::convertEvents(DataObjects::EventWorkspace_sptr outWS)
  {
    // 1. Get run_start time
    std::string runstartstr = inpWS->run().getProperty("run_start")->value();
    Kernel::DateAndTime runstart(runstartstr);
    int64_t runstarttime = runstart.totalNanoseconds();

    // 2. Convert TOF and add to new event workspace
    double tofmin = 1.0E20;
    double tofmax = -1;
    double tofminall = 1.0E20;
    double tofmaxall = -1;

    //PARALLEL_FOR_NOWS_CHECK_FIRSTPRIVATE2(tofmin, tofmax)
    for (int iws = 0; iws < static_cast<int>(inpWS->getNumberHistograms()); iws++)
    {
      DataObjects::EventList realevents = inpWS->getEventList(iws);
      DataObjects::EventList* fakeevents = outWS->getEventListPtr(iws);

      for (size_t ie = 0; ie < realevents.getNumberEvents(); ie ++)
      {
        DataObjects::WeightedEvent event = realevents.getEvent(ie);
        // Swap TOF and pulse time and add to new event list
        double newtof = static_cast<double>(event.pulseTime().totalNanoseconds()-runstarttime)*1.0E-3+event.tof();
        DataObjects::TofEvent fakeevent(newtof);

        fakeevents->addEventQuickly(fakeevent);

        // Get statistic
        if (newtof < tofmin)
          tofmin = newtof;
        else if (newtof > tofmax)
          tofmax = newtof;
      } // ENDFOR events

      PARALLEL_CRITICAL(sumtof)
      {
        if (tofmin < tofminall)
          tofminall = tofmin;
        else if (tofmax > tofmaxall)
          tofmaxall = tofmax;
      }

    } // END FOR

    // 3. Add a dummy histogram: create a default X-vector for histogramming, with just 2 bins.
    Kernel::cow_ptr<MantidVec> axis;
    MantidVec& xRef = axis.access();
    xRef.resize(2);
    xRef[0] = tofminall - 1; //Just to make sure the bins hold it all
    xRef[1] = tofmaxall + 1;
    outWS->setAllX(axis);

    return;
  }

} // namespace Mantid
} // namespace Algorithms
