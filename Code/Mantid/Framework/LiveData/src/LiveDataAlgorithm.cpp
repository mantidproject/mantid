#include "MantidLiveData/LiveDataAlgorithm.h"
#include "MantidKernel/System.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/AlgorithmManager.h"
#include "boost/tokenizer.hpp"
#include <boost/algorithm/string/trim.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using boost::tokenizer;

namespace Mantid
{
namespace LiveData
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
    // Add all the instruments (in the default facility) that have a listener specified
    std::vector<std::string> instruments;
    auto& instrInfo = Kernel::ConfigService::Instance().getFacility().instruments();
    for(auto it = instrInfo.begin(); it != instrInfo.end(); ++it)
    {
      if ( !it->liveDataAddress().empty() )
      {
        instruments.push_back( it->name() );
      }
    }
#ifndef NDEBUG
    // Debug builds only: Add all the listeners by hand for development testing purposes
    std::vector<std::string> listeners = Mantid::API::LiveListenerFactory::Instance().getKeys();
    instruments.insert( instruments.end(), listeners.begin(), listeners.end() );
#endif
    declareProperty(new PropertyWithValue<std::string>("Instrument","", boost::make_shared<StringListValidator>(instruments)),
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
        "A Python script that will be run to process each chunk of data. Only for command line usage, does not appear on the user interface.");

    std::vector<std::string> propOptions;
    propOptions.push_back("Add");
    propOptions.push_back("Replace");
    propOptions.push_back("Append");
    declareProperty("AccumulationMethod", "Add", boost::make_shared<StringListValidator>(propOptions),
        "Method to use for accumulating each chunk of live data.\n"
        " - Add: the processed chunk will be summed to the previous outpu (default).\n"
        " - Replace: the processed chunk will replace the previous output.\n"
        " - Append: the spectra of the chunk will be appended to the output workspace, increasing its size.");

    declareProperty("PreserveEvents", false,
        "Preserve events after performing the Processing step. Default False.\n"
        "This only applies if the ProcessingAlgorithm produces an EventWorkspace.\n"
        "It is strongly recommended to keep this unchecked, because preserving events\n"
        "may cause significant slowdowns when the run becomes large!");

    declareProperty(new PropertyWithValue<std::string>("PostProcessingAlgorithm","",Direction::Input),
        "Name of the algorithm that will be run to process the accumulated data.\n"
        "Optional. If blank, no post-processing will occur.");

    declareProperty(new PropertyWithValue<std::string>("PostProcessingProperties","",Direction::Input),
        "The properties to pass to the PostProcessingAlgorithm, as a single string.\n"
        "The format is propName=value;propName=value");

    declareProperty(new PropertyWithValue<std::string>("PostProcessingScript","",Direction::Input),
        "A Python script that will be run to process the accumulated data.");

    std::vector<std::string> runOptions;
    runOptions.push_back("Restart");
    runOptions.push_back("Stop");
    runOptions.push_back("Rename");
    declareProperty("RunTransitionBehavior", "Restart", boost::make_shared<StringListValidator>(runOptions),
        "What to do at run start/end boundaries?\n"
        " - Restart: the previously accumulated data is discarded.\n"
        " - Stop: live data monitoring ends.\n"
        " - Rename: the previous workspaces are renamed, and monitoring continues with cleared ones.");

