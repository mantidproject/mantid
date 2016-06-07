#include "MantidAlgorithms/SetInstrumentParameter.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Strings.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SetInstrumentParameter)

using namespace Kernel;
using namespace Geometry;
using namespace API;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SetInstrumentParameter::SetInstrumentParameter() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SetInstrumentParameter::~SetInstrumentParameter() = default;

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
  declareProperty(make_unique<WorkspaceProperty<>>(
                      "Workspace", "", Direction::InOut,
                      boost::make_shared<InstrumentValidator>()),
                  "Workspace to add the log entry to");
  declareProperty("ComponentName", "", "The name of the component to attach "
                                       "the parameter to. Default: the whole "
                                       "instrument");
  declareProperty(make_unique<ArrayProperty<detid_t>>("DetectorList"),
                  "The detector ID list to attach the parameter to. If set "
                  "this will override any ComponentName");
  declareProperty("ParameterName", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The name that will identify the parameter");

  std::vector<std::string> propOptions{"String", "Number"};
  declareProperty("ParameterType", "String",
                  boost::make_shared<StringListValidator>(propOptions),
                  "The type that the parameter value will be.");

  declareProperty("Value", "", "The content of the Parameter");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SetInstrumentParameter::exec() {
  // A pointer to the workspace to add a log to
  MatrixWorkspace_sptr ws = getProperty("Workspace");

  // get the data that the user wants to add
  std::string cmptName = getProperty("ComponentName");
  const std::vector<detid_t> detectorList = getProperty("DetectorList");
  std::string paramName = getProperty("ParameterName");
  std::string paramType = getProperty("ParameterType");
  std::string paramValue = getPropertyValue("Value");

  Strings::strip(cmptName);
  Strings::strip(paramName);
  Strings::strip(paramValue);

  auto inst = ws->getInstrument();
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

  auto &paramMap = ws->instrumentParameters();
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
    bool valueIsInt(false);
    int intVal;
    double dblVal;
    if (Strings::convert(paramValue, intVal)) {
      valueIsInt = true;
    } else if (!Strings::convert(paramValue, dblVal)) {
      throw std::invalid_argument("Error interpreting string '" + paramValue +
                                  "' as a number.");
    }

    if (valueIsInt)
      pmap.addInt(cmptId, paramName, intVal);
    else
      pmap.addDouble(cmptId, paramName, dblVal);
  } else {
    throw std::invalid_argument("Unknown Parameter Type " + paramType);
  }
}

} // namespace Algorithms
} // namespace Mantid
