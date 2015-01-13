//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/DeleteTableRows.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidKernel/ArrayProperty.h"

#include <set>
#include <functional>

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(DeleteTableRows);

using namespace Kernel;
using namespace API;

/// Initialisation method.
void DeleteTableRows::init() {
  declareProperty(new WorkspaceProperty<API::ITableWorkspace>(
                      "TableWorkspace", "", Direction::InOut),
                  "The name of the workspace that will be modified.");
  declareProperty(
      new ArrayProperty<size_t>("Rows"),
      "A comma-separated list of row numbers. Row numbering starts with 0.");
}

/**
*   Executes the algorithm.
*/
void DeleteTableRows::exec() {
  API::ITableWorkspace_sptr tw = getProperty("TableWorkspace");
  API::IPeaksWorkspace_sptr pw =
      boost::dynamic_pointer_cast<API::IPeaksWorkspace>(tw);
  std::vector<size_t> rows = getProperty("Rows");
  // sort the row indices in reverse order
  std::set<size_t, std::greater<size_t>> sortedRows(rows.begin(), rows.end());
  std::set<size_t, std::greater<size_t>>::iterator it = sortedRows.begin();
  for (; it != sortedRows.end(); ++it) {
    if (*it >= tw->rowCount())
      continue;
    if (pw) {
      pw->removePeak(static_cast<int>(*it));
    } else {
      tw->removeRow(*it);
    }
  }
  setProperty("TableWorkspace", tw);
}

} // namespace DataHandling
} // namespace Mantid
