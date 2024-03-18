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
namespace IDA {
class FqFitAddWorkspaceDialog;

/**
Presenter for a table of convolution fitting data.
*/
class MANTIDQT_INELASTIC_DLL FqFitDataView : public FitDataView {
  Q_OBJECT
public:
  FqFitDataView(QWidget *parent = nullptr);
  void addTableEntry(size_t row, FitDataRow newRow) override;

protected:
  FqFitDataView(const QStringList &headers, QWidget *parent = nullptr);

protected slots:
  void showAddWorkspaceDialog() override;

private slots:
  void notifyAddClicked();
  void notifyWorkspaceChanged(FqFitAddWorkspaceDialog *dialog, const std::string &workspaceName);
  void notifyParameterTypeChanged(FqFitAddWorkspaceDialog *dialog, const std::string &type);
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
