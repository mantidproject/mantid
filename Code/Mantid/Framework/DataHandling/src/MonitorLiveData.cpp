/*WIKI*

The MonitorLiveData algorithm is started in the background
by [[StartLiveData]] and repeatedly calls [[LoadLiveData]].
'''It should not be necessary to call MonitorLiveData directly.'''

This algorithm simply calls [[LoadLiveData]] at the given ''UpdateFrequency''.
For more details, see [[StartLiveData]].

For details on the way to specify the data processing steps, see: [[LoadLiveData#Description|LoadLiveData]].

*WIKI*/

#include "MantidDataHandling/MonitorLiveData.h"
#include "MantidKernel/System.h"
#include "MantidDataHandling/LoadLiveData.h"
#include <Poco/Thread.h>
#include "MantidAPI/AlgorithmManager.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(MonitorLiveData)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MonitorLiveData::MonitorLiveData()
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
  /** Execute the algorithm.
   */
  void MonitorLiveData::exec()
  {
    double UpdateEvery = getProperty("UpdateEvery");
    if (UpdateEvery <= 0)
      throw std::runtime_error("UpdateEvery must be > 0");

    // Get the listener (and start listening) as early as possible
    ILiveListener_sptr listener = this->getLiveListener();

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
      // This call throws if the user presses cancel
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
        Algorithm_sptr alg = createSubAlgorithm("LoadLiveData");
        LoadLiveData * loadAlg = dynamic_cast<LoadLiveData*>(alg.get());
        if (!loadAlg)
          throw std::runtime_error("Error creating LoadLiveData sub-algorithm");

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
        loadAlg->executeAsSubAlg();

        NextAccumulationMethod = this->getPropertyValue("AccumulationMethod");

        // Did we just hit the end of a run?
        if (listener->runStatus() == ILiveListener::EndRun)
        {
          g_log.notice() << "Run ended.";
          std::string EndRunBehavior = this->getPropertyValue("EndRunBehavior");
          if (EndRunBehavior == "Stop")
          {
            g_log.notice() << " Stopping live data monitoring." << std::endl;
            break;
          }
          else if (EndRunBehavior == "Restart")
          {
            g_log.notice() << " Clearing existing workspace.";
            NextAccumulationMethod = "Replace";
          }
          else if (EndRunBehavior == "Rename")
          {
            g_log.notice() << " Renaming existing workspace.";
            NextAccumulationMethod = "Replace";
            //TODO
          }
          g_log.notice() << std::endl;
        }

        m_chunkNumber++;
        //progress( double(chunk % 100)*0.01, "chunk " + Strings::toString(chunk));
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



} // namespace Mantid
} // namespace DataHandling
