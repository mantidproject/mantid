#ifndef MANTID_SINQ_POLDIANALYSERESIDUALS_H_
#define MANTID_SINQ_POLDIANALYSERESIDUALS_H_

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

    Copyright © 2014 PSI-MSS

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
class MANTID_SINQ_DLL PoldiAnalyseResiduals : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

protected:
  double sumCounts(const DataObjects::Workspace2D_sptr &workspace,
                   const std::vector<int> &workspaceIndices) const;
  size_t numberOfPoints(const DataObjects::Workspace2D_sptr &workspace,
                        const std::vector<int> &workspaceIndices) const;
  void addValue(DataObjects::Workspace2D_sptr &workspace, double value,
                const std::vector<int> &workspaceIndices) const;

  DataObjects::Workspace2D_sptr
  calculateResidualWorkspace(const DataObjects::Workspace2D_sptr &measured,
                             const DataObjects::Workspace2D_sptr &calculated);
  void normalizeResiduals(DataObjects::Workspace2D_sptr &residuals,
                          const std::vector<int> &validWorkspaceIndices);
  double relativeCountChange(const DataObjects::Workspace2D_sptr &sum,
                             double totalMeasuredCounts);

  DataObjects::Workspace2D_sptr
  addWorkspaces(const DataObjects::Workspace2D_sptr &lhs,
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

#endif /* MANTID_SINQ_POLDIANALYSERESIDUALS_H_ */
