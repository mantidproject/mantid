// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidSINQ/DllConfig.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidSINQ/PoldiUtilities/PoldiInstrumentAdapter.h"

namespace Mantid {
namespace Poldi {

/** PoldiAnalyseResiduals

    An algorithm that performs residual analysis for
    POLDI data. It uses a modified version of the correlation
    method implemented in PoldiAutoCorrelation.

        @author Michael Wedel, Paul Scherrer Institut - SINQ
        @date 21/11/2014
  */
class MANTID_SINQ_DLL PoldiAnalyseResiduals : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

protected:
  double sumCounts(const DataObjects::Workspace2D_sptr &workspace, const std::vector<int> &workspaceIndices) const;
  size_t numberOfPoints(const DataObjects::Workspace2D_sptr &workspace, const std::vector<int> &workspaceIndices) const;
  void addValue(DataObjects::Workspace2D_sptr &workspace, double value, const std::vector<int> &workspaceIndices) const;

  DataObjects::Workspace2D_sptr calculateResidualWorkspace(const DataObjects::Workspace2D_sptr &measured,
                                                           const DataObjects::Workspace2D_sptr &calculated);
  void normalizeResiduals(DataObjects::Workspace2D_sptr &residuals, const std::vector<int> &validWorkspaceIndices);
  double relativeCountChange(const DataObjects::Workspace2D_sptr &sum, double totalMeasuredCounts);

  DataObjects::Workspace2D_sptr addWorkspaces(const DataObjects::Workspace2D_sptr &lhs,
                                              const DataObjects::Workspace2D_sptr &rhs);
  void logIteration(int iteration, double relativeChange);

  bool nextIterationAllowed(int iterations, double relativeChange);
  bool relativeChangeIsLargerThanLimit(double relativeChange);
  bool iterationLimitReached(int iterations);

private:
  void init() override;
  void exec() override;
};

} // namespace Poldi
} // namespace Mantid
