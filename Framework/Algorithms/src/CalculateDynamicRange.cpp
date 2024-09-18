// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CalculateDynamicRange.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/Instrument.h"
#include "MantidHistogramData/HistogramIterator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Unit.h"

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

namespace Mantid::Algorithms {

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculateDynamicRange)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CalculateDynamicRange::name() const { return "CalculateDynamicRange"; }

/// Algorithm's version for identification. @see Algorithm::version
int CalculateDynamicRange::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CalculateDynamicRange::category() const { return "Utility\\Workspaces"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CalculateDynamicRange::summary() const {
  return "Calculates and sets Qmin and Qmax of a SANS workspace";
}

/// Validate inputs
std::map<std::string, std::string> CalculateDynamicRange::validateInputs() {
  std::map<std::string, std::string> issues;
  API::MatrixWorkspace_sptr workspace = getProperty("Workspace");
  if (!workspace) {
    issues["InputWorkspace"] = "The InputWorkspace must be a MatrixWorkspace.";
  } else if (workspace->getAxis(0)->unit()->unitID() != "Wavelength" && !workspace->run().hasProperty("wavelength")) {
    issues["InputWorkspace"] = "If the workspace is not in units of wavelength it must have a sample log wavelength.";
  }
  return issues;
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CalculateDynamicRange::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("Workspace", "", Direction::InOut),
                  "An input workspace.");

  declareProperty(std::make_unique<Mantid::Kernel::ArrayProperty<std::string>>("ComponentNames"),
                  "List of component names to calculate the q ranges for.");
}

/**
 * Calculates the max and min Q for given list of workspace indices
 * @param workspace : the input workspace
 * @param indices : the list of workspace indices
 * @param compName : the name of the detector component
 */
void CalculateDynamicRange::calculateQMinMax(const MatrixWorkspace_sptr &workspace, const std::vector<size_t> &indices,
                                             const std::string &compName = "") {
  const auto &spectrumInfo = workspace->spectrumInfo();
  const auto unit = workspace->getAxis(0)->unit()->unitID();
  double constantLambda = 0;
  if (workspace->run().hasProperty("wavelength"))
    constantLambda = workspace->run().getLogAsSingleValue("wavelength");
  double min = std::numeric_limits<double>::max(), max = std::numeric_limits<double>::lowest();
  // PARALLEL_FOR_NO_WSP_CHECK does not work with range-based for so NOLINT this
  // block
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t index = 0; index < static_cast<int64_t>(indices.size()); ++index) { // NOLINT (modernize-for-loop)
    if (spectrumInfo.hasDetectors(indices[index]) && !spectrumInfo.isMonitor(indices[index]) &&
        !spectrumInfo.isMasked(indices[index])) {
      const auto &spectrum = workspace->histogram(indices[index]);
      const Kernel::V3D detPos = spectrumInfo.position(indices[index]);
      double r, theta, phi;
      detPos.getSpherical(r, theta, phi);
      if (unit != "Wavelength") {
        const double q = calculateQ(constantLambda, theta);
        PARALLEL_CRITICAL(CalculateDynamicRange) {
          min = std::min(min, q);
          max = std::max(max, q);
        }
      } else {
        // Use the bin centers
        const double q1 = calculateQ(spectrum.begin()->center(), theta);
        const double q2 = calculateQ(std::prev(spectrum.end())->center(), theta);
        PARALLEL_CRITICAL(CalculateDynamicRange) {
          min = std::min(min, std::min(q1, q2));
          max = std::max(max, std::max(q1, q2));
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
  run.addProperty<double>(qminLogName, min, true);
  run.addProperty<double>(qmaxLogName, max, true);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalculateDynamicRange::exec() {
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
      g_log.error() << "No instrument in input workspace. Ignoring ComponentList\n";
      return;
    }
    for (const auto &compName : componentNames) {
      std::vector<IDetector_const_sptr> dets;
      instrument->getDetectorsInBank(dets, compName);
      if (dets.empty()) {
        const auto component = instrument->getComponentByName(compName);
        const auto det = std::dynamic_pointer_cast<const IDetector>(component);
        if (!det) {
          g_log.error() << "No detectors found in component '" << compName << "'\n";
          continue;
        }
        dets.emplace_back(det);
      }
      if (!dets.empty()) {
        std::vector<detid_t> detIDs;
        detIDs.reserve(dets.size());
        std::transform(dets.begin(), dets.end(), std::back_inserter(detIDs),
                       [](const auto &det) { return det->getID(); });

        const auto indices = workspace->getIndicesFromDetectorIDs(detIDs);
        calculateQMinMax(workspace, indices, compName);
      }
    }
  }
}

} // namespace Mantid::Algorithms
