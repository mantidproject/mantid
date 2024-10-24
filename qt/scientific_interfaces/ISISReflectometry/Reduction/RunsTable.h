// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "ReductionJobs.h"
#include "RowLocation.h"
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class RunsTable

    The RunsTable model holds all information about the table on the Runs tab
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL RunsTable {
public:
  RunsTable(std::vector<std::string> instruments, double thetaTolerance, ReductionJobs ReductionJobs);

  double thetaTolerance() const;
  ReductionJobs const &reductionJobs() const;
  ReductionJobs &mutableReductionJobs();
  std::vector<MantidWidgets::Batch::RowLocation> const &selectedRowLocations() const;

  void setSelectedRowLocations(std::vector<MantidWidgets::Batch::RowLocation> selected);
  void appendSelectedRowLocations(MantidWidgets::Batch::RowLocation selectedRowLocation);
  template <typename T>
  bool isInSelection(T const &item, std::vector<MantidWidgets::Batch::RowLocation> const &selectedRowLocations) const;
  void resetState();
  void resetSkippedItems();
  boost::optional<Item &> getItemWithOutputWorkspaceOrNone(std::string const &wsName);
  std::vector<const Group *> selectedGroups() const;
  std::vector<Row> selectedRows() const;

private:
  std::vector<std::string> m_instruments;
  double m_thetaTolerance;
  ReductionJobs m_reductionJobs;
  std::vector<MantidWidgets::Batch::RowLocation> m_selectedRowLocations;

  friend class Encoder;
  friend class Decoder;
  friend class CoderCommonTester;
};

template <typename T>
bool RunsTable::isInSelection(T const &item,
                              std::vector<MantidWidgets::Batch::RowLocation> const &selectedRowLocations) const {
  auto const location = m_reductionJobs.getLocation(item);
  return containsPath(selectedRowLocations, location);
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
