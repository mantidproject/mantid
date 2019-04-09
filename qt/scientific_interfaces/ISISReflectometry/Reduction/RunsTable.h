// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_CUSTOMINTERFACES_RUNSTABLE_H_
#define MANTID_CUSTOMINTERFACES_RUNSTABLE_H_

#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "ReductionJobs.h"
#include "RowLocation.h"
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

/** @class RunsTable

    The RunsTable model holds all information about the table on the Runs tab
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL RunsTable {
public:
  RunsTable(std::vector<std::string> instruments, double thetaTolerance,
            ReductionJobs ReductionJobs);

  double thetaTolerance() const;
  ReductionJobs const &reductionJobs() const;
  ReductionJobs &mutableReductionJobs();
  std::vector<MantidWidgets::Batch::RowLocation> const &
  selectedRowLocations() const;

  bool hasSelection() const;
  void setSelectedRowLocations(
      std::vector<MantidWidgets::Batch::RowLocation> selected);
  template <typename T> bool isSelected(T const &item);
  void resetState();
  void resetSkippedItems();
  boost::optional<Item &>
  getItemWithOutputWorkspaceOrNone(std::string const &wsName);

private:
  std::vector<std::string> m_instruments;
  double m_thetaTolerance;
  ReductionJobs m_reductionJobs;
  std::vector<MantidWidgets::Batch::RowLocation> m_selectedRowLocations;
};

template <typename T> bool RunsTable::isSelected(T const &item) {
  auto const path = m_reductionJobs.getPath(item);
  return containsPath(m_selectedRowLocations, path);
}
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_RUNSTABLE_H_
