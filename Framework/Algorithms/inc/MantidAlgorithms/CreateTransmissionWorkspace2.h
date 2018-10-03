// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACE2_H_
#define MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACE2_H_

#include "MantidAlgorithms/ReflectometryWorkflowBase2.h"

namespace Mantid {
namespace Algorithms {

/** CreateTransmissionWorkspace2 : Create a transmission run workspace in
 Wavelength given one or more TOF workspaces. Version 2 of the algorithm.
 */
class DLLExport CreateTransmissionWorkspace2
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
  normalizeDetectorsByMonitors(API::MatrixWorkspace_sptr IvsTOF);
  /// Get the run numbers of the input workspaces
  void getRunNumbers();
  /// Store a transition run in ADS
  void storeTransitionRun(int which, API::MatrixWorkspace_sptr ws);
  /// Store the stitched transition workspace run in ADS
  void storeOutputWorkspace(API::MatrixWorkspace_sptr ws);

  std::string m_firstTransmissionRunNumber;
  std::string m_secondTransmissionRunNumber;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACE2_H_ */
