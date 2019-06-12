// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Algorithms/EvaluateFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

using namespace Mantid::Kernel;
using namespace Mantid::API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EvaluateFunction)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string EvaluateFunction::name() const { return "EvaluateFunction"; }

/// Algorithm's version for identification. @see Algorithm::version
int EvaluateFunction::version() const { return 1; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string EvaluateFunction::summary() const {
  return "Evaluate a function on a workspace.";
}

//----------------------------------------------------------------------------------------------
/// Initialize the algorithm's properties.
void EvaluateFunction::initConcrete() {
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/// Execute the algorithm.
void EvaluateFunction::execConcrete() {

  // Function may need some preparation.
  m_function->setUpForFit();

  API::FunctionDomain_sptr domain;
  API::FunctionValues_sptr values;
  m_domainCreator->createDomain(domain, values);

  // Do something with the function which may depend on workspace.
  m_domainCreator->initFunction(m_function);

  // Apply any ties.
  m_function->applyTies();

  // Calculate function values.
  m_function->function(*domain, *values);

  // Gnegerate the output workspace
  auto outputWS = m_domainCreator->createOutputWorkspace("", m_function, domain,
                                                         values, "");

  // Store the result.
  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
