// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SetInstrumentParameter.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Strings.h"

#include <boost/algorithm/string.hpp>
#include <deque>

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SetInstrumentParameter)

using namespace Kernel;
using namespace Geometry;
using namespace API;

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string SetInstrumentParameter::name() const { return "SetInstrumentParameter"; }

/// Algorithm's version for identification. @see Algorithm::version
int SetInstrumentParameter::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SetInstrumentParameter::category() const { return "DataHandling\\Instrument"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SetInstrumentParameter::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("Workspace", "", Direction::InOut,
                                                                 std::make_shared<InstrumentValidator>()),
                  "Workspace to add the log entry to");
  declareProperty("ComponentName", "",
                  "The name of the component to attach "
                  "the parameter to. Default: the whole "
                  "instrument");
  declareProperty(std::make_unique<ArrayProperty<detid_t>>("DetectorList"),
                  "The detector ID list to attach the parameter to. If set "
                  "this will override any ComponentName");
  declareProperty("ParameterName", "", std::make_shared<MandatoryValidator<std::string>>(),
                  "The name that will identify the parameter");

  std::vector<std::string> propOptions{"String", "Number", "Bool"};
  declareProperty("ParameterType", "String", std::make_shared<StringListValidator>(propOptions),
                  "The type that the parameter value will be.");

  declareProperty("Value", "", "The content of the Parameter");
}

/// @copydoc Algorithm::validateInputs()
std::map<std::string, std::string> SetInstrumentParameter::validateInputs() {
  std::map<std::string, std::string> errors;
  const std::set<std::string> allowedBoolValues{"1", "0", "true", "false"};
  const std::string type = getPropertyValue("ParameterType");
  std::string val = getPropertyValue("Value");

  if (type == "Bool") {
    boost::algorithm::to_lower(val);
    if (allowedBoolValues.find(val) == allowedBoolValues.end()) {
      errors["Value"] = "Invalid value for Bool type.";
    }
  } else if (type == "Number") {
    int intVal;
    double dblVal;
    if (!Strings::convert(val, intVal) && !Strings::convert(val, dblVal)) {
      errors["Value"] = "Invalid value for Number type.";
    }
  }

  return errors;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SetInstrumentParameter::exec() {
  // A pointer to the workspace to add a log to
  Workspace_sptr ws = getProperty("Workspace");
  MatrixWorkspace_sptr inputW = std::dynamic_pointer_cast<MatrixWorkspace>(ws);
  DataObjects::PeaksWorkspace_sptr inputP = std::dynamic_pointer_cast<DataObjects::PeaksWorkspace>(ws);
  ExperimentInfo_sptr experimentInfo;
  if (inputW) {
    experimentInfo = inputW;
  } else if (inputP) {
    experimentInfo = inputP;
  } else {
    throw std::runtime_error("Could not get a valid instrument from the "
                             "workspace which does not seem to be valid as "
                             "input (must be either MatrixWorkspace or "
                             "PeaksWorkspace");
  }
  auto &componentInfo = experimentInfo->mutableComponentInfo();

  // get the data that the user wants to add
  std::string cmptName = getProperty("ComponentName");
  const std::vector<detid_t> detectorList = getProperty("DetectorList");
  std::string paramName = getProperty("ParameterName");
  std::string paramType = getProperty("ParameterType");
  std::string paramValue = getPropertyValue("Value");

  Strings::strip(cmptName);
  Strings::strip(paramName);
  Strings::strip(paramValue);

  // set default to whole instrument
  std::vector<size_t> componentIndices{componentInfo.root()};
  if (!detectorList.empty()) {
    const auto &detectorInfo = experimentInfo->detectorInfo();
    componentIndices.clear();
    for (const auto detID : detectorList) {
      try {
        // Detector indices are also component indices
        componentIndices.emplace_back(detectorInfo.indexOf(detID));
      } catch (const std::out_of_range &) {
        throw Kernel::Exception::NotFoundError("Instrument: Detector with ID " + std::to_string(detID) + " not found.",
                                               "");
      }
    }
  } else if (cmptName.length() > 0) {
    // get all matching cmpts
    componentIndices = componentsWithName(componentInfo, cmptName);
  }

  if (componentIndices.empty()) {
    g_log.warning("Could not find the component requested.");
  }
  for (const auto componentIndex : componentIndices) {
    addParameter(componentInfo, componentIndex, paramName, paramType, paramValue);
  }
}

/** The components with the given name, as Instrument::getAllComponentsWithName() finds them: in breadth-first
 * order, and without searching inside a component that matches.
 *  @param componentInfo  The instrument to search
 *  @param name  The name to look for
 *  @return The indices of the components with that name
 */
std::vector<size_t> SetInstrumentParameter::componentsWithName(const ComponentInfo &componentInfo,
                                                               const std::string &name) {
  std::vector<size_t> matches;
  const auto root = componentInfo.root();
  if (componentInfo.name(root) == name) {
    matches.emplace_back(root);
  }
  std::deque<size_t> toSearch{root};
  while (!toSearch.empty()) {
    const auto index = toSearch.front();
    toSearch.pop_front();
    for (const auto child : componentInfo.children(index)) {
      if (componentInfo.name(child) == name) {
        matches.emplace_back(child);
      } else {
        toSearch.emplace_back(child);
      }
    }
  }
  return matches;
}

/** Adds a parameter to the component
 *  @param componentInfo  The instrument to add the parameter to
 *  @param componentIndex  The index of the component to add the parameter to
 *  @param paramName  The parameter name to use
 *  @param paramType  The parameter type
 *  @param paramValue  The parameter value as a string
 */
void SetInstrumentParameter::addParameter(ComponentInfo &componentInfo, const size_t componentIndex,
                                          const std::string &paramName, const std::string &paramType,
                                          const std::string &paramValue) const {

  // remove existing parameters first
  componentInfo.clearParameter(componentIndex, paramName);
  // then add the new one
  if (paramType == "String") {
    componentInfo.addString(componentIndex, paramName, paramValue);
  } else if (paramType == "Number") {
    int intVal;
    if (Strings::convert(paramValue, intVal)) {
      componentInfo.addInt(componentIndex, paramName, intVal);
    } else {
      double dblVal;
      Strings::convert(paramValue, dblVal);
      componentInfo.addDouble(componentIndex, paramName, dblVal);
    }
  } else if (paramType == "Bool") {
    std::string paramValueLower(paramValue);
    boost::algorithm::to_lower(paramValueLower);
    bool paramBoolValue = (paramValueLower == "true" || paramValue == "1");
    componentInfo.addBool(componentIndex, paramName, paramBoolValue);
  }
}

} // namespace Mantid::Algorithms
