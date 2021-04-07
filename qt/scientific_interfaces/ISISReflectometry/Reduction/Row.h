// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "Item.h"
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
namespace ISISReflectometry {

/** @class Row

  The Row model represents a single row in the runs table
*/
// Immutability here makes update notification easier.
class MANTIDQT_ISISREFLECTOMETRY_DLL Row : public Item {
public:
  Row(std::vector<std::string> number, double theta, TransmissionRunPair tranmissionRuns, RangeInQ qRange,
      boost::optional<double> scaleFactor, ReductionOptionsMap reductionOptions,
      ReductionWorkspaces reducedWorkspaceNames);

  bool isGroup() const override;
  std::vector<std::string> const &runNumbers() const;
  TransmissionRunPair const &transmissionWorkspaceNames() const;
  double theta() const;
  RangeInQ const &qRange() const;
  RangeInQ const &qRangeOutput() const;
  boost::optional<double> scaleFactor() const;
  ReductionOptionsMap const &reductionOptions() const;
  ReductionWorkspaces const &reducedWorkspaceNames() const;

  void setOutputNames(std::vector<std::string> const &outputNames) override;
  void setOutputQRange(RangeInQ qRange);
  void resetOutputs() override;
  bool hasOutputWorkspace(std::string const &wsName) const;
  void renameOutputWorkspace(std::string const &oldName, std::string const &newName) override;

  Row withExtraRunNumbers(std::vector<std::string> const &runNumbers) const;

  int totalItems() const override;
  int completedItems() const override;

private:
  std::vector<std::string> m_runNumbers;
  double m_theta;
  RangeInQ m_qRange;       // user-defined Q values
  RangeInQ m_qRangeOutput; // output Q values if inputs were not specified
  boost::optional<double> m_scaleFactor;
  TransmissionRunPair m_transmissionRuns;
  ReductionWorkspaces m_reducedWorkspaceNames;
  ReductionOptionsMap m_reductionOptions;

  friend class Encoder;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(Row const &lhs, Row const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(Row const &lhs, Row const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL Row mergedRow(Row const &rowA, Row const &rowB);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt