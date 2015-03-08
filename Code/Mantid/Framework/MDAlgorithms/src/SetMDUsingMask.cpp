#include "MantidMDAlgorithms/SetMDUsingMask.h"
#include "MantidKernel/System.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include <float.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SetMDUsingMask)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SetMDUsingMask::SetMDUsingMask() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SetMDUsingMask::~SetMDUsingMask() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string SetMDUsingMask::name() const { return "SetMDUsingMask"; };

/// Algorithm's version for identification. @see Algorithm::version
int SetMDUsingMask::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string SetMDUsingMask::category() const {
  return "MDAlgorithms\\MDArithmetic";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SetMDUsingMask::init() {
  declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("InputWorkspace", "",
                                                           Direction::Input),
                  "An input MDHistoWorkspace.");
  declareProperty(
      new WorkspaceProperty<IMDHistoWorkspace>("MaskWorkspace", "",
                                               Direction::Input),
      "A mask MDHistoWorkspace, where true indicates where to set the value.");

  declareProperty(
      new WorkspaceProperty<IMDHistoWorkspace>(
          "ValueWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Workspace to copy to the output workspace over the input. Optional - "
      "specify this or Value.");

  declareProperty("Value", DBL_MAX, "Single number to set in the output "
                                    "workspace. Optional - specify this or "
                                    "ValueWorkspace");

  declareProperty(new WorkspaceProperty<IMDHistoWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output MDHistoWorkspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SetMDUsingMask::exec() {
  IMDHistoWorkspace_sptr inIWS = getProperty("InputWorkspace");
  IMDHistoWorkspace_sptr maskIWS = getProperty("MaskWorkspace");
  IMDHistoWorkspace_sptr outIWS = getProperty("OutputWorkspace");
  IMDHistoWorkspace_sptr valueIWS = getProperty("ValueWorkspace");
  double value = getProperty("Value");

  bool useValueWS = (value == DBL_MAX);

  if (useValueWS && !valueIWS)
    throw std::invalid_argument(
        "You must specify either ValueWorkspace or Value.");
  if (!useValueWS && valueIWS)
    throw std::invalid_argument(
        "You must specify either ValueWorkspace or Value, not both!");

  if (maskIWS->getNumDims() != inIWS->getNumDims())
    throw std::invalid_argument(
        "Input and Mask workspace need to have the same number of dimensions.");
  if (maskIWS->getNPoints() != inIWS->getNPoints())
    throw std::invalid_argument(
        "Input and Mask workspace need to have the same number of points.");
  if (valueIWS) {
    if (maskIWS->getNumDims() != valueIWS->getNumDims())
      throw std::invalid_argument("Input and Value workspace need to have the "
                                  "same number of dimensions.");
    if (maskIWS->getNPoints() != valueIWS->getNPoints())
      throw std::invalid_argument(
          "Input and Value workspace need to have the same number of points.");
  }

  if (outIWS != inIWS) {
    // Not in-place. So clone the input to the output
    IAlgorithm_sptr clone =
        this->createChildAlgorithm("CloneMDWorkspace", 0.0, 0.5, true);
    clone->setProperty("InputWorkspace",
                       boost::dynamic_pointer_cast<IMDWorkspace>(inIWS));
    clone->executeAsChildAlg();
    IMDWorkspace_sptr temp = clone->getProperty("OutputWorkspace");
    outIWS = boost::dynamic_pointer_cast<IMDHistoWorkspace>(temp);
  }

  MDHistoWorkspace_sptr outWS =
      boost::dynamic_pointer_cast<MDHistoWorkspace>(outIWS);
  MDHistoWorkspace_sptr maskWS =
      boost::dynamic_pointer_cast<MDHistoWorkspace>(maskIWS);
  MDHistoWorkspace_sptr valueWS =
      boost::dynamic_pointer_cast<MDHistoWorkspace>(valueIWS);

  if (!outWS || !maskWS)
    throw std::runtime_error("Error creating output workspace.");

  // Set either using the WS or the single value
  if (useValueWS)
    outWS->setUsingMask(*maskWS, *valueWS);
  else
    outWS->setUsingMask(*maskWS, value, 0.0 /* assume zero error */);

  // Save the output
  setProperty("OutputWorkspace", outIWS);
}

} // namespace Mantid
} // namespace MDAlgorithms
