// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "LookupRow.h"
#include "Reduction/RowLocation.h"
#include "Slicing.h"

#include <boost/optional.hpp>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class Experiment;
class Instrument;
class Row;
class Group;
class Item;
class RunsTable;

class IBatch {
public:
  virtual ~IBatch() = default;

  virtual Experiment const &experiment() const = 0;
  virtual Instrument const &instrument() const = 0;
  virtual RunsTable &mutableRunsTable() = 0;
  virtual RunsTable const &runsTable() const = 0;
  virtual Slicing const &slicing() const = 0;

  virtual boost::optional<LookupRow> findLookupRow(Row const &row) const = 0;
  virtual boost::optional<LookupRow> findWildcardLookupRow() const = 0;
  virtual boost::optional<Item &> getItemWithOutputWorkspaceOrNone(std::string const &wsName) = 0;
  virtual bool isInSelection(const Item &item,
                             const std::vector<MantidWidgets::Batch::RowLocation> &selectedRowLocations) = 0;
  virtual bool isInSelection(const Row &item,
                             const std::vector<MantidWidgets::Batch::RowLocation> &selectedRowLocations) = 0;
  virtual bool isInSelection(const Group &item,
                             const std::vector<MantidWidgets::Batch::RowLocation> &selectedRowLocations) = 0;
  virtual void resetSkippedItems() = 0;
  virtual void resetState() = 0;
  virtual std::vector<MantidWidgets::Batch::RowLocation> selectedRowLocations() const = 0;
  virtual void updateLookupIndex(Row &row) = 0;
  virtual void updateLookupIndexesOfGroup(Group &group) = 0;
  virtual void updateLookupIndexesOfTable() = 0;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
