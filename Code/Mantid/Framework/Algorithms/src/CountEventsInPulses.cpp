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
#include "MantidAPI/WorkspaceFactory.h"

#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(CountEventsInPulses)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
CountEventsInPulses::CountEventsInPulses() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
CountEventsInPulses::~CountEventsInPulses() {}

void CountEventsInPulses::init() {
  // Input workspace
  this->declareProperty(new API::WorkspaceProperty<DataObjects::EventWorkspace>(
                            "InputWorkspace", "", Direction::Input),
                        "Input EventWorkspace to count events");

  // Output workspace
  this->declareProperty(
      new API::WorkspaceProperty<DataObjects::EventWorkspace>(
          "OutputWorkspace", "", Direction::Output),
      "Output EventWorkspace for events count along run time.");

  // Bin size
  this->declareProperty("BinSize", EMPTY_DBL(),
                        "Bin size for output workspace in unit of time.  Left "
                        "empty will use default equal to length of 1 pulse.");

  // Tolerance (resolution)
  this->declareProperty("Tolerance", EMPTY_DBL(),
                        "Tolerance of events compressed in unit of second.  "
                        "Left empty disables.");

  // Sum spectra or not
  this->declareProperty("SumSpectra", true, "Whether to sum up all spectra.");

  // Run in parallel or not
  this->declareProperty("Parallel", true, "Make the code work in parallel");

  return;
}

/** Execute main body
 */
void CountEventsInPulses::exec() {
  // 1. Get input
  inpWS = this->getProperty("InputWorkspace");

  mBinSize = this->getProperty("BinSize");
  bool usedefaultbinsize = false;
  if (mBinSize == EMPTY_DBL()) {
    usedefaultbinsize = true;
  }

  mSumSpectra = this->getProperty("SumSpectra");

  double tolerance = this->getProperty("Tolerance");
  bool compressevent = true;
  if (tolerance == EMPTY_DBL()) {
    compressevent = false;
  }

  // 2. Some survey of
  Kernel::TimeSeriesProperty<double> *protonchargelog =
      dynamic_cast<Kernel::TimeSeriesProperty<double> *>(
          inpWS->run().getProperty("proton_charge"));
  mTimesInSecond = protonchargelog->timesAsVectorSeconds();

  // b) Check proton charge log to examine the pulses
  double dt1 = 0.0;
  double dt2 = 0.0;
  for (size_t i = 1; i < mTimesInSecond.size(); i++) {
    double dt = mTimesInSecond[i] - mTimesInSecond[i - 1];
    dt1 += dt;
    dt2 += dt * dt;
    if (dt > 1.5 / 60.0) {
      g_log.warning() << "From proton charge, delta T = " << dt
                      << "  : some pulse might be skipped" << std::endl;
    }
  }
  mPulseLength = dt1 / static_cast<double>(mTimesInSecond.size());
  double stddev = sqrt(dt2 / static_cast<double>(mTimesInSecond.size()) -
                       mPulseLength * mPulseLength);
  g_log.notice() << "For Each Pulse: Delta T = " << mPulseLength
                 << "  Standard deviation = " << stddev << std::endl;

  if (usedefaultbinsize) {
    mBinSize = mPulseLength;
  }

  // 3. Setup for parallelization
  bool useparallel = this->getProperty("Parallel");
  int nummaxcores = 1;
  if (useparallel)
    nummaxcores = PARALLEL_GET_MAX_THREADS;

  // 4.  Create and output EventWorkspace
  DataObjects::EventWorkspace_sptr outputWS =
      this->createEventWorkspace(inpWS, mSumSpectra);

  // 5. Switch Event's TOF and Pulse Time
  // Set the maximum cores to user
  PARALLEL_SET_NUM_THREADS(nummaxcores);

  this->convertEvents(outputWS, mSumSpectra);

  // Set the maximum cores back
  PARALLEL_SET_NUM_THREADS(PARALLEL_GET_MAX_THREADS);

  // 6. Rebin
  rebin(outputWS);

  // 7. Compress events
  if (compressevent) {
    outputWS = compressEvents(outputWS, tolerance);
  }

  // 8. Output
  this->setProperty("OutputWorkspace", outputWS);

  return;
}

/**
 * Create an output EventWorkspace w/o any events
 */
DataObjects::EventWorkspace_sptr CountEventsInPulses::createEventWorkspace(
    DataObjects::EventWorkspace_const_sptr parentws, bool sumspectrum) {
  size_t numspec;
  bool diffsize;
  if (sumspectrum) {
    numspec = 1;
    diffsize = true;
  } else {
    numspec = parentws->getNumberHistograms();
    diffsize = false;
  }

  DataObjects::EventWorkspace_sptr outputWS =
      boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
          API::WorkspaceFactory::Instance().create("EventWorkspace", numspec, 1,
                                                   1));
  API::WorkspaceFactory::Instance().initializeFromParent(parentws, outputWS,
                                                         diffsize);

  outputWS->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("Time");

  return outputWS;
}

