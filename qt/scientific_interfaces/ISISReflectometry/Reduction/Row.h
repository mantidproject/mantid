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
#include "ReductionOptionsMap.h"
#include "ReductionWorkspaces.h"
#include "WorkspaceNamesFactory.h"
#include <boost/optional.hpp>
#include <boost/range/algorithm/set_algorithm.hpp>
#include <boost/variant.hpp>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

// Immutability here makes update notification easier.
class MANTIDQT_ISISREFLECTOMETRY_DLL Row {
public:
  Row(std::vector<std::string> number, double theta,
      std::pair<std::string, std::string> tranmissionRuns,
      boost::optional<RangeInQ> qRange, boost::optional<double> scaleFactor,
      ReductionOptionsMap reductionOptions,
      ReductionWorkspaces reducedWorkspaceNames);

  std::vector<std::string> const &runNumbers() const;
  std::pair<std::string, std::string> const &transmissionWorkspaceNames() const;
  double theta() const;
  boost::optional<RangeInQ> const &qRange() const;
  boost::optional<double> scaleFactor() const;
  ReductionOptionsMap const &reductionOptions() const;
  ReductionWorkspaces const &reducedWorkspaceNames() const;

  template <typename WorkspaceNamesFactory>
  Row withExtraRunNumbers(std::vector<std::string> const &runNumbers,
                          WorkspaceNamesFactory const &workspaceNames) const;

private:
  std::vector<std::string> m_runNumbers;
  double m_theta;
  boost::optional<RangeInQ> m_qRange;
  boost::optional<double> m_scaleFactor;
  std::pair<std::string, std::string> m_transmissionRuns;
  ReductionWorkspaces m_reducedWorkspaceNames;
  ReductionOptionsMap m_reductionOptions;
};

// std::ostream &operator<<(std::ostream &os, Row const &row) {
//  auto runNumbers = boost::join(row.runNumbers(), "+");
//  os << "Row (runs: " << runNumbers << ", theta: " << row.theta() << ")";
//  return os;
//}

template <typename WorkspaceNamesFactory>
Row Row::withExtraRunNumbers(
    std::vector<std::string> const &extraRunNumbers,
    WorkspaceNamesFactory const &workspaceNamesFactory) const {
  auto newRunNumbers = std::vector<std::string>();
  newRunNumbers.reserve(m_runNumbers.size() + extraRunNumbers.size());
  boost::range::set_union(m_runNumbers, extraRunNumbers,
                          std::back_inserter(newRunNumbers));
  auto wsNames = workspaceNamesFactory.template makeNames(
      newRunNumbers, transmissionWorkspaceNames());
  return Row(newRunNumbers, theta(), transmissionWorkspaceNames(), qRange(),
             scaleFactor(), reductionOptions(), wsNames);
}

template <typename WorkspaceNamesFactory>
Row mergedRow(Row const &rowA, Row const &rowB,
              WorkspaceNamesFactory const &workspaceNamesFactory) {
  return rowA.withExtraRunNumbers(rowB.runNumbers(), workspaceNamesFactory);
}
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACE_RUN_H_
