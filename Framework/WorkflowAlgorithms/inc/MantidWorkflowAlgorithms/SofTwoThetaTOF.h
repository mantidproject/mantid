// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_WORKFLOWALGORITHMS_SOFTWOTHETATOF_H_
#define MANTID_WORKFLOWALGORITHMS_SOFTWOTHETATOF_H_

#include "MantidAPI/DataProcessorAlgorithm.h"

namespace Mantid {
namespace WorkflowAlgorithms {

/** SofTwoThetaTOF : Convert a S(spectrum number, TOF) workspace to
 * S(twoTheta, TOF) workspace.
 */
class DLLExport SofTwoThetaTOF : public API::DataProcessorAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  double clarifyAngleStep(API::MatrixWorkspace const &ws);
  API::MatrixWorkspace_sptr convertToConstantL2(API::MatrixWorkspace_sptr &ws);
  API::MatrixWorkspace_sptr convertToTwoTheta(API::MatrixWorkspace_sptr &ws);
  API::MatrixWorkspace_sptr groupByTwoTheta(API::MatrixWorkspace_sptr &ws,
                                            double const twoThetaStep);
  API::MatrixWorkspace_sptr
  maskEmptyBins(API::MatrixWorkspace_sptr &maskable,
                API::MatrixWorkspace_sptr &comparison);
  API::MatrixWorkspace_sptr rebinToNonRagged(API::MatrixWorkspace_sptr &ws);
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /* MANTID_WORKFLOWALGORITHMS_SOFTWOTHETATOF_H_ */
