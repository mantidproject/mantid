/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidDataHandling/MonitorLiveData.h"
#include "MantidKernel/System.h"
#include <unistd.h>
#include "MantidDataHandling/LoadLiveData.h"
#include <Poco/Thread.h>

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

    size_t chunk = 0;

    while (true)
    {
      // Keep going until you get cancelled
      this->interruption_point();

      // Sleep for 50 msec
      Poco::Thread::sleep(50);

      DateAndTime now = DateAndTime::getCurrentTime();
      double seconds = DateAndTime::secondsFromDuration( now - lastTime );
      if (seconds > UpdateEvery)
      {
        lastTime = now;
        g_log.notice() << "Loading live data chunk " << chunk << " at " << now.toFormattedString("%H:%M:%S") << std::endl;
        // Time to run LoadLiveData again
        LoadLiveData loadAlg;
        loadAlg.initialize();
        // So the output gets put into the ADS
        loadAlg.setChild(false);
        // Too much logging
        loadAlg.setLogging(false);
        // Copy settings from THIS to LoadAlg
        loadAlg.copyPropertyValuesFrom(*this);
        // Give the listener directly to LoadLiveData (don't re-create it)
        loadAlg.setLiveListener(listener);

        // Run the LoadLiveData
        loadAlg.executeAsSubAlg();

        chunk++;
      }
    }
  }



} // namespace Mantid
} // namespace DataHandling
