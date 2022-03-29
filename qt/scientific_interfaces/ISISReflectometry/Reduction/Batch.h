// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "Experiment.h"
#include "IBatch.h"
#include "Instrument.h"
#include "Reduction/LookupRow.h"
#include "RunsTable.h"
#include "Slicing.h"

#include <boost/optional.hpp>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class Batch

    The Batch model holds the entire reduction configuration for a batch of
    runs.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL Batch final : public IBatch {
public:
  Batch(Experiment const &experiment, Instrument const &instrument, RunsTable &runsTable, Slicing const &slicing);

  Experiment const &experiment() const override;
  Instrument const &instrument() const override;
  RunsTable const &runsTable() const override;
  RunsTable &mutableRunsTable() override;
  Slicing const &slicing() const override;

  std::vector<MantidWidgets::Batch::RowLocation> selectedRowLocations() const override;
  bool isInSelection(const Item &item,
                     const std::vector<MantidWidgets::Batch::RowLocation> &selectedRowLocations) override;
  bool isInSelection(const Row &item,
                     const std::vector<MantidWidgets::Batch::RowLocation> &selectedRowLocations) override;
  bool isInSelection(const Group &item,
                     const std::vector<MantidWidgets::Batch::RowLocation> &selectedRowLocations) override;
  boost::optional<LookupRow> findLookupRow(Row const &row) const override;
  boost::optional<LookupRow> findWildcardLookupRow() const override;
  void resetState() override;
  void resetSkippedItems() override;
  boost::optional<Item &> getItemWithOutputWorkspaceOrNone(std::string const &wsName) override;

  void updateLookupIndex(Row &row) override;
  void updateLookupIndexesOfGroup(Group &group) override;
  void updateLookupIndexesOfTable() override;

private:
  Experiment const &m_experiment;
  Instrument const &m_instrument;
  RunsTable &m_runsTable;
  Slicing const &m_slicing;
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
