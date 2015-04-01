#include "MantidMDAlgorithms/EvaluateMDFunction.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/FunctionDomainMD.h"
#include "MantidAPI/FunctionValues.h"

namespace Mantid {
namespace MDAlgorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EvaluateMDFunction)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
EvaluateMDFunction::EvaluateMDFunction() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
EvaluateMDFunction::~EvaluateMDFunction() {}

//----------------------------------------------------------------------------------------------

/// Algorithm's version for identification. @see Algorithm::version
int EvaluateMDFunction::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string EvaluateMDFunction::category() const {
  return "MDAlgorithms";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string EvaluateMDFunction::summary() const {
  return "Evaluates an MD function on a MD histo workspace.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void EvaluateMDFunction::init() {
  declareProperty(
      new WorkspaceProperty<API::IMDHistoWorkspace>("InputWorkspace", "",
                                                    Direction::Input),
      "An input workspace that provides dimensions for the output.");
  declareProperty(
      new API::FunctionProperty("Function"),
      "Parameters defining the fitting function and its initial values");
  declareProperty(new WorkspaceProperty<API::IMDHistoWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void EvaluateMDFunction::exec() {
  API::IMDHistoWorkspace_sptr input = getProperty("InputWorkspace");

  auto cloner = API::AlgorithmManager::Instance().create("CloneMDWorkspace");
  cloner->initialize();
  cloner->setChild(true);
  cloner->setProperty("InputWorkspace", input);
  cloner->setPropertyValue("OutputWorkspace", "_");
  cloner->execute();

  API::IMDWorkspace_sptr clone = cloner->getProperty("OutputWorkspace");
  API::IMDHistoWorkspace_sptr output =
      boost::dynamic_pointer_cast<API::IMDHistoWorkspace>(clone);

  if (!output)
    throw std::runtime_error("Cannot create output workspace");

  API::IFunction_sptr function = getProperty("Function");
  function->setWorkspace(output);

  API::FunctionDomainMD domain(output);
  API::FunctionValues values(domain);

  function->function(domain, values);

  double *data = values.getPointerToCalculated(0);
  size_t length = values.size();
  double *outputData = output->getSignalArray();
  std::copy(data, data + length, outputData);

  setProperty("OutputWorkspace", output);
}

} // namespace MDAlgorithms
} // namespace Mantid
