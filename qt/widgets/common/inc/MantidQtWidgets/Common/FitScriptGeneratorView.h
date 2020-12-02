// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "ui_FitScriptGenerator.h"

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/AddWorkspaceDialog.h"
#include "MantidQtWidgets/Common/FitOptionsBrowser.h"
#include "MantidQtWidgets/Common/FunctionBrowser.h"
#include "MantidQtWidgets/Common/IndexTypes.h"
#include "MantidQtWidgets/Common/MantidWidget.h"

#include <memory>
#include <string>
#include <vector>

#include <QMap>
#include <QString>
#include <QStringList>
#include <QWidget>

namespace MantidQt {
namespace MantidWidgets {

class FitScriptGeneratorDataTable;
class FitScriptGeneratorPresenter;

class EXPORT_OPT_MANTIDQT_COMMON FitScriptGeneratorView
    : public API::MantidWidget {
  Q_OBJECT

public:
  enum class Event { AddClicked, RemoveClicked, StartXChanged, EndXChanged };

  FitScriptGeneratorView(
      QWidget *parent = nullptr,
      QMap<QString, QString> const &fitOptions = QMap<QString, QString>());
  ~FitScriptGeneratorView() override;

  void subscribePresenter(FitScriptGeneratorPresenter *presenter);

  [[nodiscard]] std::string workspaceName(FitDomainIndex index) const;
  [[nodiscard]] WorkspaceIndex workspaceIndex(FitDomainIndex index) const;
  [[nodiscard]] double startX(FitDomainIndex index) const;
  [[nodiscard]] double endX(FitDomainIndex index) const;

  [[nodiscard]] std::vector<FitDomainIndex> selectedRows() const;

  void removeWorkspaceDomain(std::string const &workspaceName,
                             WorkspaceIndex workspaceIndex);
  void addWorkspaceDomain(std::string const &workspaceName,
                          WorkspaceIndex workspaceIndex, double startX,
                          double endX);

  [[nodiscard]] bool openAddWorkspaceDialog();
  [[nodiscard]] std::vector<Mantid::API::MatrixWorkspace_const_sptr>
  getDialogWorkspaces();
  [[nodiscard]] std::vector<WorkspaceIndex> getDialogWorkspaceIndices() const;

  void resetSelection();

  void displayWarning(std::string const &message);

private slots:
  void onRemoveClicked();
  void onAddWorkspaceClicked();
  void onCellChanged(int row, int column);

private:
  void connectUiSignals();

  void setFitBrowserOptions(QMap<QString, QString> const &fitOptions);
  void setFitBrowserOption(QString const &name, QString const &value);
  void setFittingType(QString const &fitType);

  FitScriptGeneratorPresenter *m_presenter;
  AddWorkspaceDialog m_dialog;
  std::unique_ptr<FitScriptGeneratorDataTable> m_dataTable;
  std::unique_ptr<FunctionBrowser> m_functionBrowser;
  std::unique_ptr<FitOptionsBrowser> m_fitOptionsBrowser;
  Ui::FitScriptGenerator m_ui;
};

} // namespace MantidWidgets
} // namespace MantidQt
