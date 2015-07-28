#include "MantidAlgorithms/PDDetermineCharacterizations2.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace Algorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

namespace { // anonymous namespace
// this should match those in LoadPDCharacterizations
const std::vector<std::string> COL_NAMES = {
    "frequency",  // double
    "wavelength", // double
    "bank",       // integer
    "vanadium",   // string
    "container",  // string
    "empty",      // string
    "d_min",      // string
    "d_max",      // string
    "tof_min",    // double
    "tof_max"     // double
};

/*
DEF_INFO = {
    "frequency":0.,
    "wavelength":0.,
    "bank":1,
    "vanadium":0,
    "container":0,
    "empty":0,
    "d_min":"",
    "d_max":"",
    "tof_min":0.,
    "tof_max":0.
    }
    */
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PDDetermineCharacterizations2)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
PDDetermineCharacterizations2::PDDetermineCharacterizations2() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
PDDetermineCharacterizations2::~PDDetermineCharacterizations2() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string PDDetermineCharacterizations2::name() const { return "PDDetermineCharacterizations2"; }

/// Algorithm's version for identification. @see Algorithm::version
int PDDetermineCharacterizations2::version() const { return 2; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string PDDetermineCharacterizations2::category() const {
  return "Workflow/Diffraction/UsesPropertyManager";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string PDDetermineCharacterizations2::summary() const {
  return "Determines the characterizations of a workspace.";
}

//----------------------------------------------------------------------------------------------

std::map<std::string, std::string>
PDDetermineCharacterizations2::validateInputs() {
  std::map<std::string, std::string> result;

  return result;
}

/** Initialize the algorithm's properties.
 */
void PDDetermineCharacterizations2::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "Workspace with logs to help identify frequency and wavelength");

  declareProperty(new WorkspaceProperty<API::ITableWorkspace>(
                      "Characterizations", "", Direction::Input),
                  "Table of characterization information");

  declareProperty("ReductionProperties", "__pd_reduction_properties",
                  "Property manager name for the reduction");

  const std::string defaultMsg =
      " run to use. 0 to use value in table, -1 to not use.";

  declareProperty("BackRun", 0, "Empty container" + defaultMsg);
  declareProperty("NormRun", 0, "Normalization" + defaultMsg);
  declareProperty("NormBackRun", 0, "Normalization background" + defaultMsg);

  const std::vector<std::string> DEFAULT_FREQUENCY_NAMES = {
      "SpeedRequest1", "Speed1", "frequency"};
  declareProperty(new Kernel::ArrayProperty<std::string>(
                      "FrequencyLogNames", DEFAULT_FREQUENCY_NAMES),
                  "Candidate log names for frequency");

  const std::vector<std::string> DEFAULT_WAVELENGTH_NAMES = {"LambdaRequest",
                                                             "lambda"};
  declareProperty(new Kernel::ArrayProperty<std::string>(
                      "WaveLengthLogNames", DEFAULT_WAVELENGTH_NAMES),
                  "Candidate log names for wave length");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void PDDetermineCharacterizations2::exec() {
  // TODO Auto-generated execute stub
}

} // namespace Algorithms
} // namespace Mantid