    declareProperty(new WorkspaceProperty<Workspace>("AccumulationWorkspace","",Direction::Output,
                                                     PropertyMode::Optional, LockMode::NoLock),
        "Optional, unless performing PostProcessing:\n"
        " Give the name of the intermediate, accumulation workspace.\n"
        " This is the workspace after accumulation but before post-processing steps.");

    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output,
                                                     PropertyMode::Mandatory, LockMode::NoLock),
                    "Name of the processed output workspace.");

    declareProperty(new PropertyWithValue<std::string>("LastTimeStamp","",Direction::Output),
                    "The time stamp of the last event, frame or pulse recorded.\n"
                    "Date/time is in UTC time, in ISO8601 format, e.g. 2010-09-14T04:20:12.95");
  }


  //----------------------------------------------------------------------------------------------
  /** Copy the LiveDataAlgorithm-specific properties from "other" to "this"
   *
   * @param other :: LiveDataAlgorithm-type algo.
   */
  void LiveDataAlgorithm::copyPropertyValuesFrom(const LiveDataAlgorithm & other)
  {
    std::vector<Property*> props = this->getProperties();
    for (size_t i=0; i < props.size(); i++)
    {
      Property*prop = props[i];
      this->setPropertyValue(prop->name(), other.getPropertyValue(prop->name()));
    }
  }


  //----------------------------------------------------------------------------------------------
  /// @return true if there is a post-processing step
  bool LiveDataAlgorithm::hasPostProcessing() const
  {
    return (!this->getPropertyValue("PostProcessingAlgorithm").empty()
        || !this->getPropertyValue("PostProcessingScript").empty());
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
    m_listener = LiveListenerFactory::Instance().create(inst, true, this);

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
   * @return shared pointer to the algorithm, ready for execution.
   *         Returns a NULL pointer if no algorithm was chosen.
   */
  IAlgorithm_sptr LiveDataAlgorithm::makeAlgorithm(bool postProcessing)
  {
    std::string prefix = "";
    if (postProcessing)
      prefix = "Post";

    // Get the name of the algorithm to run
    std::string algoName = this->getPropertyValue(prefix+"ProcessingAlgorithm");
    algoName = Strings::strip(algoName);

    // Get the script to run. Ignored if algo is specified
    std::string script = this->getPropertyValue(prefix+"ProcessingScript");
    script = Strings::strip(script);

    if (!algoName.empty())
    {
      // Properties to pass to algo
      std::string props = this->getPropertyValue(prefix+"ProcessingProperties");

      // Create the UNMANAGED algorithm
      IAlgorithm_sptr alg = this->createChildAlgorithm(algoName);

      // ...and pass it the properties
      boost::char_separator<char> sep(";");
      typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
      tokenizer propPairs(props, sep);
      // Iterate over the properties
      for (tokenizer::iterator it = propPairs.begin(); it != propPairs.end(); ++it)
      {
        // Pair of the type "
        std::string pair = *it;

        size_t n = pair.find('=');
        if (n == std::string::npos)
        {
          // Do nothing
        }
        else
        {
          // Normal "PropertyName=value" string.
          std::string propName = "";
          std::string value = "";

          // Extract the value string
          if (n < pair.size()-1)
          {
            propName = pair.substr(0, n);
            value = pair.substr(n+1, pair.size()-n-1);
          }
          else
          {
            // String is "PropertyName="
            propName = pair.substr(0, n);
            value = "";
          }
          // Skip some of the properties when setting
          if ((propName != "InputWorkspace") && (propName != "OutputWorkspace"))
            alg->setPropertyValue(propName,value);
        }
      }

      // Warn if someone put both values.
      if (!script.empty())
        g_log.warning() << "Running algorithm " << algoName << " and ignoring the script code in " << prefix+"ProcessingScript" << std::endl;
      return alg;
    }
    else if (!script.empty())
    {
      // Run a snippet of python
      IAlgorithm_sptr alg = this->createChildAlgorithm("RunPythonScript");
      alg->setLogging(false);
      alg->setPropertyValue("Code", script);
      return alg;
    }
    else
      return IAlgorithm_sptr();
  }

  //----------------------------------------------------------------------------------------------
  /** Validate the properties together */
  std::map<std::string, std::string> LiveDataAlgorithm::validateInputs()
  {
    std::map<std::string, std::string> out;

    const std::string instrument = getPropertyValue("Instrument");
    bool eventListener;
    if ( m_listener )
    {
      eventListener = m_listener->buffersEvents();
    }
    else
    {
      eventListener = LiveListenerFactory::Instance().create(instrument,false)->buffersEvents();
    }
    if ( !eventListener && getPropertyValue("AccumulationMethod") == "Add" )
    {
      out["AccumulationMethod"] = "The " + instrument + " live stream produces histograms. Add is not a sensible accumulation method.";
    }

    if (this->getPropertyValue("OutputWorkspace").empty())
      out["OutputWorkspace"] = "Must specify the OutputWorkspace.";

    // Validate inputs
    if (this->hasPostProcessing())
    {
      if (this->getPropertyValue("AccumulationWorkspace").empty())
        out["AccumulationWorkspace"] = "Must specify the AccumulationWorkspace parameter if using PostProcessing.";

      if (this->getPropertyValue("AccumulationWorkspace") == this->getPropertyValue("OutputWorkspace"))
        out["AccumulationWorkspace"] = "The AccumulationWorkspace must be different than the OutputWorkspace, when using PostProcessing.";
    }

    // For StartLiveData and MonitorLiveData, make sure another thread is not already using these names
    if (this->name() != "LoadLiveData")
    {
      /** Validate that the workspace names chosen are not in use already */
      std::string outName = this->getPropertyValue("OutputWorkspace");
      std::string accumName = this->getPropertyValue("AccumulationWorkspace");

      // Check that no other MonitorLiveData thread is running with the same settings
      auto runningLiveAlgorithms = AlgorithmManager::Instance().runningInstancesOf("MonitorLiveData");
      auto runningAlgs_it = runningLiveAlgorithms.begin();
      while( runningAlgs_it != runningLiveAlgorithms.end() )
      {
        IAlgorithm_const_sptr alg = *runningAlgs_it;
        // MonitorLiveData thread that is running, except THIS one.
        if ( alg->getAlgorithmID() != this->getAlgorithmID() )
        {
          if (!accumName.empty() && alg->getPropertyValue("AccumulationWorkspace") == accumName)
            out["AccumulationWorkspace"] += "Another MonitorLiveData thread is running with the same AccumulationWorkspace.\n"
                "Please specify a different AccumulationWorkspace name.";
          if (alg->getPropertyValue("OutputWorkspace") == outName)
            out["OutputWorkspace"] += "Another MonitorLiveData thread is running with the same OutputWorkspace.\n"
                "Please specify a different OutputWorkspace name.";
        }
        ++runningAlgs_it;
      }
    }

    return out;
  }


} // namespace LiveData
} // namespace Mantid
