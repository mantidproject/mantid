#include "MantidAlgorithms/CalculateQMinMax.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"

namespace Mantid {
namespace Algorithms {

using Mantid::API::MatrixWorkspace;
using Mantid::API::Run;
using Mantid::API::SpectrumInfo;
using Mantid::API::WorkspaceProperty;
using Mantid::API::WorkspaceUnitValidator;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculateQMinMax)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CalculateQMinMax::name() const { return "CalculateQMinMax"; }

/// Algorithm's version for identification. @see Algorithm::version
int CalculateQMinMax::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CalculateQMinMax::category() const {
  return "Utility\\Workspaces";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CalculateQMinMax::summary() const {
  return "Calculates and sets Qmin and Qmax of a SANS workspace";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CalculateQMinMax::init() {
  auto unitValidator = boost::make_shared<WorkspaceUnitValidator>("Wavelength");
  declareProperty(Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "Workspace", "", Direction::InOut, unitValidator),
                  "An input workspace.");
}

/**
 * @param lambda : wavelength in Angstroms
 * @param twoTheta : twoTheta in degreess
 * @return Q : momentum transfer [Aˆ-1]
 */
double CalculateQMinMax::calculateQ(const double lambda,
                                    const double twoTheta) const {
  return (4 * M_PI * std::sin(twoTheta * (M_PI / 180) / 2)) / (lambda);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalculateQMinMax::exec() {
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
      const double v1 = calculateQ(*(lambdaBinning.begin()), theta);
      const double v2 = calculateQ(*(lambdaBinning.end() - 1), theta);
      PARALLEL_CRITICAL(CalculateQMinMax) {
        if (v1 < min) {
          min = v1;
        }
        if (v2 < min) {
          min = v2;
        }
        if (v1 > max) {
          max = v1;
        }
        if (v2 > max) {
          max = v2;
        }
      }
    }
  }
  g_log.information("Calculated QMin = " + std::to_string(min));
  g_log.information("Calculated QMax = " + std::to_string(max));
  auto &run = workspace->mutableRun();
  if (run.hasProperty("qmin")) {
    run.removeProperty("qmin");
  }
  if (run.hasProperty("qmax")) {
    run.removeProperty("qmax");
  }
  run.addProperty<double>("qmin", min);
  run.addProperty<double>("qmax", max);
}

} // namespace Algorithms
} // namespace Mantid
