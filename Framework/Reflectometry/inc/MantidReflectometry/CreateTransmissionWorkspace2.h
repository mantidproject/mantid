// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidReflectometry/DllConfig.h"
#include "MantidReflectometry/ReflectometryWorkflowBase2.h"

namespace Mantid {
namespace Reflectometry {

/** CreateTransmissionWorkspace2 : Create a transmission run workspace in
 Wavelength given one or more TOF workspaces. Version 2 of the algorithm.
 */
class MANTID_REFLECTOMETRY_DLL CreateTransmissionWorkspace2
    : public ReflectometryWorkflowBase2 {
public:
  const std::string name() const override;
  const std::string summary() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CreateTransmissionWorkspaceAuto"};
  }
  const std::string category() const override;

private:
  /// Initialize
  void init() override;
  /// Execute
  void exec() override;
  /// Validate inputs
  std::map<std::string, std::string> validateInputs() override;

  /// Normalize by monitors
  API::MatrixWorkspace_sptr
  normalizeDetectorsByMonitors(const API::MatrixWorkspace_sptr &IvsTOF);
  /// Get the run numbers of the input workspaces
  void getRunNumbers();
  /// Get the run number of a given workspace
  std::string getRunNumber(std::string const &propertyName);
  /// Store a transition run in ADS
  void setOutputTransmissionRun(int which, const API::MatrixWorkspace_sptr &ws);
  /// Store the stitched transition workspace run in ADS
  void setOutputWorkspace(const API::MatrixWorkspace_sptr &ws);

  /// Run numbers for the first/second transmission run
  std::string m_firstTransmissionRunNumber;
  std::string m_secondTransmissionRunNumber;
  /// Flag to indicate that one or both transmission workspace
  /// does not have a run number set
  bool m_missingRunNumber{false};
};

} // namespace Reflectometry
} // namespace Mantid
