// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectDataTablePresenter.h"
#include "JumpFitModel.h"

#include <QTableWidget>

#include <cstddef>
#include <unordered_map>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/**
  Presenter for a table of data containing Widths/EISF.
*/
class DLLExport JumpFitDataTablePresenter : public IndirectDataTablePresenter {
  Q_OBJECT
public:
  JumpFitDataTablePresenter(JumpFitModel *model, QTableWidget *dataTable);

protected:
  void addTableEntry(TableDatasetIndex dataIndex, WorkspaceIndex spectrum,
                     TableRowIndex row) override;
  void updateTableEntry(TableDatasetIndex dataIndex, WorkspaceIndex spectrum,
                        TableRowIndex row) override;

private:
  int workspaceIndexColumn() const override;
  int startXColumn() const override;
  int endXColumn() const override;
  int excludeColumn() const override;

  JumpFitModel *m_jumpFitModel;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
