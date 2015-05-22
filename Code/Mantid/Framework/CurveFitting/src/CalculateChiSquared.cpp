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
  declareProperty("ChiSquared", 0.0, "Output value of chi squared.", Direction::Output);
  declareProperty("ChiSquaredDividedByDOF", 0.0,
                  "Output value of chi squared divided by the "
                  "number of degrees of freedom (NofData "
                  "- nOfParams).", Direction::Output);
  declareProperty("ChiSquaredWeighted", 0.0, "Output value of weighted chi squared.", Direction::Output);
  declareProperty("ChiSquaredWeightedDividedByDOF", 0.0,
                  "Output value of weighted chi squared divided by the "
                  "number of degrees of freedom (NofData "
                  "- nOfParams).", Direction::Output);
}

//----------------------------------------------------------------------------------------------
/// Execute the algorithm.
void CalculateChiSquared::execConcrete() {

  // Function may need some preparation.
  m_function->setUpForFit();

  API::FunctionDomain_sptr domain;
  API::FunctionValues_sptr values;
  m_domainCreator->createDomain(domain, values);

  // Do something with the function which may depend on workspace.
  m_domainCreator->initFunction(m_function);
  
  // Calculate function values.
  m_function->function(*domain, *values);

  // Get the number of free fitting parameters
  size_t nParams = 0;
  for (size_t i = 0; i < m_function->nParams(); ++i) {
    if (!m_function->isFixed(i))
      nParams += 1;
  }
  double dof = - static_cast<double>(nParams);

  // Calculate the chi squared.
  double chiSquared = 0.0;
  double chiSquaredWeighted = 0.0;
  for(size_t i = 0; i < values->size(); ++i) {
    auto weight = values->getFitWeight(i);
    if (weight > 0.0) {
      double tmp = values->getFitData(i) - values->getCalculated(i);
      chiSquared += tmp * tmp;
      tmp *= weight;
      chiSquaredWeighted += tmp * tmp;
      dof += 1.0;
    }
  }
  g_log.notice() << "Chi squared " << chiSquared << std::endl;
  g_log.notice() << "Chi squared weighted " << chiSquaredWeighted << std::endl;

  // Store the result.
  setProperty("ChiSquared", chiSquared);
  setProperty("chiSquaredWeighted", chiSquaredWeighted);

  // Divide by the DOF
  g_log.debug() << "DOF " << dof << std::endl;
  if (dof <= 0.0) {
    dof = 1.0;
    g_log.warning() << "DOF has a non-positive value, changing to 1.0."
                    << std::endl;
  }
  chiSquared /= dof;
  chiSquaredWeighted /= dof;
  g_log.notice() << "Chi squared / DOF " << chiSquared << std::endl;
  g_log.notice() << "Chi squared weighed / DOF " << chiSquaredWeighted << std::endl;

  // Store the result.
  setProperty("ChiSquaredDividedByDOF", chiSquared);
  setProperty("ChiSquaredWeightedDividedByDOF", chiSquaredWeighted);
}

} // namespace CurveFitting
} // namespace Mantid
