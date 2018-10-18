// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CalculateDynamicRange.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"

namespace {
/**
 * @param lambda : wavelength in Angstroms
 * @param twoTheta : twoTheta in degreess
 * @return Q : momentum transfer [Aˆ-1]
 */
double calculateQ(const double lambda, const double twoTheta) {
  return (4 * M_PI * std::sin(twoTheta * (M_PI / 180) / 2)) / (lambda);
}
} // namespace

namespace Mantid {
namespace Algorithms {

using Mantid::API::MatrixWorkspace;
using Mantid::API::Run;
using Mantid::API::SpectrumInfo;
using Mantid::API::WorkspaceProperty;
using Mantid::API::WorkspaceUnitValidator;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculateDynamicRange)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CalculateDynamicRange::name() const {
  return "CalculateDynamicRange";
}

/// Algorithm's version for identification. @see Algorithm::version
int CalculateDynamicRange::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CalculateDynamicRange::category() const {
  return "Utility\\Workspaces";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CalculateDynamicRange::summary() const {
  return "Calculates and sets Qmin and Qmax of a SANS workspace";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CalculateDynamicRange::init() {
  auto unitValidator = boost::make_shared<WorkspaceUnitValidator>("Wavelength");
  declareProperty(Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "Workspace", "", Direction::InOut, unitValidator),
                  "An input workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalculateDynamicRange::exec() {
  API::MatrixWorkspace_sptr workspace = getProperty("Workspace");
  double min = std::numeric_limits<double>::max(),
         max = std::numeric_limits<double>::min();
  const int64_t nHist = static_cast<int64_t>(workspace->getNumberHistograms());
  const auto &spectrumInfo = workspace->spectrumInfo();
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t i = 0; i < nHist; ++i) {
    const size_t index = static_cast<size_t>(i);
    if (!spectrumInfo.isMonitor(index) && !spectrumInfo.isMasked(index)) {
      const auto &lambdaBinning = workspace->x(index);
      const Kernel::V3D detPos = spectrumInfo.position(index);
      double r, theta, phi;
      detPos.getSpherical(r, theta, phi);
      const double v1 = calculateQ(lambdaBinning.front(), theta);
      const double v2 = calculateQ(lambdaBinning.back(), theta);
      PARALLEL_CRITICAL(CalculateDynamicRange) {
        min = std::min(min, std::min(v1, v2));
        max = std::max(max, std::max(v1, v2));
      }
    }
  }
  g_log.information("Calculated QMin = " + std::to_string(min));
  g_log.information("Calculated QMax = " + std::to_string(max));
  auto &run = workspace->mutableRun();
  run.addProperty<double>("qmin", min, true);
  run.addProperty<double>("qmax", max, true);
}

} // namespace Algorithms
} // namespace Mantid
