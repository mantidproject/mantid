#include "MantidDataHandling/LiveDataAlgorithm.h"
#include "MantidKernel/System.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidAPI/LiveListenerFactory.h"

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
  const std::string LiveDataAlgorithm::category() const { return "DataHandling\\LiveData";}

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
        "Name of the algorithm that will be run to process each chunk of data.\n"
        "Optional. If blank, no processing will occur.");

    declareProperty(new PropertyWithValue<std::string>("ProcessingProperties","",Direction::Input),
        "The properties to pass to the ProcessingAlgorithm, as a single string.\n"
        "The format is propName=value;propName=value");

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

    declareProperty(new PropertyWithValue<std::string>("PostProcessingAlgorithm","",Direction::Input),
        "Name of the algorithm that will be run to process the accumulated data.\n"
        "Optional. If blank, no post-processing will occur.");

    declareProperty(new PropertyWithValue<std::string>("PostProcessingProperties","",Direction::Input),
        "The properties to pass to the PostProcessingAlgorithm, as a single string.\n"
        "The format is propName=value;propName=value");

    declareProperty(new PropertyWithValue<std::string>("PostProcessingScript","",Direction::Input),
        "Not currently supported, but reserved for future use.");

    declareProperty(new WorkspaceProperty<Workspace>("AccumulationWorkspace","",Direction::Output, true),
        "Optional, unless performing PostProcessing:\n"
        " Give the name of the intermediate, accumulation workspace.\n"
        " This is the workspace after accumulation but before post-processing steps.");

    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output),
        "Name of the processed output workspace.");

    declareProperty(new PropertyWithValue<std::string>("LastTimeStamp","",Direction::Output),
        "The time stamp of the last event, frame or pulse recorded.\n"
        "Date/time is in UTC time, in ISO8601 format, e.g. 2010-09-14T04:20:12.95");
  }

  //----------------------------------------------------------------------------------------------
  /// @return true if there is a post-processing step
  bool LiveDataAlgorithm::hasPostProcessing() const
  {
    // TODO: Handle post-processing script too
    return !this->getPropertyValue("PostProcessingAlgorithm").empty();
  }

  //----------------------------------------------------------------------------------------------
  /** Return or create the ILiveListener for this algorithm.
   *
   * If the ILiveListener has not already been created, it creates it using
   * the properties on the algorithm. It then starts the listener
   * by calling the ILiveListener->start(StartTime) method.
   *
   * @return ILiveListener_sptr
   */
  ILiveListener_sptr LiveDataAlgorithm::getLiveListener()
  {
    if (m_listener)
      return m_listener;

    // Not stored? Need to create it
    std::string inst = this->getPropertyValue("Instrument");
    m_listener = LiveListenerFactory::Instance().create(inst);

    // Start at the given date/time
    m_listener->start( this->getStartTime() );

    return m_listener;
  }


  //----------------------------------------------------------------------------------------------
  /** Directly set the LiveListener for this algorithm.
   *
   * @param listener :: ILiveListener_sptr
   */
  void LiveDataAlgorithm::setLiveListener(Mantid::API::ILiveListener_sptr listener)
  {
    m_listener = listener;
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
  /** Using the [Post]ProcessingAlgorithm and [Post]ProcessingProperties properties,
   * create and initialize an algorithm for processing.
   *
   * @param postProcessing :: true to create the PostProcessingAlgorithm.
   *        false to create the ProcessingAlgorithm
   * @return pointer to the algorithm, ready for execution.
   *         Returns a NULL pointer if no algorithm was chosen.
   *         This pointer is owned by the AlgorithmManager, so DO NOT DELETE!
   */
  IAlgorithm * LiveDataAlgorithm::makeAlgorithm(bool postProcessing)
  {
    std::string prefix = "";
    if (postProcessing)
      prefix = "Post";

    std::string algoName = this->getPropertyValue(prefix+"ProcessingAlgorithm");
    algoName = Strings::strip(algoName);
    if (algoName.empty())
      return NULL;

    std::string props = this->getPropertyValue(prefix+"ProcessingProperties");

    // TODO: Handle script too.

    // Create the algorithm and pass it the properties
    IAlgorithm * alg = FrameworkManager::Instance().createAlgorithm(algoName, props);

    return alg;
  }

} // namespace Mantid
} // namespace DataHandling
