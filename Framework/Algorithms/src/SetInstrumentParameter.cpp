// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SetInstrumentParameter.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Strings.h"

#include <boost/algorithm/string.hpp>

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SetInstrumentParameter)

using namespace Kernel;
using namespace Geometry;
using namespace API;

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string SetInstrumentParameter::name() const {
  return "SetInstrumentParameter";
}

/// Algorithm's version for identification. @see Algorithm::version
int SetInstrumentParameter::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SetInstrumentParameter::category() const {
  return "DataHandling\\Instrument";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SetInstrumentParameter::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                      "Workspace", "", Direction::InOut,
                      boost::make_shared<InstrumentValidator>()),
                  "Workspace to add the log entry to");
  declareProperty("ComponentName", "",
                  "The name of the component to attach "
                  "the parameter to. Default: the whole "
                  "instrument");
  declareProperty(std::make_unique<ArrayProperty<detid_t>>("DetectorList"),
                  "The detector ID list to attach the parameter to. If set "
                  "this will override any ComponentName");
  declareProperty("ParameterName", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The name that will identify the parameter");

  std::vector<std::string> propOptions{"String", "Number", "Bool"};
  declareProperty("ParameterType", "String",
                  boost::make_shared<StringListValidator>(propOptions),
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
  MatrixWorkspace_sptr inputW =
      boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
  DataObjects::PeaksWorkspace_sptr inputP =
      boost::dynamic_pointer_cast<DataObjects::PeaksWorkspace>(ws);
  // Get some stuff from the input workspace
  Instrument_sptr inst;
  if (inputW) {
    inst = boost::const_pointer_cast<Instrument>(inputW->getInstrument());
    if (!inst)
      throw std::runtime_error("Could not get a valid instrument from the "
                               "MatrixWorkspace provided as input");
  } else if (inputP) {
    inst = boost::const_pointer_cast<Instrument>(inputP->getInstrument());
    if (!inst)
      throw std::runtime_error("Could not get a valid instrument from the "
                               "PeaksWorkspace provided as input");
  } else {
    throw std::runtime_error("Could not get a valid instrument from the "
                             "workspace which does not seem to be valid as "
                             "input (must be either MatrixWorkspace or "
                             "PeaksWorkspace");
  }

  // get the data that the user wants to add
  std::string cmptName = getProperty("ComponentName");
  const std::vector<detid_t> detectorList = getProperty("DetectorList");
  std::string paramName = getProperty("ParameterName");
  std::string paramType = getProperty("ParameterType");
  std::string paramValue = getPropertyValue("Value");

  Strings::strip(cmptName);
  Strings::strip(paramName);
  Strings::strip(paramValue);

  std::vector<IDetector_const_sptr> dets;
  std::vector<IComponent_const_sptr> cmptList;
  // set default to whole instrument
  cmptList.push_back(inst);

  if (!detectorList.empty()) {
    dets = inst->getDetectors(detectorList);
  } else if (cmptName.length() > 0) {
    // get all matching cmpts
    cmptList = inst->getAllComponentsWithName(cmptName);
  }

  if (inputW) {
    auto &paramMap = inputW->instrumentParameters();
    if (!dets.empty()) {
      for (auto &det : dets) {
        addParameter(paramMap, det.get(), paramName, paramType, paramValue);
      }
    } else {
      if (!cmptList.empty()) {
        for (auto &cmpt : cmptList) {
          addParameter(paramMap, cmpt.get(), paramName, paramType, paramValue);
        }
      } else {
        g_log.warning("Could not find the component requested.");
      }
    }
  }
  if (inputP) {
    auto &paramMap = inputP->instrumentParameters();
    if (!dets.empty()) {
      for (auto &det : dets) {
        addParameter(paramMap, det.get(), paramName, paramType, paramValue);
      }
    } else {
      if (!cmptList.empty()) {
        for (auto &cmpt : cmptList) {
          addParameter(paramMap, cmpt.get(), paramName, paramType, paramValue);
        }
      } else {
        g_log.warning("Could not find the component requested.");
      }
    }
  }
}

/** Adds a parameter to the component
 *  @param pmap  The parameter map to use
 *  @param cmptId  The component id to add the the parameter to
 *  @param paramName  The parameter name to use
 *  @param paramType  The parameter type
 *  @param paramValue  The parameter value as a string
 */
void SetInstrumentParameter::addParameter(
    Mantid::Geometry::ParameterMap &pmap,
    const Mantid::Geometry::IComponent *cmptId, const std::string &paramName,
    const std::string &paramType, const std::string &paramValue) const {

  // remove existing parameters first
  pmap.clearParametersByName(paramName, cmptId);
  // then add the new one
  if (paramType == "String") {
    pmap.addString(cmptId, paramName, paramValue);
  } else if (paramType == "Number") {
    int intVal;
    if (Strings::convert(paramValue, intVal)) {
      pmap.addInt(cmptId, paramName, intVal);
    } else {
      double dblVal;
      Strings::convert(paramValue, dblVal);
      pmap.addDouble(cmptId, paramName, dblVal);
    }
  } else if (paramType == "Bool") {
    std::string paramValueLower(paramValue);
    boost::algorithm::to_lower(paramValueLower);
    bool paramBoolValue = (paramValueLower == "true" || paramValue == "1");
    pmap.addBool(cmptId, paramName, paramBoolValue);
  }
}

} // namespace Algorithms
} // namespace Mantid
