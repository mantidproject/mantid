// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_WORKFLOWALGORITHMS_MUONGROUPCOUNTSCALCULATOR_H_
#define MANTID_WORKFLOWALGORITHMS_MUONGROUPCOUNTSCALCULATOR_H_

#include "MantidWorkflowAlgorithms/MuonGroupCalculator.h"

namespace Mantid {
namespace WorkflowAlgorithms {

/** MuonGroupCountsCalculator : Calculates pure counts of the group specified
  via group index
*/
class DLLExport MuonGroupCountsCalculator : public MuonGroupCalculator {
public:
  MuonGroupCountsCalculator(const Mantid::API::WorkspaceGroup_sptr inputWS,
                            const std::vector<int> summedPeriods,
                            const std::vector<int> subtractedPeriods,
                            const int groupIndex);
  /// Performs group counts calculation
  Mantid::API::MatrixWorkspace_sptr calculate() const override;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /* MANTID_WORKFLOWALGORITHMS_MUONGROUPCOUNTSCALCULATOR_H_ */