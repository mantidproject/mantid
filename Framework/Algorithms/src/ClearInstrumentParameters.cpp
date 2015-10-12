#include "MantidAlgorithms/ClearInstrumentParameters.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ClearInstrumentParameters)

using namespace Kernel;
using namespace API;
using namespace Geometry;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ClearInstrumentParameters::ClearInstrumentParameters() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ClearInstrumentParameters::~ClearInstrumentParameters() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string ClearInstrumentParameters::name() const {
  return "ClearInstrumentParameters";
}

/// Summary of the algorithm's purpose. @see Algorithm::summary
const std::string ClearInstrumentParameters::summary() const {
  return "Clears all the parameters associated with a workspace's instrument.";
}

/// Algorithm's version for identification. @see Algorithm::version
int ClearInstrumentParameters::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ClearInstrumentParameters::category() const {
  return "DataHandling\\Instrument";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ClearInstrumentParameters::init() {
  declareProperty(
      new WorkspaceProperty<>("Workspace", "", Direction::InOut,
                              boost::make_shared<InstrumentValidator>()),
      "Workspace whose instrument parameters are to be cleared.");

  declareProperty(
      "LocationParameters", true,
      "Clear the location parameters used to calibrate the instrument.",
      Direction::Input);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ClearInstrumentParameters::exec() {
  const MatrixWorkspace_const_sptr ws = getProperty("Workspace");
  const bool clearLocationParams = getProperty("LocationParameters");

  const Instrument_const_sptr instrument = ws->getInstrument();
  const ParameterMap_sptr params = instrument->getParameterMap();

  ParameterMap::pmap paramsToKeep;

  // Go through all the parameters, keep a hold of any we don't want to clear.
  for (auto paramIt = params->begin(); paramIt != params->end(); ++paramIt) {
    // Are we keeping the location parameters?
    const std::string pName = (*paramIt).second->name();
    if (!clearLocationParams &&
        (pName == "x" || pName == "y" || pName == "z" ||
         pName == "r-position" || pName == "t-position" ||
         pName == "p-position" || pName == "rotx" || pName == "roty" ||
         pName == "rotz")) {
      paramsToKeep.insert(*paramIt);
    }
  }

  // Clear out the parameter map
  params->clear();

  // Add any parameters we're keeping back into the parameter map.
  for (auto paramIt = paramsToKeep.begin(); paramIt != paramsToKeep.end();
       ++paramIt) {
    params->add((*paramIt).first, (*paramIt).second);
  }
}

} // namespace Algorithms
} // namespace Mantid
