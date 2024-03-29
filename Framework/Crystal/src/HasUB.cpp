// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/HasUB.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid::Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(HasUB)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string HasUB::name() const { return "HasUB"; }

/// Algorithm's version for identification. @see Algorithm::version
int HasUB::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string HasUB::category() const { return "Crystal\\UBMatrix"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void HasUB::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("Workspace", "", Direction::Input),
                  "Workspace to clear the UB from.");
  declareProperty(std::make_unique<PropertyWithValue<bool>>("HasUB", false, Direction::Output),
                  "Indicates action performed, or predicted to perform if DryRun.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void HasUB::exec() {
  Workspace_sptr ws = this->getProperty("Workspace");
  bool hasUB = ClearUB::doExecute(ws.get(), true /*DryRun*/);
  this->setProperty("HasUB", hasUB);
}

} // namespace Mantid::Crystal
