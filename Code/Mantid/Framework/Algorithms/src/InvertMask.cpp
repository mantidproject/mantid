#include "MantidAlgorithms/InvertMask.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/MaskWorkspace.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(InvertMask)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
InvertMask::InvertMask() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
InvertMask::~InvertMask() {}

void InvertMask::init() {
  this->declareProperty(new API::WorkspaceProperty<DataObjects::MaskWorkspace>(
                            "InputWorkspace", "Anonymous", Direction::Input),
                        "MaskWorkspace to be inverted. ");

  this->declareProperty(
      new API::WorkspaceProperty<DataObjects::MaskWorkspace>(
          "OutputWorkspace", "AnonynmousOutput", Direction::Output),
      "MaskWorkspace has inverted bits from input MaskWorkspace.");

  return;
}

void InvertMask::exec() {
  // 1. Get input
  DataObjects::MaskWorkspace_const_sptr inWS =
      this->getProperty("InputWorkspace");
  if (!inWS) {
    throw std::invalid_argument("InputWorkspace is not a MaskWorkspace.");
  }

  // 2. Do Invert by calling Child Algorithm
  API::IAlgorithm_sptr invert =
      createChildAlgorithm("BinaryOperateMasks", 0.0, 1.0, true);
  invert->setPropertyValue("InputWorkspace1", inWS->name());
  invert->setProperty("OperationType", "NOT");
  invert->setProperty("OutputWorkspace", "tempws");

  invert->execute();

  if (!invert->isExecuted()) {
    g_log.error() << "ChildAlgorithm BinaryOperateMask() cannot be executed. "
                  << std::endl;
    throw std::runtime_error(
        "ChildAlgorithm BinaryOperateMask() cannot be executed. ");
  }

  DataObjects::MaskWorkspace_sptr outputws =
      invert->getProperty("OutputWorkspace");
  if (!outputws) {
    throw std::runtime_error("Output Workspace is not a MaskWorkspace. ");
  }

  // 3. Set
  this->setProperty("OutputWorkspace", outputws);

  return;
}

} // namespace Mantid
} // namespace Algorithms
