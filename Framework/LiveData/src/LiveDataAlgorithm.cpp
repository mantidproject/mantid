// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidLiveData/LiveDataAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Strings.h"

#include <boost/algorithm/string/trim.hpp>
#include <unordered_set>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Types::Core::DateAndTime;

namespace Mantid {
namespace LiveData {

/// Algorithm's category for identification. @see Algorithm::category
const std::string LiveDataAlgorithm::category() const {
  return "DataHandling\\LiveData";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LiveDataAlgorithm::initProps() {
  // Add all the instruments (in the default facility) that have a listener
  // specified
  std::vector<std::string> instruments;
  auto &instrInfo =
      Kernel::ConfigService::Instance().getFacility().instruments();
  for (const auto &instrument : instrInfo) {
    if (instrument.hasLiveListenerInfo()) {
      instruments.push_back(instrument.name());
    }
  }

  // All available listener class names
  auto listeners = LiveListenerFactory::Instance().getKeys();
  listeners.push_back(""); // Allow not specifying a listener too

  declareProperty(std::make_unique<PropertyWithValue<std::string>>(
                      "Instrument", "",
                      boost::make_shared<StringListValidator>(instruments)),
                  "Name of the instrument to monitor.");

  declareProperty(std::make_unique<PropertyWithValue<std::string>>("Connection", "",
                                                              Direction::Input),
                  "Selects the listener connection entry to use. "
                  "Default connection will be used if not specified");

  declareProperty(
      std::make_unique<PropertyWithValue<std::string>>(
          "Listener", "", boost::make_shared<StringListValidator>(listeners)),
      "Name of the listener class to use. "
      "If specified, overrides class specified by Connection.");

  declareProperty(std::make_unique<PropertyWithValue<std::string>>("Address", "",
                                                              Direction::Input),
                  "Address for the listener to connect to. "
                  "If specified, overrides address specified by Connection.");

  declareProperty(std::make_unique<PropertyWithValue<std::string>>("StartTime", "",
                                                              Direction::Input),
                  "Absolute start time, if you selected FromTime.\n"
                  "Specify the date/time in UTC time, in ISO8601 format, e.g. "
                  "2010-09-14T04:20:12.95");

  declareProperty(
      std::make_unique<PropertyWithValue<std::string>>("ProcessingAlgorithm", "",
                                                  Direction::Input),
      "Name of the algorithm that will be run to process each chunk of data.\n"
      "Optional. If blank, no processing will occur.");

  declareProperty(
      std::make_unique<PropertyWithValue<std::string>>("ProcessingProperties", "",
                                                  Direction::Input),
      "The properties to pass to the ProcessingAlgorithm, as a single string.\n"
      "The format is propName=value;propName=value");

  declareProperty(std::make_unique<PropertyWithValue<std::string>>(
                      "ProcessingScript", "", Direction::Input),
                  "A Python script that will be run to process each chunk of "
                  "data. Only for command line usage, does not appear on the "
                  "user interface.");

  declareProperty(std::make_unique<FileProperty>("ProcessingScriptFilename", "",
                                            FileProperty::OptionalLoad, "py"),
                  "A Python script that will be run to process each chunk of "
                  "data. Only for command line usage, does not appear on the "
                  "user interface.");

  std::vector<std::string> propOptions{"Add", "Replace", "Append"};
  declareProperty(
      "AccumulationMethod", "Add",
      boost::make_shared<StringListValidator>(propOptions),
      "Method to use for accumulating each chunk of live data.\n"
      " - Add: the processed chunk will be summed to the previous outpu "
      "(default).\n"
      " - Replace: the processed chunk will replace the previous output.\n"
      " - Append: the spectra of the chunk will be appended to the output "
      "workspace, increasing its size.");

  declareProperty(
      "PreserveEvents", false,
      "Preserve events after performing the Processing step. Default False.\n"
      "This only applies if the ProcessingAlgorithm produces an "
      "EventWorkspace.\n"
      "It is strongly recommended to keep this unchecked, because preserving "
      "events\n"
      "may cause significant slowdowns when the run becomes large!");

  declareProperty(std::make_unique<PropertyWithValue<std::string>>(
                      "PostProcessingAlgorithm", "", Direction::Input),
                  "Name of the algorithm that will be run to process the "
                  "accumulated data.\n"
                  "Optional. If blank, no post-processing will occur.");

  declareProperty(std::make_unique<PropertyWithValue<std::string>>(
                      "PostProcessingProperties", "", Direction::Input),
                  "The properties to pass to the PostProcessingAlgorithm, as a "
                  "single string.\n"
                  "The format is propName=value;propName=value");

  declareProperty(
      std::make_unique<PropertyWithValue<std::string>>("PostProcessingScript", "",
                                                  Direction::Input),
      "A Python script that will be run to process the accumulated data.");
  declareProperty(
      std::make_unique<FileProperty>("PostProcessingScriptFilename", "",
                                FileProperty::OptionalLoad, "py"),
      " Python script that will be run to process the accumulated data.");

  std::vector<std::string> runOptions{"Restart", "Stop", "Rename"};
  declareProperty("RunTransitionBehavior", "Restart",
                  boost::make_shared<StringListValidator>(runOptions),
                  "What to do at run start/end boundaries?\n"
                  " - Restart: the previously accumulated data is discarded.\n"
                  " - Stop: live data monitoring ends.\n"
                  " - Rename: the previous workspaces are renamed, and "
                  "monitoring continues with cleared ones.");

  declareProperty(
      std::make_unique<WorkspaceProperty<Workspace>>(
          "AccumulationWorkspace", "", Direction::Output,
          PropertyMode::Optional, LockMode::NoLock),
      "Optional, unless performing PostProcessing:\n"
      " Give the name of the intermediate, accumulation workspace.\n"
      " This is the workspace after accumulation but before post-processing "
      "steps.");

  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                      "OutputWorkspace", "", Direction::Output,
                      PropertyMode::Mandatory, LockMode::NoLock),
                  "Name of the processed output workspace.");

