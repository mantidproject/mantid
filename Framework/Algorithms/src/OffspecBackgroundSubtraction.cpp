// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/OffspecBackgroundSubtraction.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid {
namespace Algorithms {

/// Algorithm's name for identification. @see Algorithm::name
const std::string OffspecBackgroundSubtraction::name() const {
  return "OffspecBackgroundSubtraction";
}

/// Algorithm's version for identification. @see Algorithm::version
int OffspecBackgroundSubtraction::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string OffspecBackgroundSubtraction::category() const {
  return "Reflectometry;Reflectometry\\ISIS";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string OffspecBackgroundSubtraction::summary() const {
  return "";
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(OffspecBackgroundSubtraction)

/** Sums spectra bin by bin in the given range using the child algorithm GroupDetectors. Note that KeepUngroupedSpectra is set to false.
 * @param inputWS:: The Workspace2D to take as input
 * @param indexList:: An array of workspace indices to combine
 * @return :: A workspace containing a single spectra and DectectorGroups
 */
API::MatrixWorkspace_sptr
OffspecBackgroundSubtraction::groupBackgroundDetectors(
    API::MatrixWorkspace_sptr inputWS, const std::vector<size_t> indexList) {

  auto alg = this->createChildAlgorithm("GroupDetectors");
  alg->setProperty("InputWorkspace", inputWS);
  alg->setProperty("WorkspaceIndexList", indexList);
  alg->setProperty("KeepUngroupedSpectra", false);
  alg->execute();
  return alg->getProperty("OutputWorkspace");
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void OffspecBackgroundSubtraction::init() {

  // Input workspace
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<CommonBinsValidator>()),
                  "An input workspace.");
  //bottom background range
  declareProperty(Kernel::make_unique<Kernel::ArrayProperty<size_t>>(
                      "BottomBackgroundRanges", Direction::Input),
                  "A list of the bottom background ranges.");
  //top background range
  declareProperty(Kernel::make_unique<Kernel::ArrayProperty<size_t>>(
                      "TopBackgroundRanges", Direction::Input),
      "A list of the top background ranges.");

  //Output workspace
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "A Workspace with the background removed.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void OffspecBackgroundSubtraction::exec() { 
  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  std::vector<size_t> bottomBgdRange = getProperty("BottomBackgroundRanges");
  std::vector<size_t> topBgdRange = getProperty("TopBackgroundRanges"); 

  if (bottomBgdRange.empty() && topBgdRange.empty())
    throw std::invalid_argument(
        "At least one background range is required");

  API::MatrixWorkspace_sptr bottomBgd, topBgd;
  // groups the bottom background into a single spectra workspace
  if (!bottomBgdRange.empty()) {
    bottomBgd = groupBackgroundDetectors(inputWS, bottomBgdRange);
  }

  // groups the top background into a single spectra workspace
  if (!topBgdRange.empty()) {
    topBgd = groupBackgroundDetectors(inputWS, topBgdRange);
  }

  API::MatrixWorkspace_sptr bgd;
  if (topBgdRange.empty()) {
    bgd = bottomBgd;
  } else if (bottomBgdRange.empty()) {
    bgd = topBgd;
  } else {
	bgd = plus(bottomBgd, topBgd);
  }

  //sort bottom and top ranges
  sort(bottomBgdRange.begin(), bottomBgdRange.end());
  sort(topBgdRange.begin(), topBgdRange.end());

  // find the average of the background
  const auto totalBkgRange = (bottomBgdRange.end() - bottomBgdRange.begin()) +
                       (topBgdRange.end() - topBgdRange.begin());
  API::MatrixWorkspace_sptr averageBgd = divide(bgd, totalBkgRange);

  // subtract the background from the input
  API::MatrixWorkspace_sptr outputWS = minus(inputWS, averageBgd);

  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid
