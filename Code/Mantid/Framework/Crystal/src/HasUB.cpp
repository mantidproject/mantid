#include "MantidCrystal/HasUB.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(HasUB)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
HasUB::HasUB() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
HasUB::~HasUB() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string HasUB::name() const { return "HasUB"; };

/// Algorithm's version for identification. @see Algorithm::version
int HasUB::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string HasUB::category() const { return "Crystal"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void HasUB::init() {
  declareProperty(
      new WorkspaceProperty<Workspace>("Workspace", "", Direction::Input),
      "Workspace to clear the UB from.");
  declareProperty(
      new PropertyWithValue<bool>("HasUB", "", Direction::Output),
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

} // namespace Crystal
} // namespace Mantid
