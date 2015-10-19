#include "MantidAlgorithms/AddHistoryNote.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid {
namespace Algorithms {

using Mantid::Kernel::Direction;
using Mantid::Kernel::MandatoryValidator;
using Mantid::API::WorkspaceProperty;
using Mantid::API::Workspace;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AddHistoryNote)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
AddHistoryNote::AddHistoryNote() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
AddHistoryNote::~AddHistoryNote() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string AddHistoryNote::name() const { return "AddHistoryNote"; }

/// Algorithm's version for identification. @see Algorithm::version
int AddHistoryNote::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string AddHistoryNote::category() const {
  return "DataHandling";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string AddHistoryNote::summary() const {
  return "Adds a note into the history record of a workspace";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void AddHistoryNote::init() {
  declareProperty(
      new WorkspaceProperty<Workspace>("Workspace", "", Direction::InOut),
      "An InOut workspace that will store the new history record");

  declareProperty(
      "Note", "", boost::make_shared<MandatoryValidator<std::string>>(),
      "The note you want to store in the history of the workspace", Direction::Input);

  //always record history for this algorithm
  enableHistoryRecordingForChild(true);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void AddHistoryNote::exec() {
  //do nothing
}

} // namespace Algorithms
} // namespace Mantid
