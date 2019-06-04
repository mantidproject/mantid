// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidLiveData/StartLiveData.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/AlgorithmProxy.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/System.h"
#include "MantidLiveData/LoadLiveData.h"
#include "MantidLiveData/MonitorLiveData.h"
#include "MantidTypes/Core/DateAndTime.h"

#include <Poco/ActiveResult.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Types::Core::DateAndTime;

namespace Mantid {
namespace LiveData {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(StartLiveData)

namespace {
/// name for a group of properties that get copied from the listener
const char *listenerPropertyGroup = "ListenerProperties";
} // namespace

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string StartLiveData::name() const { return "StartLiveData"; }

/// Algorithm's version for identification. @see Algorithm::version
int StartLiveData::version() const { return 1; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void StartLiveData::init() {
  declareProperty(
      std::make_unique<PropertyWithValue<bool>>("FromNow", true, Direction::Input),
      "Process live data starting from the current time only.");

  declareProperty(
      std::make_unique<PropertyWithValue<bool>>("FromStartOfRun", false,
                                           Direction::Input),
      "Record live data, but go back to the the start of the run and process "
      "all data since then.");

  declareProperty(
      std::make_unique<PropertyWithValue<bool>>("FromTime", false, Direction::Input),
      "Record live data, but go back to a specific time and process all data "
      "since then.\n"
      "You must specify the StartTime property if this is checked.");

  declareProperty(
      std::make_unique<PropertyWithValue<double>>("UpdateEvery", 60.0,
                                             Direction::Input),
      "Frequency of updates, in seconds. Default 60.\n"
      "If you specify 0, MonitorLiveData will not launch and you will get only "
      "one chunk.");

  // Initialize the properties common to LiveDataAlgorithm.
  initProps();

  declareProperty(std::make_unique<AlgorithmProperty>(
                      "MonitorLiveData", boost::make_shared<NullValidator>(),
                      Direction::Output),
                  "A handle to the MonitorLiveData algorithm instance that "
                  "continues to read live data after this algorithm "
                  "completes.");
}

/**
 * After Listener or Connection properties are set, copy any properties that
 * the listener may have to this algorithm.
 *
 * @param propName Name of property that was just set
 */
void StartLiveData::afterPropertySet(const std::string &propName) {
  // If any of these properties change, the listener class might change
  if (propName == "Instrument" || propName == "Listener" ||
      propName == "Connection") {
    // Properties of old listener, if any, need to be removed
    removeListenerProperties();

    // Get temp instance of listener for this instrument with current properties
    auto listener = createLiveListener();

    // Copy over properties of listener to this algorithm
    copyListenerProperties(listener);
  }
}

/**
 * Copies properties from an ILiveListener to this algorithm. This makes them
 * appear in the "listener properties" group on the StartLiveData custom dialog.
 *
 * @param listener ILiveListener from which to copy properties
 */
void StartLiveData::copyListenerProperties(
    const boost::shared_ptr<ILiveListener> &listener) {
  // Add clones of listener's properties to this algorithm
  for (auto listenerProp : listener->getProperties()) {
    auto prop = std::unique_ptr<Property>(listenerProp->clone());
    prop->setGroup(listenerPropertyGroup);
    declareProperty(std::move(prop));
  }
}

/**
 * Removes previously copied ILiveListener properties.
 */
void StartLiveData::removeListenerProperties() {
  std::vector<std::string> propertiesToRemove;

  // Find properties tagged with the listener property group
  for (const auto &prop : getProperties()) {
    if (prop->getGroup() == listenerPropertyGroup) {
      propertiesToRemove.push_back(prop->name());
    }
  }

  // Remove identified properties
  for (const auto &prop : propertiesToRemove) {
    removeProperty(prop);
  }
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void StartLiveData::exec() {
  // Validate the inputs
  bool FromNow = getProperty("FromNow");
  bool FromStartOfRun = getProperty("FromStartOfRun");
  bool FromTime = getProperty("FromTime");
  int numChecked = 0;
  if (FromNow)
    numChecked++;
  if (FromStartOfRun)
    numChecked++;
  if (FromTime)
    numChecked++;

  if (numChecked != 1)
    throw std::runtime_error(
        "Please check exactly one of FromNow, FromStartOfRun, FromTime.");

  // Adjust the StartTime if you are starting from run/now.
  if (FromNow)
    this->setPropertyValue("StartTime", "1990-01-01T00:00:00");
  // Use the epoch value for the start time, as documented in
  // ILiveListener::start.
  else if (FromStartOfRun)
    // At this point, we don't know when the start of the run was.  Set the
    // requested time
    // to 1 second past the epoch, which is sure to be before that. We're then
    // relying
    // on the concrete live listener to never give data from before the current
    // run.
    // So far, any that give historical data behave like this but there's no way
    // to enforce it.
    this->setPropertyValue("StartTime", "1990-01-01T00:00:01");
  else {
    // Validate the StartTime property.  Don't allow times from the future
    DateAndTime reqStartTime(this->getPropertyValue("StartTime"));
    // DateAndTime will throw an exception if it can't interpret the string, so
    // we don't need to test for that condition.

    // check for a requested time in the future
    if (reqStartTime > DateAndTime::getCurrentTime()) {
      g_log.error(
          "Requested start time in the future. Resetting to current time.");
      this->setPropertyValue("StartTime", "1990-01-01T00:00:00");
    }
  }

  // Get the listener (and start listening) as early as possible
  ILiveListener_sptr listener = this->getLiveListener();

  // Issue a warning if historical data has been requested but the listener does
  // not support it.
  // This is only for event data; histogram data is by its nature historical and
  // specifying a time is meaningless.
  if (!FromNow && !listener->supportsHistory() && listener->buffersEvents()) {
    g_log.error("Requested start time is in the past, but this instrument does "
                "not support historical data. "
                "The effective start time is therefore 'now'.");
  }

  auto loadAlg = boost::dynamic_pointer_cast<LoadLiveData>(
      createChildAlgorithm("LoadLiveData"));
  if (!loadAlg)
    throw std::logic_error(
        "Error creating LoadLiveData - contact the Mantid developer team");
  // Copy settings from THIS to LoadAlg
  loadAlg->copyPropertyValuesFrom(*this);
  // Force replacing the output workspace on the first run, to clear out old
  // junk.
  loadAlg->setPropertyValue("AccumulationMethod", "Replace");
  // Give the listener directly to LoadLiveData (don't re-create it)
  loadAlg->setLiveListener(listener);

  // Run the LoadLiveData for the first time.
  loadAlg->executeAsChildAlg();

  // Copy the output workspace properties from LoadLiveData
  Workspace_sptr outWS = loadAlg->getProperty("OutputWorkspace");
  this->setProperty("OutputWorkspace", outWS);
  Workspace_sptr accumWS = loadAlg->getProperty("AccumulationWorkspace");
  this->setProperty("AccumulationWorkspace", accumWS);

  double UpdateEvery = this->getProperty("UpdateEvery");
  if (UpdateEvery > 0) {
    // Create the MonitorLiveData but DO NOT make a AlgorithmProxy to it
    IAlgorithm_sptr algBase =
        AlgorithmManager::Instance().create("MonitorLiveData", -1, false);
    MonitorLiveData *monitorAlg =
        dynamic_cast<MonitorLiveData *>(algBase.get());

    if (!monitorAlg)
      throw std::runtime_error("Error creating the MonitorLiveData algorithm");

    // Copy settings from THIS to monitorAlg
    monitorAlg->initialize();
    monitorAlg->copyPropertyValuesFrom(*this);
    monitorAlg->setProperty("UpdateEvery", UpdateEvery);

    // Give the listener directly to LoadLiveData (don't re-create it)
    monitorAlg->setLiveListener(listener);

    // Check for possible cancellation
    interruption_point();
    // Launch asyncronously
    monitorAlg->executeAsync();

    // Set the output property that passes back a handle to the ongoing live
    // algorithm
    setProperty("MonitorLiveData", algBase);
  }
}

} // namespace LiveData
} // namespace Mantid
