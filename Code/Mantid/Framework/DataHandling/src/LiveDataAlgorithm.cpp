#include "MantidDataHandling/LiveDataAlgorithm.h"
#include "MantidKernel/System.h"
#include "MantidKernel/DateAndTime.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace DataHandling
{

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LiveDataAlgorithm::LiveDataAlgorithm()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LiveDataAlgorithm::~LiveDataAlgorithm()
  {
  }
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string LiveDataAlgorithm::category() const { return "DataHandling//LiveData";}

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void LiveDataAlgorithm::initProps()
  {

    declareProperty(new PropertyWithValue<std::string>("Instrument","",Direction::Input),
        "Name of the instrument to monitor.");

    declareProperty(new PropertyWithValue<std::string>("StartTime","",Direction::Input),
        "Absolute start time, if you selected FromTime.\n"
        "Specify the date/time in UTC time, in ISO8601 format, e.g. 2010-09-14T04:20:12.95");

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
  /** @return the value of the StartTime property */
  Mantid::Kernel::DateAndTime LiveDataAlgorithm::getStartTime() const
  {
    std::string date = getPropertyValue("StartTime");
    if (date.empty())
      return DateAndTime();
    return DateAndTime(date);
  }

  //----------------------------------------------------------------------------------------------
  /** Using the ProcessingAlgorithm and AlgorithmProperties properties,
   * create and initialize an algorithm for processing.
   *
   * @return shared pointer to the algorithm, ready for execution.
   *         Returns an empty pointer if no algorithm was chosen.
   */
  IAlgorithm_sptr LiveDataAlgorithm::makeAlgorithm()
  {
    IAlgorithm_sptr alg;
    std::string algoName = this->getPropertyValue("ProcessingAlgorithm");
    algoName = Strings::strip(algoName);
    if (algoName.empty())
      return alg;

    std::string props = this->getPropertyValue("AlgorithmProperties");

    // Create the algorithm and pass it the properties
    alg = IAlgorithm_sptr(FrameworkManager::Instance().createAlgorithm(algoName, props));

    return alg;
  }

} // namespace Mantid
} // namespace DataHandling
