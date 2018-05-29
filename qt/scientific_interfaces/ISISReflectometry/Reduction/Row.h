#ifndef MANTID_CUSTOMINTERFACES_RUN_H_
#define MANTID_CUSTOMINTERFACES_RUN_H_
#include <string>
#include <vector>
#include <unordered_map>
#include <boost/variant.hpp>
#include "RangeInQ.h"
#include "ReductionWorkspaces.h"
#include "SlicedReductionWorkspaces.h"
#include "../DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

using ReductionOptionsMap = std::unordered_map<std::string, std::string>;

// Immutability here makes update notification easier.
template <typename ReducedWorkspaceNames> class Row {
public:
  Row(std::vector<std::string> number, double theta,
      std::pair<std::string, std::string> tranmissionRuns, RangeInQ qRange,
      double scaleFactor,
      boost::optional<ReducedWorkspaceNames> reducedWorkspaceNames,
      ReductionOptionsMap reductionOptions = ReductionOptionsMap());

  std::vector<std::string> const &runNumbers() const;
  std::pair<std::string, std::string> transmissionWorkspaceNames();
  double theta() const;
  RangeInQ const &qRange() const;
  double scaleFactor() const;
  ReductionOptionsMap const &reductionOptions() const;
  boost::optional<ReducedWorkspaceNames> const &reducedWorkspaceNames() const;

  Row withRunNumbers(std::vector<std::string> const &rows) const;
  Row withTheta(double theta) const;
  Row withQRange(RangeInQ const &qRange) const;
  Row withScaleFactor(double scaleFactor) const;
  Row withReductionOptions(ReductionOptionsMap const &reductionOptions) const;
  Row withTransmissionRunNumbers(
      std::pair<std::string, std::string> const &tranmissionRuns) const;
  Row withReducedWorkspaceNames(
      boost::optional<ReducedWorkspaceNames> const &workspaceNames) const;

private:
  std::vector<std::string> m_runNumbers;
  double m_theta;
  RangeInQ m_qRange;
  double m_scaleFactor;
  std::pair<std::string, std::string> m_transmissionRuns;
  boost::optional<ReducedWorkspaceNames> m_reducedWorkspaceNames;
  std::unordered_map<std::string, std::string> m_reductionOptions;
};

template <typename ReducedWorkspaceNames>
bool operator==(Row<ReducedWorkspaceNames> const &lhs,
                Row<ReducedWorkspaceNames> const &rhs);

template <typename ReducedWorkspaceNames>
bool operator!=(Row<ReducedWorkspaceNames> const &lhs,
                Row<ReducedWorkspaceNames> const &rhs);

extern template class MANTIDQT_ISISREFLECTOMETRY_DLL
    Row<SlicedReductionWorkspaces>;
using SlicedRow = Row<SlicedReductionWorkspaces>;
extern template MANTIDQT_ISISREFLECTOMETRY_DLL bool
operator==(SlicedRow const &, SlicedRow const &);
extern template MANTIDQT_ISISREFLECTOMETRY_DLL bool
operator!=(SlicedRow const &, SlicedRow const &);

extern template class MANTIDQT_ISISREFLECTOMETRY_DLL Row<ReductionWorkspaces>;
using SingleRow = Row<ReductionWorkspaces>;
extern template MANTIDQT_ISISREFLECTOMETRY_DLL bool
operator==(SingleRow const &, SingleRow const &);
extern template MANTIDQT_ISISREFLECTOMETRY_DLL bool
operator!=(SingleRow const &, SingleRow const &);

// class RowTemplate {
// public:
//  std::pair<std::string, std::string> TransmissionRowNumbers;
//  RangeInQ QRange;
//  double ScaleFactor;
//  std::string ProcessingInstructions;
//};
}
}
#endif // MANTID_CUSTOMINTERFACES_RUN_H_