  declareProperty(std::make_unique<PropertyWithValue<std::string>>(
                      "LastTimeStamp", "", Direction::Output),
                  "The time stamp of the last event, frame or pulse recorded.\n"
                  "Date/time is in UTC time, in ISO8601 format, e.g. "
                  "2010-09-14T04:20:12.95");
}

//----------------------------------------------------------------------------------------------
/** Copy the LiveDataAlgorithm-specific properties from "other" to "this"
 *
 * @param other :: LiveDataAlgorithm-type algo.
 */
void LiveDataAlgorithm::copyPropertyValuesFrom(const LiveDataAlgorithm &other) {
  std::vector<Property *> props = this->getProperties();
  for (auto prop : props) {
    this->setPropertyValue(prop->name(), other.getPropertyValue(prop->name()));
  }
}

//----------------------------------------------------------------------------------------------
/// @return true if there is a post-processing step
bool LiveDataAlgorithm::hasPostProcessing() const {
  return (!this->getPropertyValue("PostProcessingAlgorithm").empty() ||
          !this->getPropertyValue("PostProcessingScript").empty() ||
          !this->getPropertyValue("PostProcessingScriptFilename").empty());
}

//----------------------------------------------------------------------------------------------
/**
 * Return or create the ILiveListener for this algorithm.
 *
 * If the ILiveListener has not already been created, it creates it using
 * the properties on the algorithm. It then starts the listener
 * by calling the ILiveListener->start(StartTime) method if start is true.
 *
 * @param start Whether to start data acquisition right away
 * @return Shared pointer to interface of this algorithm's LiveListener.
 */
ILiveListener_sptr LiveDataAlgorithm::getLiveListener(bool start) {
  if (m_listener)
    return m_listener;

  // Create a new listener
  m_listener = createLiveListener(start);

  // Start at the given date/time
  if (start)
    m_listener->start(this->getStartTime());

  return m_listener;
}

/**
 * Creates a new instance of a LiveListener based on current values of this
 * algorithm's properties, respecting Facilities.xml defaults as well as any
 * provided properties to override them.
 *
 * The created LiveListener is not stored or cached as this algorithm's
 * LiveListener. This is useful for creating temporary instances.
 *
 * @param connect Whether the created LiveListener should attempt to connect
 *                immediately after creation.
 * @return Shared pointer to interface of created LiveListener instance.
 */
ILiveListener_sptr LiveDataAlgorithm::createLiveListener(bool connect) {
  // Get the LiveListenerInfo from Facilities.xml
  std::string inst_name = this->getPropertyValue("Instrument");
  std::string conn_name = this->getPropertyValue("Connection");

  const auto &inst = ConfigService::Instance().getInstrument(inst_name);
  const auto &conn = inst.liveListenerInfo(conn_name);

  // See if listener and/or address override has been specified
  std::string listener = this->getPropertyValue("Listener");
  if (listener.empty())
    listener = conn.listener();

  std::string address = this->getPropertyValue("Address");
  if (address.empty())
    address = conn.address();

  // Construct new LiveListenerInfo with overrides, if given
  LiveListenerInfo info(listener, address);

  // Create and return
  return LiveListenerFactory::Instance().create(info, connect, this);
}

//----------------------------------------------------------------------------------------------
/** Directly set the LiveListener for this algorithm.
 *
 * @param listener :: ILiveListener_sptr
 */
void LiveDataAlgorithm::setLiveListener(
    Mantid::API::ILiveListener_sptr listener) {
  m_listener = listener;
}

//----------------------------------------------------------------------------------------------
/** @return the value of the StartTime property */
Mantid::Types::Core::DateAndTime LiveDataAlgorithm::getStartTime() const {
  std::string date = getPropertyValue("StartTime");
  if (date.empty())
    return DateAndTime();
  return DateAndTime(date);
}

//----------------------------------------------------------------------------------------------
/** Using the [Post]ProcessingAlgorithm and [Post]ProcessingProperties
 *properties,
 * create and initialize an algorithm for processing.
 *
 * @param postProcessing :: true to create the PostProcessingAlgorithm.
 *        false to create the ProcessingAlgorithm
 * @return shared pointer to the algorithm, ready for execution.
 *         Returns a NULL pointer if no algorithm was chosen.
 */
IAlgorithm_sptr LiveDataAlgorithm::makeAlgorithm(bool postProcessing) {
  std::string prefix;
  if (postProcessing)
    prefix = "Post";

  // Get the name of the algorithm to run
  std::string algoName = this->getPropertyValue(prefix + "ProcessingAlgorithm");
  algoName = Strings::strip(algoName);

  // Get the script to run. Ignored if algo is specified
  std::string script = this->getPropertyValue(prefix + "ProcessingScript");
  script = Strings::strip(script);

  std::string scriptfile =
      this->getPropertyValue(prefix + "ProcessingScriptFilename");

  if (!algoName.empty()) {
    g_log.information() << "Creating algorithm from name \'" << algoName
                        << "\'\n";

    // Properties to pass to algo
    std::string props = this->getPropertyValue(prefix + "ProcessingProperties");

    // Create the UNMANAGED algorithm
    IAlgorithm_sptr alg = this->createChildAlgorithm(algoName);

    // Skip some of the properties when setting
    std::unordered_set<std::string> ignoreProps;
    ignoreProps.insert("InputWorkspace");
    ignoreProps.insert("OutputWorkspace");

    // ...and pass it the properties
    alg->setPropertiesWithString(props, ignoreProps);

    // Warn if someone put both values.
    if (!script.empty())
      g_log.warning() << "Running algorithm " << algoName
                      << " and ignoring the script code in "
                      << prefix + "ProcessingScript\n";
    return alg;
  } else if (!script.empty() || !scriptfile.empty()) {
    // Run a snippet of python
    IAlgorithm_sptr alg = this->createChildAlgorithm("RunPythonScript");
    alg->setLogging(false);
    if (scriptfile.empty()) {
      g_log.information("Creating python algorithm from string");
      alg->setPropertyValue("Code", script);
    } else {
      g_log.information() << "Creating python algorithm from file \'"
                          << scriptfile << "\'\n";
      alg->setPropertyValue("Filename", scriptfile);
    }
    g_log.information("    stack traces will be off by 5"
                      " lines because of boiler-plate");
    return alg;
  } else {
    return IAlgorithm_sptr();
  }
}

//----------------------------------------------------------------------------------------------
/** Validate the properties together */
std::map<std::string, std::string> LiveDataAlgorithm::validateInputs() {
  std::map<std::string, std::string> out;

  const std::string instrument = getPropertyValue("Instrument");
  bool eventListener;
  if (m_listener) {
    eventListener = m_listener->buffersEvents();
  } else {
    eventListener = createLiveListener()->buffersEvents();
  }
  if (!eventListener && getPropertyValue("AccumulationMethod") == "Add") {
    out["AccumulationMethod"] =
        "The " + instrument +
        " live stream produces histograms. Add is not a "
        "sensible accumulation method.";
  }

  if (this->getPropertyValue("OutputWorkspace").empty())
    out["OutputWorkspace"] = "Must specify the OutputWorkspace.";

  // check that only one method was specified for specifying processing
  int numProc = 0;
  if (!this->getPropertyValue("ProcessingAlgorithm").empty())
    numProc += 1;
  if (!this->getPropertyValue("ProcessingScript").empty())
    numProc += 1;
  if (!this->getPropertyValue("ProcessingScriptFilename").empty())
    numProc += 1;
  if (numProc > 1) {
    std::string msg("Only specify one processing method");
    out["ProcessingAlgorithm"] = msg;
    out["ProcessingScript"] = msg;
    out["ProcessingScriptFilename"] = msg;
  }

  // Validate inputs
  if (this->hasPostProcessing()) {
    if (this->getPropertyValue("AccumulationWorkspace").empty())
      out["AccumulationWorkspace"] = "Must specify the AccumulationWorkspace "
                                     "parameter if using PostProcessing.";

    if (this->getPropertyValue("AccumulationWorkspace") ==
        this->getPropertyValue("OutputWorkspace"))
      out["AccumulationWorkspace"] = "The AccumulationWorkspace must be "
                                     "different than the OutputWorkspace, when "
                                     "using PostProcessing.";

    // check that only one method was specified for specifying processing
    int numPostProc = 0;
    if (!this->getPropertyValue("PostProcessingAlgorithm").empty())
      numPostProc += 1;
    if (!this->getPropertyValue("PostProcessingScript").empty())
      numPostProc += 1;
    if (!this->getPropertyValue("PostProcessingScriptFilename").empty())
      numPostProc += 1;
    if (numPostProc > 1) {
      std::string msg("Only specify one post processing method");
      out["PostProcessingAlgorithm"] = msg;
      out["PostProcessingScript"] = msg;
      out["PostProcessingScriptFilename"] = msg;
    }
  }

  // For StartLiveData and MonitorLiveData, make sure another thread is not
  // already using these names
  if (this->name() != "LoadLiveData") {
    /** Validate that the workspace names chosen are not in use already */
    std::string outName = this->getPropertyValue("OutputWorkspace");
    std::string accumName = this->getPropertyValue("AccumulationWorkspace");

    // Check that no other MonitorLiveData thread is running with the same
    // settings
    auto runningLiveAlgorithms =
        AlgorithmManager::Instance().runningInstancesOf("MonitorLiveData");
    auto runningAlgs_it = runningLiveAlgorithms.begin();
    while (runningAlgs_it != runningLiveAlgorithms.end()) {
      IAlgorithm_const_sptr alg = *runningAlgs_it;
      // MonitorLiveData thread that is running, except THIS one.
      if (alg->getAlgorithmID() != this->getAlgorithmID()) {
        if (!accumName.empty() &&
            alg->getPropertyValue("AccumulationWorkspace") == accumName)
          out["AccumulationWorkspace"] +=
              "Another MonitorLiveData thread is running with the same "
              "AccumulationWorkspace.\n"
              "Please specify a different AccumulationWorkspace name.";
        if (alg->getPropertyValue("OutputWorkspace") == outName)
          out["OutputWorkspace"] +=
              "Another MonitorLiveData thread is running with the same "
              "OutputWorkspace.\n"
              "Please specify a different OutputWorkspace name.";
      }
      ++runningAlgs_it;
    }
  }

  return out;
}

} // namespace LiveData
} // namespace Mantid
