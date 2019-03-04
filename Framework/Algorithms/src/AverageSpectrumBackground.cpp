// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/AverageSpectrumBackground.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace Algorithms {

/// Algorithm's name for identification. @see Algorithm::name
const std::string AverageSpectrumBackground::name() const {
  return "AverageSpectrumBackground";
}

/// Algorithm's version for identification. @see Algorithm::version
int AverageSpectrumBackground::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string AverageSpectrumBackground::category() const {
  return "Reflectometry;Reflectometry\\ISIS";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string AverageSpectrumBackground::summary() const { return ""; }

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AverageSpectrumBackground)

/** Sums spectra bin by bin in the given range using the child algorithm
 * GroupDetectors. Note that KeepUngroupedSpectra is set to false.
 * @param inputWS:: The Workspace2D to take as input
 * @param indexList:: An array of workspace indices to combine
 * @return :: A workspace containing a single spectra and DectectorGroups
 */
API::MatrixWorkspace_sptr AverageSpectrumBackground::groupBackgroundDetectors(
    API::MatrixWorkspace_sptr inputWS, const std::vector<size_t> indexList) {

  auto alg = this->createChildAlgorithm("GroupDetectors");
  alg->setProperty("InputWorkspace", inputWS);
  alg->setProperty("WorkspaceIndexList", indexList);
  alg->setProperty("KeepUngroupedSpectra", false);
  alg->execute();
  return alg->getProperty("OutputWorkspace");
}

std::vector<size_t> AverageSpectrumBackground::getSpectraFromRange(
    const std::vector<size_t> range) {
  const auto start = range[0];
  const auto end = range[1];
  std::vector<size_t> spectra;
  for (auto itr = start; itr < end; ++itr) {
    spectra.push_back(itr);
  }
  return spectra;
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void AverageSpectrumBackground::init() {

  // Input workspace
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          "InputWorkspace", "", Kernel::Direction::Input,
          boost::make_shared<API::CommonBinsValidator>()),
      "An input workspace.");

  // bottom background range
  declareProperty(Kernel::make_unique<Kernel::ArrayProperty<size_t>>(
                      "BottomBackgroundRange", std::vector<size_t>()),
                  "A list of the bottom background ranges.");
  // top background range
  declareProperty(Kernel::make_unique<Kernel::ArrayProperty<size_t>>(
                      "TopBackgroundRange", std::vector<size_t>()),
                  "A list of the top background ranges.");

  // Output workspace
  declareProperty(Kernel::make_unique<API::WorkspaceProperty<>>(
                      "OutputWorkspace", "", Kernel::Direction::Output),
                  "A Workspace with the background removed.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void AverageSpectrumBackground::exec() {
  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  std::vector<size_t> bottomBgdRange = getProperty("BottomBackgroundRange");
  std::vector<size_t> topBgdRange = getProperty("TopBackgroundRange");

  if (bottomBgdRange.empty() && topBgdRange.empty())
    throw std::invalid_argument("At least one background range is required");
  if (bottomBgdRange.size() != 2 && !bottomBgdRange.empty())
    throw std::invalid_argument("BottomBackgroundRange must have length 2");
  if (topBgdRange.size() != 2 && !topBgdRange.empty())
    throw std::invalid_argument("TopBackgroundRange must have length 2");
  if ((!bottomBgdRange.empty() && bottomBgdRange[1] - bottomBgdRange[0] == 0) ||
      (!topBgdRange.empty() && topBgdRange[1] - topBgdRange[0] == 0)) {
    throw std::invalid_argument("Cannot have a range of length 0");
  }

  API::MatrixWorkspace_sptr bottomBgd, topBgd;
  // groups the bottom background into a single spectra workspace
  if (!bottomBgdRange.empty()) {
    bottomBgd =
        groupBackgroundDetectors(inputWS, getSpectraFromRange(bottomBgdRange));
  }

  // groups the top background into a single spectra workspace
  if (!topBgdRange.empty()) {
    topBgd =
        groupBackgroundDetectors(inputWS, getSpectraFromRange(topBgdRange));
  }
  
  size_t totalBkgRange;
  API::MatrixWorkspace_sptr bgd;
  if (topBgdRange.empty()) {
    bgd = bottomBgd;
    totalBkgRange = bottomBgdRange[1] - bottomBgdRange[0];
  } else if (bottomBgdRange.empty()) {
    bgd = topBgd;
    totalBkgRange = topBgdRange[1] - topBgdRange[0];
  } else {
    bgd = plus(bottomBgd, topBgd);
    totalBkgRange = (bottomBgdRange[1] - bottomBgdRange[0]) +
                    (topBgdRange[1] - topBgdRange[0]);
  }

  // find the average of the background
  API::MatrixWorkspace_sptr averageBgd =
      divide(bgd, static_cast<double>(totalBkgRange));

  auto subtract = createChildAlgorithm("Minus");
  subtract->setProperty("LHSWorkspace", inputWS);
  subtract->setProperty("RHSWorkspace", averageBgd);
  subtract->setProperty("AllowDifferentNumberSpectra", true);
  subtract->execute();
  API::MatrixWorkspace_sptr outputWS = subtract->getProperty("OutputWorkspace");

  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid
