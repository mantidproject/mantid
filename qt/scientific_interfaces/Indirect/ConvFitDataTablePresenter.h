// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ConvFitModel.h"
#include "IndirectFitDataTablePresenter.h"

#include <QTableWidget>

#include <cstddef>
#include <unordered_map>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/**
  Presenter for a table of convolution fitting data.
*/
class DLLExport ConvFitDataTablePresenter : public IndirectFitDataTablePresenter {
  Q_OBJECT
public:
  ConvFitDataTablePresenter(ConvFitModel *model, QTableWidget *dataTable);

protected:
  void addTableEntry(FitDomainIndex row) override;

private:
  int workspaceIndexColumn() const override;
  int startXColumn() const override;
  int endXColumn() const override;
  int excludeColumn() const override;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
