#include "MantidCurveFitting/CalculateChiSquared.h"

namespace Mantid {
namespace CurveFitting {

using namespace Mantid::Kernel;
using namespace Mantid::API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculateChiSquared)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
CalculateChiSquared::CalculateChiSquared() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
CalculateChiSquared::~CalculateChiSquared() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CalculateChiSquared::name() const {
  return "CalculateChiSquared";
}

/// Algorithm's version for identification. @see Algorithm::version
int CalculateChiSquared::version() const { return 1; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CalculateChiSquared::summary() const {
  return "Calculate chi squared for a function and a data set in a workspace.";
}

//----------------------------------------------------------------------------------------------
/// Initialize the algorithm's properties.
void CalculateChiSquared::initConcrete() {
  declareProperty("ChiSquared", 0.0, "Output value of chi squared.");
}

//----------------------------------------------------------------------------------------------
/// Execute the algorithm.
void CalculateChiSquared::execConcrete() {
  
}

} // namespace CurveFitting
} // namespace Mantid