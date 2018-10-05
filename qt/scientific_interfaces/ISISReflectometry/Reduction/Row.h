// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_CUSTOMINTERFACES_RUN_H_
#define MANTID_CUSTOMINTERFACES_RUN_H_
#include "../DllConfig.h"
#include "RangeInQ.h"
#include "ReductionOptionsMap.h"
#include "ReductionWorkspaces.h"
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
      std::pair<std::string, std::string> tranmissionRuns, RangeInQ qRange,
      boost::optional<double> scaleFactor, ReductionOptionsMap reductionOptions,
      ReductionWorkspaces reducedWorkspaceNames);

  std::vector<std::string> const &runNumbers() const;
  std::pair<std::string, std::string> const &transmissionWorkspaceNames() const;
  double theta() const;
  RangeInQ const &qRange() const;
  boost::optional<double> scaleFactor() const;
  ReductionOptionsMap const &reductionOptions() const;
  ReductionWorkspaces const &reducedWorkspaceNames() const;

  Row withExtraRunNumbers(std::vector<std::string> const &runNumbers) const;

private:
  std::vector<std::string> m_runNumbers;
  double m_theta;
  RangeInQ m_qRange;
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

MANTIDQT_ISISREFLECTOMETRY_DLL Row mergedRow(Row const &rowA, Row const &rowB);
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACE_RUN_H_
