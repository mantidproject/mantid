// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ReflectometryWorkflowBase3.h"
#include "MantidKernel/EnabledWhenProperty.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace Mantid {
namespace Algorithms {

/** Initialize properties related to transmission normalization
 */
void ReflectometryWorkflowBase3::initTransmissionProperties() {
  // Include everything from the base class
  ReflectometryWorkflowBase2::initTransmissionProperties();

  // Add additional output workspace properties
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspaceTransmission", "", Direction::Output,
                      PropertyMode::Optional),
                  "Output transmissison workspace in wavelength");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspaceFirstTransmission", "", Direction::Output,
                      PropertyMode::Optional),
                  "First transmissison workspace in wavelength");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspaceSecondTransmission", "",
                      Direction::Output, PropertyMode::Optional),
                  "Second transmissison workspace in wavelength");

  // Specify conditional output properties for when debug is on
  setPropertySettings(
      "OutputWorkspaceFirstTransmission",
      std::make_unique<Kernel::EnabledWhenProperty>("Debug", IS_EQUAL_TO, "1"));
  setPropertySettings(
      "OutputWorkspaceSecondTransmission",
      std::make_unique<Kernel::EnabledWhenProperty>("Debug", IS_EQUAL_TO, "1"));

  // Put them in the Transmission group
  setPropertyGroup("OutputWorkspaceTransmission", "Transmission");
  setPropertyGroup("OutputWorkspaceFirstTransmission", "Transmission");
  setPropertyGroup("OutputWorkspaceSecondTransmission", "Transmission");
}
} // namespace Algorithms
} // namespace Mantid
