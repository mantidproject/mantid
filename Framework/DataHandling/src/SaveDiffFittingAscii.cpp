

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveDiffFittingAscii.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace Mantid::API;

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveDiffFittingAscii)

using namespace Kernel;
using namespace API;

/// Empty constructor
SaveDiffFittingAscii::SaveDiffFittingAscii() : Mantid::API::Algorithm() {}

/// Initialisation method.
void SaveDiffFittingAscii::init() {

}

/**
*   Executes the algorithm.
*/
void SaveDiffFittingAscii::exec() {
  // Get the workspace
}

} // namespace DataHandling
} // namespace Mantid
