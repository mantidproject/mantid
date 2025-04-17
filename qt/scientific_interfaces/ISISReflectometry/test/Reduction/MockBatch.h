// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../Reduction/Experiment.h"
#include "../../Reduction/IBatch.h"
#include "../../Reduction/Instrument.h"
#include "../../Reduction/LookupRow.h"
#include "../../Reduction/RunsTable.h"
#include "MantidFrameworkTestHelpers/FallbackBoostOptionalIO.h"

#include <gmock/gmock.h>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class MockBatch : public IBatch {
public:
  virtual ~MockBatch() = default;
  MOCK_METHOD(Experiment const &, experiment, (), (const, override));
  MOCK_METHOD(Instrument const &, instrument, (), (const, override));
  MOCK_METHOD(RunsTable &, mutableRunsTable, (), (override));
  MOCK_METHOD(RunsTable const &, runsTable, (), (const, override));
  MOCK_METHOD(Slicing const &, slicing, (), (const, override));

  MOCK_METHOD(std::optional<LookupRow>, findLookupRowProxy, (Row const &), (const));
  MOCK_METHOD(std::optional<LookupRow>, findLookupPreviewRowProxy, (PreviewRow const &), (const));
  MOCK_METHOD(std::optional<LookupRow>, findWildcardLookupRow, (), (const, override));
  MOCK_METHOD(std::optional<std::reference_wrapper<Item>>, getItemWithOutputWorkspaceOrNone, (std::string const &),
              (override));
  MOCK_METHOD(bool, isInSelection, (const Item &, const std::vector<MantidWidgets::Batch::RowLocation> &), (override));
  MOCK_METHOD(bool, isInSelection, (const Row &, const std::vector<MantidWidgets::Batch::RowLocation> &), (override));
  MOCK_METHOD(bool, isInSelection, (const Group &, const std::vector<MantidWidgets::Batch::RowLocation> &), (override));
  MOCK_METHOD(void, resetSkippedItems, (), (override));
  MOCK_METHOD(void, resetState, (), (override));
  MOCK_METHOD(std::vector<MantidWidgets::Batch::RowLocation>, selectedRowLocations, (), (const, override));
  MOCK_METHOD(void, updateLookupIndex, (Row &), (override));
  MOCK_METHOD(void, updateLookupIndexesOfGroup, (Group &), (override));
  MOCK_METHOD(void, updateLookupIndexesOfTable, (), (override));

  // gtest struggles with the combination of overrides and references, so create proxies
  std::optional<LookupRow> findLookupRow(Row const &row) const override { return findLookupRowProxy(row); }
  std::optional<LookupRow> findLookupRow(PreviewRow const &row) const override {
    return findLookupPreviewRowProxy(row);
  }
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
