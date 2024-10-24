// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Item.h"
#include "Row.h"
#include <boost/optional.hpp>
#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class IGroup : public Item {
public:
  virtual ~IGroup() = default;

  virtual std::string const &name() const = 0;
  virtual void setName(std::string const &name) = 0;
  virtual bool hasPostprocessing() const = 0;
  virtual bool requiresPostprocessing(bool reprocessFailed) const = 0;
  virtual std::string postprocessedWorkspaceName() const = 0;

  virtual void appendEmptyRow() = 0;
  virtual void appendRow(boost::optional<Row> const &row) = 0;
  virtual void insertRow(boost::optional<Row> const &row, int beforeRowAtIndex) = 0;
  virtual int insertRowSortedByAngle(boost::optional<Row> const &row) = 0;
  virtual void removeRow(int rowIndex) = 0;
  virtual void updateRow(int rowIndex, boost::optional<Row> const &row) = 0;

  virtual void resetSkipped() = 0;

  virtual boost::optional<int> indexOfRowWithTheta(double angle, double tolerance) const = 0;

  virtual boost::optional<Row> const &operator[](int rowIndex) const = 0;
  virtual std::vector<boost::optional<Row>> const &rows() const = 0;
  virtual std::vector<boost::optional<Row>> &mutableRows() = 0;

  virtual boost::optional<Item &> getItemWithOutputWorkspaceOrNone(std::string const &wsName) = 0;

  virtual void setAllRowParents() = 0;

  virtual void notifyChildStateChanged() = 0;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
