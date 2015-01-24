#include "MantidLiveData/MonitorLiveData.h"
#include "MantidKernel/System.h"
#include "MantidLiveData/LoadLiveData.h"
#include <Poco/Thread.h>
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidKernel/WriteLock.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace LiveData {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MonitorLiveData)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
MonitorLiveData::MonitorLiveData() : m_chunkNumber(0) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
MonitorLiveData::~MonitorLiveData() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string MonitorLiveData::name() const { return "MonitorLiveData"; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string MonitorLiveData::category() const {
  return "DataHandling\\LiveData\\Support";
}

/// Algorithm's version for identification. @see Algorithm::version
int MonitorLiveData::version() const { return 1; };

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void MonitorLiveData::init() {
  declareProperty(
      new PropertyWithValue<double>("UpdateEvery", 60.0, Direction::Input),
      "Frequency of updates, in seconds. Default 60.");

  this->initProps();
}

//----------------------------------------------------------------------------------------------
/** Clone a workspace, if there is enough memory available.
    It needs to be a clone rather than a rename so that an open instrument
   window continues
    to track the live workspace.
 */
void MonitorLiveData::doClone(const std::string &originalName,
                              const std::string &newName) {
  auto &ads = AnalysisDataService::Instance();
  if (ads.doesExist(originalName)) {
    Workspace_sptr original = ads.retrieveWS<Workspace>(originalName);
    if (original) {
      size_t bytesUsed = original->getMemorySize();
      size_t bytesAvail =
          MemoryManager::Instance().getMemoryInfo().availMemory * size_t(1024);
      // Give a buffer of 3 times the size of the workspace
      if (size_t(3) * bytesUsed < bytesAvail) {
        WriteLock _lock(*original);

        // Clone the monitor workspace, if there is one
        auto originalMatrix =
            boost::dynamic_pointer_cast<MatrixWorkspace>(original);
        MatrixWorkspace_sptr monitorWS, newMonitorWS;
        if (originalMatrix &&
            (monitorWS = originalMatrix->monitorWorkspace())) {
          auto monitorsCloner =
              createChildAlgorithm("CloneWorkspace", 0, 0, false);
          monitorsCloner->setProperty("InputWorkspace", monitorWS);
          monitorsCloner->executeAsChildAlg();
          Workspace_sptr outputWS =
              monitorsCloner->getProperty("OutputWorkspace");
          newMonitorWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS);
        }

        Algorithm_sptr cloner =
            createChildAlgorithm("CloneWorkspace", 0, 0, false);
        cloner->setPropertyValue("InputWorkspace", originalName);
        cloner->setPropertyValue("OutputWorkspace", newName);
        cloner->setAlwaysStoreInADS(
            true); // We must force the ADS to be updated
        cloner->executeAsChildAlg();

        if (newMonitorWS) // If there was a monitor workspace, set it back on
                          // the result
        {
          ads.retrieveWS<MatrixWorkspace>(newName)
              ->setMonitorWorkspace(newMonitorWS);
        }
      } else {
        std::cout << "Not cloning\n";
        g_log.warning() << "Not enough spare memory to clone " << originalName
                        << ". Workspace will be reset." << std::endl;
      }
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void MonitorLiveData::exec() {
  double UpdateEvery = getProperty("UpdateEvery");
  if (UpdateEvery <= 0)
    throw std::runtime_error("UpdateEvery must be > 0");

  // Get the listener (and start listening) as early as possible
  ILiveListener_sptr listener = this->getLiveListener();
  // Grab ownership of the pointer in the LOCAL listener variable.
  // This will make the listener destruct if the algorithm throws,
  // but the Algorithm is still in the list of "Managed" algorithms.
  m_listener.reset();

  // The last time we called LoadLiveData.
  // Since StartLiveData _just_ called it, use the current time.
  DateAndTime lastTime = DateAndTime::getCurrentTime();

  m_chunkNumber = 0;
  int runNumber = 0;
  int prevRunNumber = 0;

  std::string AccumulationWorkspace =
      this->getPropertyValue("AccumulationWorkspace");
  std::string OutputWorkspace = this->getPropertyValue("OutputWorkspace");

  std::string NextAccumulationMethod =
      this->getPropertyValue("AccumulationMethod");

  // Keep going until you get cancelled
  while (true) {
    // Exit if the user presses cancel
    this->interruption_point();

    // Sleep for 50 msec
    Poco::Thread::sleep(50);

    DateAndTime now = DateAndTime::getCurrentTime();
    double seconds = DateAndTime::secondsFromDuration(now - lastTime);
    if (seconds > UpdateEvery) {
      lastTime = now;
      g_log.notice() << "Loading live data chunk " << m_chunkNumber << " at "
                     << now.toFormattedString("%H:%M:%S") << std::endl;

      // Time to run LoadLiveData again
      Algorithm_sptr alg = createChildAlgorithm("LoadLiveData");
      LoadLiveData *loadAlg = dynamic_cast<LoadLiveData *>(alg.get());
      if (!loadAlg)
        throw std::runtime_error("Error creating LoadLiveData Child Algorithm");

      loadAlg->setChild(true);
      // So the output gets put into the ADS
      loadAlg->setAlwaysStoreInADS(true);
      // Too much logging
      loadAlg->setLogging(false);
      loadAlg->initialize();
      // Copy settings from THIS to LoadAlg
      loadAlg->copyPropertyValuesFrom(*this);
      // Give the listener directly to LoadLiveData (don't re-create it)
      loadAlg->setLiveListener(listener);
      // Override the AccumulationMethod when a run ends.
      loadAlg->setPropertyValue("AccumulationMethod", NextAccumulationMethod);

      // Run the LoadLiveData
      loadAlg->executeAsChildAlg();

      NextAccumulationMethod = this->getPropertyValue("AccumulationMethod");

      if (runNumber == 0) {
        runNumber = listener->runNumber();
        g_log.debug() << "Run number set to " << runNumber << std::endl;
      }

      // Did we just hit a run transition?
      ILiveListener::RunStatus runStatus = listener->runStatus();
      if (runStatus == ILiveListener::EndRun) {
        // Need to keep track of what the run number *was* so we
        // can properly rename workspaces
        prevRunNumber = runNumber;
      }

      if ((runStatus == ILiveListener::BeginRun) ||
          (runStatus == ILiveListener::EndRun)) {
        std::stringstream message;
        message << "Run";
        if (runNumber != 0)
          message << " #" << runNumber;
        message << " ended. ";
        std::string RunTransitionBehavior =
            this->getPropertyValue("RunTransitionBehavior");
        if (RunTransitionBehavior == "Stop") {
          g_log.notice() << message.str() << "Stopping live data monitoring.\n";
          break;
        } else if (RunTransitionBehavior == "Restart") {
          g_log.notice() << message.str() << "Clearing existing workspace.\n";
          NextAccumulationMethod = "Replace";
        } else if (RunTransitionBehavior == "Rename") {
          g_log.notice() << message.str() << "Renaming existing workspace.\n";
          NextAccumulationMethod = "Replace";

          // Now we clone the existing workspaces
          std::string postFix;
          if (runStatus == ILiveListener::EndRun) {
            postFix = "_" + Strings::toString(runNumber);
          } else {
            // indicate that this is data that arrived *after* the run ended
            postFix = "_";
            if (prevRunNumber != 0)
              postFix += Strings::toString(prevRunNumber);

            postFix += "_post";
          }

          doClone(OutputWorkspace, OutputWorkspace + postFix);
        }

        runNumber = 0;
      }

      m_chunkNumber++;
      progress(0.0, "Live Data " + Strings::toString(m_chunkNumber));
    }

    // This is the time to process a single chunk. Is it too long?
    seconds = DateAndTime::secondsFromDuration(now - lastTime);
    if (seconds > UpdateEvery)
      g_log.warning() << "Cannot process live data as quickly as requested: "
                         "requested every " << UpdateEvery
                      << " seconds but it takes " << seconds << " seconds!"
                      << std::endl;
  } // loop until aborted

  // Set the outputs (only applicable when RunTransitionBehavior is "Stop")
  Workspace_sptr OutputWS =
      AnalysisDataService::Instance().retrieveWS<Workspace>(OutputWorkspace);
  this->setProperty("OutputWorkspace", OutputWS);
  if (!AccumulationWorkspace.empty()) {
    Workspace_sptr AccumulationWS =
        AnalysisDataService::Instance().retrieveWS<Workspace>(
            AccumulationWorkspace);
    this->setProperty("AccumulationWorkspace", AccumulationWS);
  }

} // exec()

} // namespace LiveData
} // namespace Mantid
