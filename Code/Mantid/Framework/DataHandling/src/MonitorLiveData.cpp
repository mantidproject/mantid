/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidDataHandling/MonitorLiveData.h"
#include "MantidKernel/System.h"

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
    // The last time we called LoadLiveData.
    // Since StartLiveData _just_ called it, use the current time.
    DateAndTime lastTime = DateAndTime::getCurrentTime();
  }



} // namespace Mantid
} // namespace DataHandling
