// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidQtWidgets/Common/IndexTypes.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <QTableWidget>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

class IFitDataPresenter;

struct FitDataRow {
  std::string name;
  std::string exclude;
  size_t workspaceIndex;
  double startX;
  double endX;
  std::string resolution;
  std::string parameter;
};

class MANTIDQT_INELASTIC_DLL IFitDataView {

public:
  virtual void subscribePresenter(IFitDataPresenter *presenter) = 0;

  virtual QTableWidget *getDataTable() const = 0;
  virtual bool isTableEmpty() const = 0;

  virtual void validate(IUserInputValidator *validator) = 0;
  virtual void addTableEntry(size_t row, FitDataRow const &newRow) = 0;
  virtual void updateNumCellEntry(double numEntry, size_t row, size_t column) = 0;
  virtual int columnIndex(std::string const &name) const = 0;
  virtual void clearTable() = 0;
  virtual QString getText(int row, int column) const = 0;
  virtual QModelIndexList getSelectedIndexes() const = 0;
  virtual bool columnContains(std::string const &columnHeader, std::string const &text) const = 0;

  virtual void displayWarning(std::string const &warning) = 0;
};
} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
