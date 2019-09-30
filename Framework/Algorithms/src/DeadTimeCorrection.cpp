// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/DeadTimeCorrection.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/PropertyWithValue.h"

#include <limits>

namespace Mantid {
namespace Algorithms {

using API::MatrixWorkspace_sptr;
using API::Progress;
using API::WorkspaceProperty;
using Kernel::BoundedValidator;
using Kernel::Direction;
using Kernel::PropertyWithValue;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DeadTimeCorrection)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string DeadTimeCorrection::name() const {
  return "DeadTimeCorrection";
}

/// Algorithm's version for identification. @see Algorithm::version
int DeadTimeCorrection::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string DeadTimeCorrection::category() const {
  return "CorrectionFunctions";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string DeadTimeCorrection::summary() const {
  return "Performs a dead time correction based on count rate.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void DeadTimeCorrection::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                        Direction::Input),
                  "An input workspace.");

  declareProperty(std::make_unique<PropertyWithValue<std::string>>(
                      "GroupingPattern", "", Direction::Input),
                  "See the GroupingPattern documentation of GroupDetectors.");

  auto positive = boost::make_shared<BoundedValidator<double>>();
  positive->setLower(0.);
  declareProperty("Tau", 0., positive, "The count rate coefficient.");

  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void DeadTimeCorrection::exec() {
  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWorkspace = getProperty("OutputWorkspace");
  if (inputWorkspace != outputWorkspace) {
    outputWorkspace = inputWorkspace->clone();
  }
  const auto map = outputWorkspace->getDetectorIDToWorkspaceIndexMap();
  const double tau = getProperty("Tau");
  MatrixWorkspace_sptr integrated = inputWorkspace;
  if (outputWorkspace->blocksize() != 1) {
    auto integrator = createChildAlgorithm("Integration");
    integrator->setProperty("InputWorkspace", inputWorkspace);
    integrator->setPropertyValue("OutputWorkspace", "unused");
    integrator->executeAsChildAlg();
    integrated = integrator->getProperty("OutputWorkspace");
    // after integration we end up with one bin
    // however the bin edges might vary, which does not matter, we just need to
    // group the counts, hence we need to do this before we can group the pixels
    for (size_t index = 1; index < integrated->getNumberHistograms(); ++index) {
      integrated->setSharedX(index, integrated->sharedX(0));
    }
  }
  const std::string groupingPattern = getProperty("GroupingPattern");
  MatrixWorkspace_sptr grouped = integrated;
  if (!groupingPattern.empty()) {
    auto grouper = createChildAlgorithm("GroupDetectors");
    grouper->setProperty("InputWorkspace", integrated);
    grouper->setPropertyValue("OutputWorkspace", "unused");
    grouper->setPropertyValue("GroupingPattern", groupingPattern);
    grouper->setPropertyValue("Behaviour", "Sum");
    grouper->setProperty("KeepUngroupedSpectra", true);
    grouper->executeAsChildAlg();
    grouped = grouper->getProperty("OutputWorkspace");
  }
  Progress progress(this, 0.0, 1.0, grouped->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*outputWorkspace))
  for (int index = 0; index < static_cast<int>(grouped->getNumberHistograms());
       ++index) {
    PARALLEL_START_INTERUPT_REGION
    progress.report("Performing the correction for the group at index " +
                    std::to_string(index));
    const double y = grouped->y(index)[0];
    double correction = 1. / (1. - tau * y);
    if (y >= 1. / tau) {
      g_log.warning()
          << "Saturation count rate reached for grouped detector at index "
          << index
          << ". Correction will be infinity. Check your tau or input "
             "workspace, make sure it is normalised by acquisition time.\n";
      correction = std::numeric_limits<double>::infinity();
    }
    const auto detIDs = grouped->getSpectrum(index).getDetectorIDs();
    for (const auto id : detIDs) {
      const size_t originalIndex = map.at(id);
      auto &spectrum = outputWorkspace->mutableY(originalIndex);
      auto &errors = outputWorkspace->mutableE(originalIndex);
      spectrum *= correction;
      errors *= correction;
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Algorithms
} // namespace Mantid
