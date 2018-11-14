// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACE_H_
#define MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACE_H_

#include "MantidAlgorithms/ReflectometryWorkflowBase.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** CreateTransmissionWorkspace : Create a transmission run workspace in
 Wavelength given one or more TOF workspaces
 */
class DLLExport CreateTransmissionWorkspace : public ReflectometryWorkflowBase {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Creates a transmission run workspace in Wavelength from input TOF "
           "workspaces.";
  }

  int version() const override;
  const std::string category() const override;

private:
  /// Make a transmission correction workspace
  API::MatrixWorkspace_sptr makeTransmissionCorrection(
      const std::string &processingCommands, const MinMax &wavelengthInterval,
      const OptionalMinMax &wavelengthMonitorBackgroundInterval,
      const OptionalMinMax &wavelengthMonitorIntegrationInterval,
      const OptionalInteger &i0MonitorIndex,
      API::MatrixWorkspace_sptr firstTransmissionRun,
      OptionalMatrixWorkspace_sptr secondTransmissionRun,
      const OptionalDouble &stitchingStart,
      const OptionalDouble &stitchingDelta, const OptionalDouble &stitchingEnd,
      const OptionalDouble &stitchingStartOverlap,
      const OptionalDouble &stitchingEndOverlap);

  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACE_H_ */
