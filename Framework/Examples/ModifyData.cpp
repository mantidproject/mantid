// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ModifyData.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"

namespace Mantid {
namespace Algorithms {

// Algorithm must be declared
DECLARE_ALGORITHM(ModifyData)

using namespace Kernel;
using namespace API;

/**  Initialization code
 *
 *   Properties have to be declared here before they can be used
 */
void ModifyData::init() {

  // Declare a 2D input workspace property.
  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input));

  // Declare a 2D output workspace property.
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output));

  // Switches between two ways of accessing the data in the input workspace
  declareProperty("UseVectors", false);
}

/** Executes the algorithm
 */
void ModifyData::exec() {
  // g_log is a reference to the logger. It is used to print out information,
  // warning, and error messages
  g_log.information() << "Running algorithm " << name() << " version "
                      << version() << std::endl;

  // Get the input workspace
  MatrixWorkspace_const_sptr inputW = getProperty("InputWorkspace");

  // make output Workspace the same type and size as the input one
  MatrixWorkspace_sptr outputW = WorkspaceFactory::Instance().create(inputW);

  g_log.information() << "Option 1. Original values:" << std::endl;
  // Get the count of histograms in the input workspace
  size_t histogramCount = inputW->getNumberHistograms();
  // Loop over spectra
  for (size_t i = 0; i < histogramCount; ++i) {
    // Retrieve the data into a vector
    auto &newX = outputW->mutableX(i);
    auto &newY = outputW->mutableY(i);
    auto &newE = outputW->mutableE(i);
    const auto &XValues = inputW->x(i);
    const auto &YValues = inputW->y(i);
    const auto &EValues = inputW->e(i);

    // Iterate over i-th spectrum and modify the data
    const auto numBins = YValues.size();
    for (size_t j = 0; j < numBins; j++) {
      g_log.information() << "Spectrum " << i << " Point " << j
                          << " values: " << XValues[j] << ' ' << YValues[j]
                          << ' ' << EValues[j] << std::endl;
      newX[j] = XValues[j] + static_cast<double>(i + j);
      newY[j] = YValues[j] * (2. + 0.1 * static_cast<double>(j));
      newE[j] = EValues[j] + 0.1;
    }
  }

  // Assign it to the output workspace property
  setProperty("OutputWorkspace", outputW);

  // Check the new workspace
  g_log.information() << "New values:" << std::endl;
  int count = 0;
  for (size_t i = 0; i < histogramCount; ++i) {
    const auto &XValues = outputW->x(i);
    const auto &YValues = outputW->y(i);
    const auto &EValues = outputW->e(i);

    const auto numBins = YValues.size();
    for (size_t j = 0; j < numBins; ++j) {
      // Get the reference to a data point
      g_log.information() << "Point number " << count++
                          << " values: " << XValues[j] << ' ' << YValues[j]
                          << ' ' << EValues[j] << std::endl;
    }
  }
}
} // namespace Algorithms
} // namespace Mantid
