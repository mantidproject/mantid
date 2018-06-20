#ifndef MANTID_CUSTOMINTERFACES_RUN_H_
#define MANTID_CUSTOMINTERFACES_RUN_H_
#include <string>
#include <vector>
#include <map>
#include <boost/variant.hpp>
#include <boost/range/algorithm/set_algorithm.hpp>
#include <boost/optional.hpp>
#include "RangeInQ.h"
#include "WorkspaceNamesFactory.h"
#include "ReductionWorkspaces.h"
#include "SlicedReductionWorkspaces.h"
#include "Slicing.h"
#include "../DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

using ReductionOptionsMap = std::map<std::string, std::string>;

// Immutability here makes update notification easier.
template <typename ReducedWorkspaceNames> class Row {
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
                         Row<ReducedWorkspaceNames> const &row);

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

extern template class MANTIDQT_ISISREFLECTOMETRY_DLL
    Row<SlicedReductionWorkspaces>;
using SlicedRow = Row<SlicedReductionWorkspaces>;

extern template class MANTIDQT_ISISREFLECTOMETRY_DLL Row<ReductionWorkspaces>;
using UnslicedRow = Row<ReductionWorkspaces>;

extern template MANTIDQT_ISISREFLECTOMETRY_DLL std::ostream &
operator<<(std::ostream &, UnslicedRow const &);
extern template MANTIDQT_ISISREFLECTOMETRY_DLL std::ostream &
operator<<(std::ostream &, SlicedRow const &);

using RowVariant = boost::variant<SlicedRow, UnslicedRow>;

boost::optional<UnslicedRow>
unslice(boost::optional<SlicedRow> const &row,
        WorkspaceNamesFactory const &workspaceNamesFactory);
boost::optional<SlicedRow>
slice(boost::optional<UnslicedRow> const &row,
      WorkspaceNamesFactory const &workspaceNamesFactory);
}
}
#endif // MANTID_CUSTOMINTERFACE_RUN_H_
