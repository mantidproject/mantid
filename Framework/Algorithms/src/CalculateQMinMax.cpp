#include "MantidAlgorithms/CalculateQMinMax.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace Algorithms {

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

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

  declareProperty(
      Kernel::make_unique<Mantid::Kernel::ArrayProperty<std::string>>(
          "ComponentNames"),
      "List of component names to calculate the q ranges for.");
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

/**
 * Calculates the max and min Q for given list of workspace indices
 * @param workspace : the input workspace
 * @param indices : the list of workspace indices
 * @param compName : the name of the detector component
 */
void CalculateQMinMax::calculateQMinMax(MatrixWorkspace_sptr workspace,
                                        const std::vector<size_t> &indices,
                                        const std::string &compName = "") {
  const auto &spectrumInfo = workspace->spectrumInfo();
  double min = std::numeric_limits<double>::max(),
         max = std::numeric_limits<double>::min();
  PARALLEL_FOR_NO_WSP_CHECK()
  for (const auto index : indices) {
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
  auto &run = workspace->mutableRun();
  std::string qminLogName = "qmin";
  std::string qmaxLogName = "qmax";
  if (!compName.empty()) {
    qminLogName += "_" + compName;
    qmaxLogName += "_" + compName;
  }
  if (run.hasProperty(qminLogName)) {
    run.removeProperty(qminLogName);
  }
  if (run.hasProperty(qmaxLogName)) {
    run.removeProperty(qmaxLogName);
  }
  run.addProperty<double>(qminLogName, min);
  run.addProperty<double>(qmaxLogName, max);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalculateQMinMax::exec() {
  API::MatrixWorkspace_sptr workspace = getProperty("Workspace");
  const size_t nHist = workspace->getNumberHistograms();
  std::vector<size_t> allIndices(nHist);
  for (size_t i = 0; i < nHist; ++i) {
    allIndices.emplace_back(i);
  }
  calculateQMinMax(workspace, allIndices);
  const std::vector<std::string> componentNames = getProperty("ComponentNames");
  if (!componentNames.empty()) {
    const auto instrument = workspace->getInstrument();
    if (!instrument) {
      g_log.error()
          << "No instrument in input workspace. Ignoring ComponentList\n";
      return;
    }
    for (const auto &compName : componentNames) {
      std::vector<detid_t> detIDs;
      std::vector<IDetector_const_sptr> dets;
      instrument->getDetectorsInBank(dets, compName);
      if (dets.empty()) {
        const auto component = instrument->getComponentByName(compName);
        const auto det =
            boost::dynamic_pointer_cast<const IDetector>(component);
        if (!det) {
          g_log.error() << "No detectors found in component '" << compName
                        << "'\n";
          continue;
        }
        dets.emplace_back(det);
      }
      if (!dets.empty()) {
        detIDs.reserve(dets.size());
        for (const auto &det : dets) {
          detIDs.emplace_back(det->getID());
        }
        const auto indices = workspace->getIndicesFromDetectorIDs(detIDs);
        calculateQMinMax(workspace, indices, compName);
      }
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
