/**
Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#ifndef MANTID_CUSTOMINTERFACES_RUN_H_
#define MANTID_CUSTOMINTERFACES_RUN_H_
#include "../DllConfig.h"
#include "RangeInQ.h"
#include "ReductionWorkspaces.h"
#include "SlicedReductionWorkspaces.h"
#include "Slicing.h"
#include "WorkspaceNamesFactory.h"
#include <boost/optional.hpp>
#include <boost/range/algorithm/set_algorithm.hpp>
#include <boost/variant.hpp>
#include <map>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

using ReductionOptionsMap = std::map<std::string, std::string>;

// Immutability here makes update notification easier.
template <typename ReducedWorkspaceNames>
class MANTIDQT_ISISREFLECTOMETRY_DLL Row {
public:
  using WorkspaceNames = ReducedWorkspaceNames;

  Row(std::vector<std::string> number, double theta,
      std::pair<std::string, std::string> tranmissionRuns,
      boost::optional<RangeInQ> qRange, boost::optional<double> scaleFactor,
      ReductionOptionsMap reductionOptions,
      ReducedWorkspaceNames reducedWorkspaceNames);

  std::vector<std::string> const &runNumbers() const;
  std::pair<std::string, std::string> const &transmissionWorkspaceNames() const;
  double theta() const;
  boost::optional<RangeInQ> const &qRange() const;
  boost::optional<double> scaleFactor() const;
  ReductionOptionsMap const &reductionOptions() const;
  ReducedWorkspaceNames const &reducedWorkspaceNames() const;

  template <typename WorkspaceNamesFactory>
  Row withExtraRunNumbers(std::vector<std::string> const &runNumbers,
                          WorkspaceNamesFactory const &workspaceNames) const;

private:
  std::vector<std::string> m_runNumbers;
  double m_theta;
  boost::optional<RangeInQ> m_qRange;
  boost::optional<double> m_scaleFactor;
  std::pair<std::string, std::string> m_transmissionRuns;
  ReducedWorkspaceNames m_reducedWorkspaceNames;
  ReductionOptionsMap m_reductionOptions;
};

template <typename ReducedWorkspaceNames>
std::ostream &operator<<(std::ostream &os,
                         Row<ReducedWorkspaceNames> const &row) {
  auto runNumbers = boost::join(row.runNumbers(), "+");
  os << "Row (runs: " << runNumbers << ", theta: " << row.theta() << ")";
  return os;
}

template <typename ReducedWorkspaceNames>
// cppcheck-suppress syntaxError
template <typename WorkspaceNamesFactory>
Row<ReducedWorkspaceNames> Row<ReducedWorkspaceNames>::withExtraRunNumbers(
    std::vector<std::string> const &extraRunNumbers,
    WorkspaceNamesFactory const &workspaceNamesFactory) const {
  auto newRunNumbers = std::vector<std::string>();
  newRunNumbers.reserve(m_runNumbers.size() + extraRunNumbers.size());
  boost::range::set_union(m_runNumbers, extraRunNumbers,
                          std::back_inserter(newRunNumbers));
  auto wsNames = workspaceNamesFactory.template makeNames<WorkspaceNames>(
      newRunNumbers, transmissionWorkspaceNames());
  return Row(newRunNumbers, theta(), transmissionWorkspaceNames(), qRange(),
             scaleFactor(), reductionOptions(), wsNames);
}

template <typename WorkspaceNames, typename WorkspaceNamesFactory>
Row<WorkspaceNames>
mergedRow(Row<WorkspaceNames> const &rowA, Row<WorkspaceNames> const &rowB,
          WorkspaceNamesFactory const &workspaceNamesFactory) {
  return rowA.withExtraRunNumbers(rowB.runNumbers(), workspaceNamesFactory);
}

using SlicedRow = Row<SlicedReductionWorkspaces>;
using UnslicedRow = Row<ReductionWorkspaces>;
using RowVariant = boost::variant<SlicedRow, UnslicedRow>;

boost::optional<UnslicedRow>
unslice(boost::optional<SlicedRow> const &row,
        WorkspaceNamesFactory const &workspaceNamesFactory);
boost::optional<SlicedRow>
slice(boost::optional<UnslicedRow> const &row,
      WorkspaceNamesFactory const &workspaceNamesFactory);
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACE_RUN_H_
