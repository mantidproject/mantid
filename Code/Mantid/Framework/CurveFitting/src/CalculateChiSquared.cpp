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
  declareProperty("DivideByDOF", false, "Flag to divide the chi^2 by the "
                                        "number of degrees of freedom (NofData "
                                        "- nOfParams).");
  declareProperty("ChiSquared", 0.0, "Output value of chi squared.");
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

  // Calculate the chi squared.
  double chiSquared = 0.0;
  for(size_t i = 0; i < values->size(); ++i) {
    if (values->getFitWeight(i) > 0.0) {
      double tmp = values->getFitData(i) - values->getCalculated(i);
      chiSquared += tmp * tmp;
    }
  }
  g_log.debug() << "Chi squared " << chiSquared << std::endl;

  // Divide by the DOF
  bool divideByDOF = getProperty("DivideByDOF");
  if (divideByDOF) {
    // Get the number of free fitting parameters
    size_t nParams = 0;
    for(size_t i = 0; i < m_function->nParams(); ++i) {
      if ( !m_function->isFixed(i) ) nParams += 1;
    }
    double dof = static_cast<double>(domain->size()) - static_cast<double>(nParams);
    g_log.debug() << "DOF " << dof << std::endl;
    if (dof <= 0.0) {
      dof = 1.0;
      g_log.warning() << "DOF has a non-positive value, changing to 1,0." << std::endl;
    }
    chiSquared /= dof;
    g_log.debug() << "Chi squared / DOF " << chiSquared << std::endl;
  }
  // Store the result.
  setProperty("ChiSquared", chiSquared);
}

} // namespace CurveFitting
} // namespace Mantid
