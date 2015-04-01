#include "MantidCrystal/SortPeaksWorkspace.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include <boost/make_shared.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SortPeaksWorkspace)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SortPeaksWorkspace::SortPeaksWorkspace() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SortPeaksWorkspace::~SortPeaksWorkspace() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string SortPeaksWorkspace::name() const {
  return "SortPeaksWorkspace";
}

/// Algorithm's version for identification. @see Algorithm::version
int SortPeaksWorkspace::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SortPeaksWorkspace::category() const { return "Crystal"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SortPeaksWorkspace::init() {
  declareProperty(new WorkspaceProperty<IPeaksWorkspace>("InputWorkspace", "",
                                                         Direction::Input),
                  "An input workspace.");
  declareProperty(new WorkspaceProperty<IPeaksWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "An output workspace.");

  auto mustHave = boost::make_shared<MandatoryValidator<std::string>>();
  declareProperty("ColumnNameToSortBy", "", mustHave, "Column to sort by");

  declareProperty(
      "SortAscending", true,
      "Sort the OutputWorkspace by the target column in a Ascending fashion.");
}

PeaksWorkspace_sptr SortPeaksWorkspace::tryFetchOutputWorkspace() const {
  IPeaksWorkspace_sptr temp = getProperty("OutputWorkspace");
  PeaksWorkspace_sptr outputWS;
  if (temp != NULL) {
    outputWS = boost::dynamic_pointer_cast<PeaksWorkspace>(temp);
    if (outputWS == NULL) {
      throw std::invalid_argument("OutputWorkspace is not a PeaksWorkspace.");
    }
  }
  return outputWS;
}

PeaksWorkspace_sptr SortPeaksWorkspace::tryFetchInputWorkspace() const {
  IPeaksWorkspace_sptr temp = getProperty("InputWorkspace");
  PeaksWorkspace_sptr inputWS =
      boost::dynamic_pointer_cast<PeaksWorkspace>(temp);
  if (inputWS == NULL) {
    throw std::invalid_argument("InputWorkspace is not a PeaksWorkspace.");
  }
  return inputWS;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SortPeaksWorkspace::exec() {
  const std::string columnToSortBy = getProperty("ColumnNameToSortBy");
  const bool sortAscending = getProperty("SortAscending");
  PeaksWorkspace_sptr inputWS = tryFetchInputWorkspace();
  PeaksWorkspace_sptr outputWS = tryFetchOutputWorkspace();

  try {
    // Try to get the column. This will throw if the column does not exist.
    inputWS->getColumn(columnToSortBy);

    if (inputWS != outputWS) {
      outputWS = boost::shared_ptr<PeaksWorkspace>(inputWS->clone());
    }

    // Perform the sorting.
    std::vector<std::pair<std::string, bool>> sortCriteria;
    sortCriteria.push_back(
        std::pair<std::string, bool>(columnToSortBy, sortAscending));
    outputWS->sort(sortCriteria);
    setProperty("OutputWorkspace", outputWS);
  } catch (std::invalid_argument &) {
    this->g_log.error("Specified ColumnToSortBy does not exist");
    throw;
  }
}

} // namespace Crystal
} // namespace Mantid
