// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "FitDataView.h"

#include "DllConfig.h"

#include <QTabWidget>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {
class FunctionQAddWorkspaceDialog;

/**
Presenter for a table of convolution fitting data.
*/
class MANTIDQT_INELASTIC_DLL FunctionQDataView : public FitDataView {
  Q_OBJECT
public:
  FunctionQDataView(QWidget *parent);
  void addTableEntry(size_t row, FitDataRow const &newRow) override;

protected:
  FunctionQDataView(const QStringList &headers, QWidget *parent);

protected slots:
  void showAddWorkspaceDialog() override;

private slots:
  void notifyAddClicked();
  void notifyWorkspaceChanged(FunctionQAddWorkspaceDialog *dialog, const std::string &workspaceName);
  void notifyParameterTypeChanged(FunctionQAddWorkspaceDialog *dialog, const std::string &type);
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