/** Rebin output workspace
  */
void CountEventsInPulses::rebin(DataObjects::EventWorkspace_sptr outputWS) {
  const double xmin = outputWS->getTofMin();
  const double xmax = outputWS->getTofMax();
  if (xmax <= xmin) {
    std::stringstream ss;
    ss << "Tof_max " << xmax << " is less than Tof_min " << xmin;
    throw std::runtime_error(ss.str());
  }

  std::stringstream ss;
  ss << xmin << ", " << mPulseLength << ", " << xmax;
  std::string binparam = ss.str();

  g_log.debug() << "Binning parameter = " << binparam << std::endl;

  API::Algorithm_sptr rebin =
      this->createChildAlgorithm("Rebin", 0.8, 0.9, true);
  rebin->initialize();
  rebin->setProperty("InputWorkspace", outputWS);
  rebin->setProperty("OutputWorkspace", outputWS);
  rebin->setProperty("Params", binparam);
  rebin->setProperty("PreserveEvents", true);

  bool success = rebin->execute();

  if (!success) {
    g_log.warning() << "Rebin output event workspace failed! " << std::endl;
  }

  return;
}

/** Compress event
  */
DataObjects::EventWorkspace_sptr
CountEventsInPulses::compressEvents(DataObjects::EventWorkspace_sptr inputws,
                                    double tolerance) {
  API::Algorithm_sptr alg =
      this->createChildAlgorithm("CompressEvents", 0.9, 1.0, true);
  alg->initialize();

  alg->setProperty("InputWorkspace", inputws);
  alg->setProperty("OutputWorkspace", "TempWS");
  alg->setProperty("Tolerance", tolerance);

  bool successful = alg->execute();

  DataObjects::EventWorkspace_sptr outputws;

  if (!successful) {
    // Failed
    outputws = inputws;
    g_log.warning() << "CompressEvents() Failed!" << std::endl;
  } else {
    // Successful
    outputws = alg->getProperty("OutputWorkspace");
    if (!outputws) {
      g_log.error()
          << "CompressEvents failed as the output is not a MatrixWorkspace. "
          << std::endl;
      throw std::runtime_error(
          "SumSpectra failed as the output is not a MatrixWorkspace.");
    }

    /*
    API::MatrixWorkspace_sptr testws = alg->getProperty("OutputWorkspace");
    if (!testws)
    {
        g_log.error() << "CompressEvents failed as the output is not a
    MatrixWorkspace. " << std::endl;
        throw std::runtime_error("SumSpectra failed as the output is not a
    MatrixWorkspace.");
    }
    else
    {
        outputws =
    boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(testws);
    }
   */
  }

  return outputws;
}

/** Convert events to "fake" events as counts in outWS
 */
void CountEventsInPulses::convertEvents(DataObjects::EventWorkspace_sptr outWS,
                                        bool sumspectra) {
  // 1. Get run_start time
  std::string runstartstr = inpWS->run().getProperty("run_start")->value();
  Kernel::DateAndTime runstart(runstartstr);
  int64_t runstarttime = runstart.totalNanoseconds();

  // 2. Convert TOF and add to new event workspace
  double margin_sec = mPulseLength * 0.01;
  g_log.information() << "Pulse length = " << mPulseLength
                      << " (sec).  Margine = " << margin_sec
                      << " For safe binning. " << std::endl;

  PARALLEL_FOR2(inpWS, outWS)
  for (int wsindex = 0;
       wsindex < static_cast<int>(inpWS->getNumberHistograms()); wsindex++) {

    DataObjects::EventList realevents = inpWS->getEventList(wsindex);
    DataObjects::EventList *fakeevents;
    if (sumspectra) {
      fakeevents = outWS->getEventListPtr(0);
    } else {
      fakeevents = outWS->getEventListPtr(wsindex);
    }

    size_t numevents = realevents.getNumberEvents();
    for (size_t ie = 0; ie < numevents; ie++) {
      DataObjects::WeightedEvent event = realevents.getEvent(ie);
      // Swap TOF and pulse time and add to new event list
      // a) TOF (ms) -> Pulse Time
      double oldtof = event.tof();
      int64_t oldtofns = int64_t(oldtof * 1000);
      Kernel::DateAndTime newpulsetime(oldtofns);

      // b) Pulse Time (ns) -> TOF (ms) -> second
      int64_t oldpulsetime =
          event.pulseTime().totalNanoseconds() - runstarttime;
      double newtofinsecond = double(oldpulsetime) * 1.0E-9 + margin_sec;

      DataObjects::TofEvent fakeevent(newtofinsecond, newpulsetime);

      fakeevents->addEventQuickly(fakeevent);

    } // ENDFOR events

  } // END FOR: Per Spectrum

  g_log.debug() << "DBx505 Input Events = " << inpWS->getNumberEvents()
                << "; Output Events = " << outWS->getNumberEvents()
                << std::endl;

  return;
}

} // namespace Mantid
} // namespace Algorithms
