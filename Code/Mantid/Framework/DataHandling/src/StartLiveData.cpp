/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidDataHandling/StartLiveData.h"
#include "MantidKernel/System.h"

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
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string StartLiveData::category() const { return "General";}

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
    declareProperty(new PropertyWithValue<std::string>("Instrument","",Direction::Input),
        "Name of the instrument to monitor.");

    declareProperty(new PropertyWithValue<bool>("FromNow", true, Direction::Input),
        "Process live data starting from the current time only.");

    declareProperty(new PropertyWithValue<bool>("FromStartOfRun", false, Direction::Input),
        "Record live data, but go back to the the start of the run and process all data since then.");

    declareProperty(new PropertyWithValue<bool>("FromTime", false, Direction::Input),
        "Record live data, but go back to a specific time and process all data since then.\n"
        "You must specify the StartTime property if this is checked.");

    declareProperty(new PropertyWithValue<std::string>("StartTime","",Direction::Input),
        "Absolute start time, if you selected FromTime.\n"
        "Specify the date/time in UTC time, in ISO8601 format, e.g. 2010-09-14T04:20:12.95");

    declareProperty(new PropertyWithValue<double>("UpdateEvery", 60.0, Direction::Input),
        "Frequency of updates, in seconds. Default 60.");

    declareProperty(new PropertyWithValue<std::string>("ProcessingAlgorithm","",Direction::Input),
        "Name of the algorithm that will be run to process the data.\n"
        "Optional. If blank, no processing will occur.");

    declareProperty(new PropertyWithValue<std::string>("AlgorithmProperties","",Direction::Input),
        "The properties to pass to the ProcessingAlgorithm, as a single string."
        "The format is propName=value,propName=value,propName=value");

    declareProperty(new PropertyWithValue<std::string>("ProcessingScript","",Direction::Input),
        "Not currently supported, but reserved for future use.");

    std::vector<std::string> propOptions;
    propOptions.push_back("Add");
    propOptions.push_back("Replace");
    propOptions.push_back("Conjoin");
    declareProperty("AccumulationMethod", "Add", new ListValidator(propOptions),
        "Method to use for accumulating each chunk of live data.\n"
        " - Add: the processed chunk will be summed to the previous outpu (default).\n"
        " - Replace: the processed chunk will replace the previous output.\n"
        " - Conjoin: the spectra of the chunk will be appended to the output workspace, increasing its size.");

    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
        "Name of the processed output workspace.");

    declareProperty(new PropertyWithValue<std::string>("LastTimeStamp","",Direction::Output),
        "The time stamp of the last event, frame or pulse recorded.\n"
        "Date/time is in UTC time, in ISO8601 format, e.g. 2010-09-14T04:20:12.95");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void StartLiveData::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace Mantid
} // namespace DataHandling
