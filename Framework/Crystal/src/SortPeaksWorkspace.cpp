// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/SortPeaksWorkspace.h"
#include "MantidKernel/MandatoryValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid::Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SortPeaksWorkspace)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string SortPeaksWorkspace::name() const { return "SortPeaksWorkspace"; }

/// Algorithm's version for identification. @see Algorithm::version
int SortPeaksWorkspace::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SortPeaksWorkspace::category() const { return "Crystal\\Peaks;Utility\\Sorting"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SortPeaksWorkspace::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");

  auto mustHave = std::make_shared<MandatoryValidator<std::string>>();
  declareProperty("ColumnNameToSortBy", "", mustHave, "Column to sort by");

  declareProperty("SortAscending", true, "Sort the OutputWorkspace by the target column in a Ascending fashion.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SortPeaksWorkspace::exec() {
  const std::string columnToSortBy = getProperty("ColumnNameToSortBy");
  const bool sortAscending = getProperty("SortAscending");
  IPeaksWorkspace_sptr inputWS = getProperty("InputWorkspace");
  IPeaksWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  // Try to get the column. This will throw if the column does not exist.
  inputWS->getColumn(columnToSortBy);

  if (inputWS != outputWS) {
    outputWS = inputWS->clone();
  }

  std::vector<PeaksWorkspace::ColumnAndDirection> sortCriteria;
  sortCriteria.emplace_back(columnToSortBy, sortAscending);
  outputWS->sort(sortCriteria);
  setProperty("OutputWorkspace", outputWS);
}

} // namespace Mantid::Crystal
