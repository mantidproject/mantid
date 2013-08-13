/*WIKI*

The MonitorLiveData algorithm is started in the background
by [[StartLiveData]] and repeatedly calls [[LoadLiveData]].
'''It should not be necessary to call MonitorLiveData directly.'''

This algorithm simply calls [[LoadLiveData]] at the given ''UpdateFrequency''.
For more details, see [[StartLiveData]].

For details on the way to specify the data processing steps, see: [[LoadLiveData#Description|LoadLiveData]].

*WIKI*/

#include "MantidLiveData/MonitorLiveData.h"
#include "MantidKernel/System.h"
#include "MantidLiveData/LoadLiveData.h"
#include <Poco/Thread.h>
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidKernel/WriteLock.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace LiveData
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(MonitorLiveData)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MonitorLiveData::MonitorLiveData()
  : m_chunkNumber(0)
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MonitorLiveData::~MonitorLiveData()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string MonitorLiveData::name() const { return "MonitorLiveData";};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string MonitorLiveData::category() const { return "DataHandling\\LiveData\\Support";}

  /// Algorithm's version for identification. @see Algorithm::version
  int MonitorLiveData::version() const { return 1;};
  
  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void MonitorLiveData::initDocs()
  {
    this->setWikiSummary("Call LoadLiveData at a given update frequency. Do not call this algorithm directly; instead call StartLiveData.");
    this->setOptionalMessage("Call LoadLiveData at a given update frequency. Do not call this algorithm directly; instead call StartLiveData.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void MonitorLiveData::init()
  {
    declareProperty(new PropertyWithValue<double>("UpdateEvery", 60.0, Direction::Input),
        "Frequency of updates, in seconds. Default 60.");

    this->initProps();
  }

  //----------------------------------------------------------------------------------------------
  /** Clone a workspace, if there is enough memory available */
  void MonitorLiveData::doClone(const std::string & originalName, const std::string & newName)
  {
    if (AnalysisDataService::Instance().doesExist(originalName))
    {
      Workspace_sptr original = AnalysisDataService::Instance().retrieveWS<Workspace>(originalName);
      if (original)
      {
        size_t bytesUsed = original->getMemorySize();
        size_t bytesAvail = MemoryManager::Instance().getMemoryInfo().availMemory * size_t(1024);
        // Give a buffer of 3 times the size of the workspace
        if (size_t(3)*bytesUsed < bytesAvail)
        {
          WriteLock _lock(*original);
          Algorithm_sptr cloner = createChildAlgorithm("CloneWorkspace", 0, 0, false);
          cloner->setPropertyValue("InputWorkspace", originalName);
          cloner->setPropertyValue("OutputWorkspace", newName);
          cloner->setAlwaysStoreInADS(true); // We must force the ADS to be updated
          cloner->executeAsChildAlg();
        }
        else
        {
          std::cout << "Not cloning\n";
          g_log.warning() << "Not enough spare memory to clone " << originalName <<
              ". Workspace will be reset." << std::endl;
        }
      }
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void MonitorLiveData::exec()
  {
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
    std::string AccumulationWorkspace = this->getPropertyValue("AccumulationWorkspace");
    std::string OutputWorkspace = this->getPropertyValue("OutputWorkspace");

    std::string NextAccumulationMethod = this->getPropertyValue("AccumulationMethod");

    // Keep going until you get cancelled
    while (true)
    {
      // Exit if the user presses cancel
      this->interruption_point();

      // Sleep for 50 msec
      Poco::Thread::sleep(50);

      DateAndTime now = DateAndTime::getCurrentTime();
      double seconds;
      seconds = DateAndTime::secondsFromDuration( now - lastTime );
      if (seconds > UpdateEvery)
      {
        lastTime = now;
        g_log.notice() << "Loading live data chunk " << m_chunkNumber << " at " << now.toFormattedString("%H:%M:%S") << std::endl;

        // Time to run LoadLiveData again
        Algorithm_sptr alg = createChildAlgorithm("LoadLiveData");
        LoadLiveData * loadAlg = dynamic_cast<LoadLiveData*>(alg.get());
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

        // Did we just hit the end of a run?
        if (listener->runStatus() == ILiveListener::EndRun)
        {
          // Find the run number, if that is possible.
          int runNumber = 0;
          MatrixWorkspace_sptr OutputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(OutputWorkspace);
          if (OutputWS)
            runNumber = OutputWS->getRunNumber();

          g_log.notice();
          std::string EndRunBehavior = this->getPropertyValue("EndRunBehavior");
          if (EndRunBehavior == "Stop")
          {
            g_log.notice() << "Run #" << runNumber << " ended. Stopping live data monitoring." << std::endl;
            break;
          }
          else if (EndRunBehavior == "Restart")
          {
            g_log.notice() << "Run #" << runNumber << " ended. Clearing existing workspace." << std::endl;
            NextAccumulationMethod = "Replace";
          }
          else if (EndRunBehavior == "Rename")
          {
            g_log.notice() << "Run #" << runNumber << " ended. Renaming existing workspace." << std::endl;
            NextAccumulationMethod = "Replace";

            // Now we clone the existing workspaces
            std::string postFix = "_" + Strings::toString(runNumber);
            doClone(OutputWorkspace, OutputWorkspace + postFix);
            if (!AccumulationWorkspace.empty())
              doClone(AccumulationWorkspace, AccumulationWorkspace + postFix);
          }
        }

        m_chunkNumber++;
        progress( 0.0, "Live Data " + Strings::toString(m_chunkNumber));
      }

      // This is the time to process a single chunk. Is it too long?
      seconds = DateAndTime::secondsFromDuration( now - lastTime );
      if (seconds > UpdateEvery)
        g_log.warning() << "Cannot process live data as quickly as requested: requested every " << UpdateEvery << " seconds but it takes " << seconds << " seconds!" << std::endl;
    } // loop until aborted

    // Set the outputs (only applicable when EndRunBehavior is "Stop")
    Workspace_sptr OutputWS = AnalysisDataService::Instance().retrieveWS<Workspace>(OutputWorkspace);
    this->setProperty("OutputWorkspace", OutputWS);
    if (!AccumulationWorkspace.empty())
    {
      Workspace_sptr AccumulationWS = AnalysisDataService::Instance().retrieveWS<Workspace>(AccumulationWorkspace);
      this->setProperty("AccumulationWorkspace", AccumulationWS);
    }

  } // exec()



} // namespace LiveData
} // namespace Mantid
