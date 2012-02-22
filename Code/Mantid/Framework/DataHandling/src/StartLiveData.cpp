/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidDataHandling/StartLiveData.h"
#include "MantidKernel/System.h"
#include "MantidDataHandling/LoadLiveData.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(StartLiveData)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  StartLiveData::StartLiveData()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  StartLiveData::~StartLiveData()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string StartLiveData::name() const { return "StartLiveData";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int StartLiveData::version() const { return 1;};

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void StartLiveData::initDocs()
  {
    this->setWikiSummary("Begin live data monitoring.");
    this->setOptionalMessage("Begin live data monitoring.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void StartLiveData::init()
  {
    declareProperty(new PropertyWithValue<bool>("FromNow", true, Direction::Input),
        "Process live data starting from the current time only.");

    declareProperty(new PropertyWithValue<bool>("FromStartOfRun", false, Direction::Input),
        "Record live data, but go back to the the start of the run and process all data since then.");

    declareProperty(new PropertyWithValue<bool>("FromTime", false, Direction::Input),
        "Record live data, but go back to a specific time and process all data since then.\n"
        "You must specify the StartTime property if this is checked.");

    declareProperty(new PropertyWithValue<double>("UpdateEvery", 60.0, Direction::Input),
        "Frequency of updates, in seconds. Default 60.\n"
        "If you specify 0, MonitorLiveData will not launch and you will get only one chunk.");

    // Initialize the properties common to LiveDataAlgorithm.
    initProps();
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void StartLiveData::exec()
  {
    // Validate the inputs
    bool FromNow = getProperty("FromNow");
    bool FromStartOfRun = getProperty("FromStartOfRun");
    bool FromTime = getProperty("FromTime");
    int numChecked = 0;
    if (FromNow) numChecked++;
    if (FromStartOfRun) numChecked++;
    if (FromTime) numChecked++;

    if (numChecked != 1)
      throw std::runtime_error("Please check exactly one of FromNow, FromStartOfRun, FromTime.");

    // Adjust the StartTime if you are starting from run/now.
    if (FromNow)
      this->setPropertyValue("StartTime", DateAndTime::getCurrentTime().toISO8601String());
    else if (FromStartOfRun)
      // TODO: implement
      throw Kernel::Exception::NotImplementedError("Cannot start from the run start yet.");

    // Get the listener (and start listening) as early as possible
    ILiveListener_sptr listener = this->getLiveListener();

    // TODO: Wait a bit to make sure something gets accumulated?

    LoadLiveData loadAlg;
    loadAlg.initialize();
    loadAlg.setChild(true);
    // Copy settings from THIS to LoadAlg
    loadAlg.copyPropertyValuesFrom(*this);
    // Give the listener directly to LoadLiveData (don't re-create it)
    loadAlg.setLiveListener(listener);

    // Run the LoadLiveData for the first time.
    loadAlg.executeAsSubAlg();

    // Copy the output workspace properties from LoadLiveData
    Workspace_sptr outWS = loadAlg.getProperty("OutputWorkspace");
    this->setProperty("OutputWorkspace", outWS);
    Workspace_sptr accumWS = loadAlg.getProperty("AccumulationWorkspace");
    this->setProperty("AccumulationWorkspace", accumWS);


    double UpdateEvery = this->getProperty("UpdateEvery");
    if (UpdateEvery > 0)
    {
      //TODO: Launch MonitorLiveData asynchronously
    }

  }



} // namespace Mantid
} // namespace DataHandling
