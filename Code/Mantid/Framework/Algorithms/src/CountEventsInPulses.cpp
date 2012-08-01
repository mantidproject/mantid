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
      /// Input workspace
      this->declareProperty(new API::WorkspaceProperty<DataObjects::EventWorkspace>("InputWorkspace", "", Direction::Input),
                            "Input EventWorkspace to count events");

      /// Output workspace
      this->declareProperty(new API::WorkspaceProperty<DataObjects::EventWorkspace>("OutputWorkspace", "", Direction::Output),
                            "Output EventWorkspace for events count along run time.");

      /// Tolerance (resolution)
      this->declareProperty("Tolerance",  0.16, "Tolerance of events compressed in unit of second. ");

      /// Sum spectra or not
      this->declareProperty("SumSpectra", true, "Whether to sum up all spectra.");

      /// Run in parallel or not
      this->declareProperty("Parallel", true, "Make the code work in parallel");

      return;
  }

  /*
   * Execute main body
   */
  void CountEventsInPulses::exec()
  {
      // 1. Get input
      inpWS = this->getProperty("InputWorkspace");
      mTolerance = this->getProperty("Tolerance");
      mSumSpectra = this->getProperty("SumSpectra");

      // 2. Some survey of
      Kernel::TimeSeriesProperty<double>* protonchargelog =
              dynamic_cast<Kernel::TimeSeriesProperty<double>* >(inpWS->run().getProperty("proton_charge"));
      mTimesInSecond = protonchargelog->timesAsVectorSeconds();
      mTimes = protonchargelog->timesAsVector();

      // b) Check proton charge log to examine the pulses
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
      g_log.notice() << "For Each Pulse: Delta T = " << mPulseLength << "  standard deviation = " << stddev << std::endl;

      // 3. Setup for parallelization
      bool useparallel = this->getProperty("Parallel");
      int nummaxcores = 1;
      if (useparallel)
          nummaxcores = PARALLEL_GET_MAX_THREADS;
      PARALLEL_SET_NUM_THREADS(nummaxcores);

      // 4. Count!
      DataObjects::EventWorkspace_sptr outputWS = this->countInEventWorkspace();

      // 5. Set output
      // a) Sort
      API::Algorithm_sptr sort = this->createSubAlgorithm("SortEvents", 0.7, 0.8, true);
      sort->initialize();
      sort->setProperty("InputWorkspace", outputWS);
      sort->setPropertyValue("SortBy", "X Value");
      sort->execute();

      // b) Output
      this->setProperty("OutputWorkspace", outputWS);

      // 6. Rebin
      std::stringstream ss;
      ss << 0 << ", " << mTolerance << ", " << outputWS->readX(0).back();
      std::string binparam = ss.str();

      g_log.debug() << "Binning parameter = " << binparam << std::endl;

      API::Algorithm_sptr rebin = this->createSubAlgorithm("Rebin", 0.8, 0.9, true);
      rebin->initialize();
      rebin->setProperty("InputWorkspace", outputWS);
      rebin->setProperty("OutputWorkspace", outputWS);
      rebin->setProperty("Params", binparam);
      rebin->setProperty("PreserveEvents", true);

      rebin->execute();

      // 5. Sum spectrum?
      if (mSumSpectra)
      {
          std::cout << "About to sort events! " << std::endl;
          API::Algorithm_sptr sumspec = this->createSubAlgorithm("SumSpectra", 0.9, 1.0, true);
          sumspec->initialize();
          sumspec->setProperty("InputWorkspace", outputWS);
          sumspec->setProperty("OutputWorkspace", "TempWS");

          bool sumsuccessful = sumspec->execute();
          if (!sumsuccessful)
          {
              g_log.error() << "Sum Spectra Failed!" << std::endl;
          }
          else
          {
              API::MatrixWorkspace_sptr testws = sumspec->getProperty("OutputWorkspace");
              if (!testws)
              {
                  g_log.error() << "SumSpectra failed as the output is not a MatrixWorkspace. " << std::endl;
              }
              else
              {
                  outputWS = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(testws);
                  this->setProperty("OutputWorkspace", outputWS);
              }
          }
      }

      return;
  }

  /*
   * Count events and output in event workspace
   */
  DataObjects::EventWorkspace_sptr CountEventsInPulses::countInEventWorkspace()
  {
      // 1. Create and output EventWorkspace
      DataObjects::EventWorkspace_sptr outWS = this->createEventWorkspace(inpWS);

      // 2. Switch Event's TOF and Pulse Time
      this->convertEvents(outWS);

      return outWS;
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
      // FIXME NO SURE WHETHER THIS UNIT WORKS OR NOT
      outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("Time");
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

      double margin_sec = mPulseLength*0.01;
      g_log.information() << "Pulse length = " << mPulseLength << " (sec).  Margine = " << margin_sec
                          << " For safe binning. " << std::endl;

      PARALLEL_FOR_NO_WSP_CHECK_FIRSTPRIVATE2(tofmin, tofmax)
      for (int iws = 0; iws < static_cast<int>(inpWS->getNumberHistograms()); iws++)
      {
          DataObjects::EventList realevents = inpWS->getEventList(iws);
          DataObjects::EventList* fakeevents = outWS->getEventListPtr(iws);

          for (size_t ie = 0; ie < realevents.getNumberEvents(); ie ++)
          {
              DataObjects::WeightedEvent event = realevents.getEvent(ie);
              // Swap TOF and pulse time and add to new event list
              // a) TOF (ms) -> Pulse Time
              double oldtof = event.tof();
              int64_t oldtofns = int64_t(oldtof*1000);
              Kernel::DateAndTime newpulsetime(oldtofns);

              // b) Pulse Time (ns) -> TOF (ms) -> second
              // double newtof = static_cast<double>(event.pulseTime().totalNanoseconds()-runstarttime)*1.0E-3+event.tof();
              int64_t oldpulsetime = event.pulseTime().totalNanoseconds()-runstarttime;
              double newtofinsecond = double(oldpulsetime)*1.0E-9 + margin_sec;

              // DataObjects::WeightedEvent fakeevent(newtofinsecond, newpulsetime, 1.0, 1.0);
              // DataObjects::WeightedEventNoTime fakeevent(newtofinsecond);
              DataObjects::TofEvent fakeevent(newtofinsecond, newpulsetime);

              // size_t numevents1 = fakeevents->getNumberEvents();
              fakeevents->addEventQuickly(fakeevent);
              // size_t numevents2 = fakeevents->getNumberEvents();
              /*
              if (numevents1 != numevents2-1)
              {
                  std::cout << "Add event but no change in number of events.  Before " << numevents1 <<
                            " After " << numevents2 << std::endl;
                  std::cout << fakeevent.tof() << std::endl;
              }
              else
              {
                  std::cout << "Before " << numevents1 << " After " << numevents2 << std::endl;
              }
              throw std::runtime_error("Debug Stop");
              */

              // Get statistic
              if (newtofinsecond < tofmin)
                  tofmin = newtofinsecond;
              else if (newtofinsecond > tofmax)
                  tofmax = newtofinsecond;


          } // ENDFOR events

          // std::cout << "Spectrum " << iws << " Number of events: " << realevents.getNumberEvents() << " (Faked) TOF Range: "
          //      << tofmin << ", " << tofmax << std::endl;

          PARALLEL_CRITICAL(sumtof)
          {
              if (tofmin < tofminall)
                  tofminall = tofmin;
              else if (tofmax > tofmaxall)
                  tofmaxall = tofmax;
          }
      } // END FOR: Per Spectrum

      g_log.debug() << "DBx505 Input Events = " << inpWS->getNumberEvents() << "; Output Events = "
                    << outWS->getNumberEvents() << std::endl;

      // 3. Add a dummy histogram: create a default X-vector for histogramming, with just 2 bins.
      Kernel::cow_ptr<MantidVec> axis;
      MantidVec& xRef = axis.access();
      xRef.resize(2);
      if (tofminall > 1.0E-6)
          tofminall = tofminall - 1.0E-6;
      xRef[0] = tofminall; //Just to make sure the bins hold it all
      xRef[1] = tofmaxall + mTolerance;
      outWS->setAllX(axis);

      g_log.debug() << "DBx506 Input Events = " << inpWS->getNumberEvents() << "; Output Events = "
                    << outWS->getNumberEvents() << std::endl;

      return;
  }

} // namespace Mantid
} // namespace Algorithms
