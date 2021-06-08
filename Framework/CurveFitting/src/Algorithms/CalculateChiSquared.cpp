// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Algorithms/CalculateChiSquared.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

using namespace Kernel;
using namespace API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculateChiSquared)

/// Algorithms name for identification. @see Algorithm::name
const std::string CalculateChiSquared::name() const { return "CalculateChiSquared"; }

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
                  "- nOfParams).",
                  Direction::Output);
  declareProperty("ChiSquaredDividedByNData", 0.0,
                  "Output value of chi squared divided by the "
                  "number of data points).",
                  Direction::Output);
  declareProperty("ChiSquaredWeighted", 0.0, "Output value of weighted chi squared.", Direction::Output);
  declareProperty("ChiSquaredWeightedDividedByDOF", 0.0,
                  "Output value of weighted chi squared divided by the "
                  "number of degrees of freedom (NofData "
                  "- nOfParams).",
                  Direction::Output);
  declareProperty("ChiSquaredWeightedDividedByNData", 0.0,
                  "Output value of weighted chi squared divided by the "
                  "number of  data points).",
                  Direction::Output);
  declareProperty("Weighted", false,
                  "Option to use the weighted chi squared "
                  "in error estimation. Default is false.");
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

  // Get the number of free fitting parameters
  size_t nParams = 0;
  for (size_t i = 0; i < m_function->nParams(); ++i) {
    if (m_function->isActive(i))
      nParams += 1;
  }

  // Calculate function values.
  m_function->function(*domain, *values);

  // Calculate the chi squared.
  double chiSquared = 0.0;
  double chiSquaredWeighted = 0.0;
  double dof = 0.0;
  calcChiSquared(*m_function, nParams, *domain, *values, chiSquared, chiSquaredWeighted, dof);
  g_log.notice() << "Chi squared " << chiSquared << "\n"
                 << "Chi squared weighted " << chiSquaredWeighted << "\n";

  // Store the result.
  setProperty("ChiSquared", chiSquared);
  setProperty("chiSquaredWeighted", chiSquaredWeighted);

  // Divided by NParams
  double nData = dof + static_cast<double>(nParams);
  const double chiSquaredNData = chiSquared / nData;
  const double chiSquaredWeightedNData = chiSquaredWeighted / nData;
  g_log.notice() << "Chi squared / NData " << chiSquaredNData << "\n"
                 << "Chi squared weighed / NData " << chiSquaredWeightedNData << "\n"
                 << "NParams " << nParams << "\n";

  // Store the result.
  setProperty("ChiSquaredDividedByNData", chiSquaredNData);
  setProperty("ChiSquaredWeightedDividedByNData", chiSquaredWeightedNData);

  // Divided by the DOF
  if (dof <= 0.0) {
    dof = 1.0;
    g_log.warning("DOF has a non-positive value, changing to 1.0");
  }
  const double chiSquaredDOF = chiSquared / dof;
  const double chiSquaredWeightedDOF = chiSquaredWeighted / dof;
  g_log.notice() << "Chi squared / DOF " << chiSquaredDOF << "\n"
                 << "Chi squared weighed / DOF " << chiSquaredWeightedDOF << "\n"
                 << "DOF " << dof << "\n";

  // Store the result.
  setProperty("ChiSquaredDividedByDOF", chiSquaredDOF);
  setProperty("ChiSquaredWeightedDividedByDOF", chiSquaredWeightedDOF);
}

//----------------------------------------------------------------------------------------------

/// Caclculate the chi squared, weighted chi squared and the number of degrees
/// of freedom.
/// @param fun :: Function's domain.
/// @param nParams :: Number of free fitting parameters.
/// @param domain :: Function's domain
/// @param values :: Function's values
/// @param chiSquared :: unweighted chi squared
/// @param chiSquaredWeighted :: weighted chi squared
/// @param dof :: degrees of freedom
void CalculateChiSquared::calcChiSquared(const API::IFunction &fun, size_t nParams, const API::FunctionDomain &domain,
                                         API::FunctionValues &values, double &chiSquared, double &chiSquaredWeighted,
                                         double &dof) {

  // Calculate function values.
  fun.function(domain, values);

  // Calculate the chi squared.
  chiSquared = 0.0;
  chiSquaredWeighted = 0.0;
  dof = -static_cast<double>(nParams);
  for (size_t i = 0; i < values.size(); ++i) {
    auto weight = values.getFitWeight(i);
    if (weight > 0.0) {
      double tmp = values.getFitData(i) - values.getCalculated(i);
      chiSquared += tmp * tmp;
      tmp *= weight;
      chiSquaredWeighted += tmp * tmp;
      dof += 1.0;
    }
  }
  if (dof <= 0.0) {
    dof = 1.0;
  }
}

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
