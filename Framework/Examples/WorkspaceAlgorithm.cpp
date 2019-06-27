// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "WorkspaceAlgorithm.h"

#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace Algorithms {

// Algorithm must be declared
DECLARE_ALGORITHM(WorkspaceAlgorithm)

using namespace Kernel;
using namespace API;

/**  Initialization code
 *
 *   Properties have to be declared here before they can be used
 */
void WorkspaceAlgorithm::init() {

  // Declare a 1D workspace property.
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("Workspace", "", Direction::Input));
}

/** Executes the algorithm
 */
void WorkspaceAlgorithm::exec() {
  // g_log is a reference to the logger. It is used to print out information,
  // warning, and error messages
  g_log.information() << "Running algorithm " << name() << " version "
                      << version() << std::endl;

  // Get the input workspace
  MatrixWorkspace_const_sptr workspace = getProperty("Workspace");

  // Number of single indexable items in the workspace
  g_log.information() << "Number of items = " << workspace->size() << std::endl;

  int count = 0;
  size_t histogramCount = workspace->getNumberHistograms();
  for (size_t i = 0; i < histogramCount; ++i) {
    auto &XValues = workspace->x(i);
    auto &YValues = workspace->y(i);
    auto &EValues = workspace->e(i);

    const auto numBins = YValues.size();
    for (size_t j = 0; numBins; ++j) {
      g_log.information() << "Point number " << count++
                          << " values: " << XValues[j] << ' ' << YValues[j]
                          << ' ' << EValues[j] << std::endl;
    }
  }
}
} // namespace Algorithms
} // namespace Mantid
