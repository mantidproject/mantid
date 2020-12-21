// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "ui_FitScriptGenerator.h"

#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/AddWorkspaceDialog.h"
#include "MantidQtWidgets/Common/FitOptionsBrowser.h"
#include "MantidQtWidgets/Common/FunctionTreeView.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorView.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

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
class IFitScriptGeneratorPresenter;
struct GlobalTie;

class EXPORT_OPT_MANTIDQT_COMMON FitScriptGeneratorView
    : public IFitScriptGeneratorView {
  Q_OBJECT

public:
  FitScriptGeneratorView(
      QWidget *parent = nullptr,
      QMap<QString, QString> const &fitOptions = QMap<QString, QString>());
  ~FitScriptGeneratorView() override;

  void subscribePresenter(IFitScriptGeneratorPresenter *presenter) override;

  [[nodiscard]] std::string workspaceName(FitDomainIndex index) const override;
  [[nodiscard]] WorkspaceIndex
  workspaceIndex(FitDomainIndex index) const override;
  [[nodiscard]] double startX(FitDomainIndex index) const override;
  [[nodiscard]] double endX(FitDomainIndex index) const override;

  [[nodiscard]] std::vector<FitDomainIndex> allRows() const override;
  [[nodiscard]] std::vector<FitDomainIndex> selectedRows() const override;

  [[nodiscard]] double
  parameterValue(std::string const &parameter) const override;
  [[nodiscard]] Mantid::API::IFunction::Attribute
  attributeValue(std::string const &attribute) const override;

  void removeWorkspaceDomain(std::string const &workspaceName,
                             WorkspaceIndex workspaceIndex) override;
  void addWorkspaceDomain(std::string const &workspaceName,
                          WorkspaceIndex workspaceIndex, double startX,
                          double endX) override;

  [[nodiscard]] bool openAddWorkspaceDialog() override;
  [[nodiscard]] std::vector<Mantid::API::MatrixWorkspace_const_sptr>
  getDialogWorkspaces() override;
  [[nodiscard]] std::vector<WorkspaceIndex>
  getDialogWorkspaceIndices() const override;

  void resetSelection() override;

  bool isApplyFunctionChangesToAllChecked() const override;

  void clearFunction() override;
  void setFunction(Mantid::API::IFunction_sptr const &function) const override;

  void setSimultaneousMode(bool simultaneousMode) override;

  void setGlobalTies(std::vector<GlobalTie> const &globalTies) override;

  void displayWarning(std::string const &message) override;

public:
  /// Testing accessors
  FitScriptGeneratorDataTable *tableWidget() const override {
    return m_dataTable.get();
  }
  QPushButton *removeButton() const override { return m_ui.pbRemove; }
  QPushButton *addWorkspaceButton() const override {
    return m_ui.pbAddWorkspace;
  }
  AddWorkspaceDialog *addWorkspaceDialog() const override {
    return m_dialog.get();
  }

private slots:
  void onRemoveClicked();
  void onAddWorkspaceClicked();
  void onCellChanged(int row, int column);
  void onItemPressed();
  void onFunctionRemoved(QString const &function);
  void onFunctionAdded(QString const &function);
  void onFunctionReplaced(QString const &function);
  void onParameterChanged(QString const &parameter);
  void onAttributeChanged(QString const &attribute);
  void onParameterTieChanged(QString const &parameter, QString const &tie);
  void onCopyFunctionToClipboard();
  void onFunctionHelpRequested();
  void onChangeToSequentialFitting();
  void onChangeToSimultaneousFitting();

private:
  void connectUiSignals();

  void setFitBrowserOptions(QMap<QString, QString> const &fitOptions);
  void setFitBrowserOption(QString const &name, QString const &value);
  void setFittingType(QString const &fitType);

  IFitScriptGeneratorPresenter *m_presenter;
  std::unique_ptr<AddWorkspaceDialog> m_dialog;
  std::unique_ptr<FitScriptGeneratorDataTable> m_dataTable;
  std::unique_ptr<FunctionTreeView> m_functionTreeView;
  std::unique_ptr<FitOptionsBrowser> m_fitOptionsBrowser;
  Ui::FitScriptGenerator m_ui;
};

} // namespace MantidWidgets
} // namespace MantidQt
