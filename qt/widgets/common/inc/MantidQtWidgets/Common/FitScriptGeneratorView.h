// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "ui_FitScriptGenerator.h"

#include "MantidAPI/AnalysisDataServiceObserver.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/AddWorkspaceDialog.h"
#include "MantidQtWidgets/Common/FitScriptOptionsBrowser.h"
#include "MantidQtWidgets/Common/FittingMode.h"
#include "MantidQtWidgets/Common/FunctionModelSpectra.h"
#include "MantidQtWidgets/Common/FunctionTreeView.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorView.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <QCloseEvent>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QWidget>

namespace MantidQt {
namespace MantidWidgets {

class FitScriptGeneratorDataTable;
class EditLocalParameterDialog;
class IFitScriptGeneratorPresenter;
struct GlobalParameter;
struct GlobalTie;

class EXPORT_OPT_MANTIDQT_COMMON FitScriptGeneratorView : public IFitScriptGeneratorView,
                                                          public Mantid::API::AnalysisDataServiceObserver {
  Q_OBJECT

public:
  FitScriptGeneratorView(QWidget *parent = nullptr, FittingMode fittingMode = FittingMode::SEQUENTIAL,
                         QMap<QString, QString> const &fitOptions = QMap<QString, QString>());
  ~FitScriptGeneratorView() override;

  void subscribePresenter(IFitScriptGeneratorPresenter *presenter) override;

  void deleteHandle(std::string const &wsName, [[maybe_unused]] Workspace_sptr const &ws) override;
  void clearHandle() override;
  void renameHandle(std::string const &wsName, std::string const &newName) override;

  [[nodiscard]] std::string workspaceName(FitDomainIndex index) const override;
  [[nodiscard]] WorkspaceIndex workspaceIndex(FitDomainIndex index) const override;
  [[nodiscard]] double startX(FitDomainIndex index) const override;
  [[nodiscard]] double endX(FitDomainIndex index) const override;

  [[nodiscard]] std::vector<FitDomainIndex> allRows() const override;
  [[nodiscard]] std::vector<FitDomainIndex> selectedRows() const override;
  [[nodiscard]] FitDomainIndex currentRow() const override;

  [[nodiscard]] bool hasLoadedData() const override;

  [[nodiscard]] double parameterValue(std::string const &parameter) const override;
  [[nodiscard]] Mantid::API::IFunction::Attribute attributeValue(std::string const &attribute) const override;

  void renameWorkspace(std::string const &workspaceName, std::string const &newName) override;

  void removeDomain(FitDomainIndex domainIndex) override;
  void addWorkspaceDomain(std::string const &workspaceName, WorkspaceIndex workspaceIndex, double startX,
                          double endX) override;

  void openAddWorkspaceDialog() override;
  [[nodiscard]] std::vector<Mantid::API::MatrixWorkspace_const_sptr>
  getDialogWorkspaces(MantidWidgets::IAddWorkspaceDialog *dialog) override;

  void openEditLocalParameterDialog(std::string const &parameter, std::vector<std::string> const &workspaceNames,
                                    std::vector<std::string> const &domainNames, std::vector<double> const &values,
                                    std::vector<bool> const &fixes, std::vector<std::string> const &ties,
                                    std::vector<std::string> const &constraints) override;
  std::tuple<std::string, std::vector<double>, std::vector<bool>, std::vector<std::string>, std::vector<std::string>>
  getEditLocalParameterResults() const override;

  [[nodiscard]] std::tuple<std::string, std::string, std::string, std::string, std::string, bool>
  fitOptions() const override;
  [[nodiscard]] std::string filepath() const override;

  void resetSelection() override;

  bool applyFunctionChangesToAll() const override;

  void clearFunction() override;
  void setFunction(Mantid::API::IFunction_sptr const &function) const override;

  void setSimultaneousMode(bool simultaneousMode) override;

  void setGlobalTies(std::vector<GlobalTie> const &globalTies) override;
  void setGlobalParameters(std::vector<GlobalParameter> const &globalParameter) override;

  void displayWarning(std::string const &message) override;

  void setSuccessText(std::string const &text) override;
  void saveTextToClipboard(std::string const &text) const override;

public:
  /// Testing accessors
  FitScriptGeneratorDataTable *tableWidget() const override { return m_dataTable.get(); }
  QPushButton *removeButton() const override { return m_ui.pbRemoveDomain; }
  QPushButton *addWorkspaceButton() const override { return m_ui.pbAddDomain; }
  QPushButton *generateScriptToFileButton() const override { return m_ui.pbGenerateScriptToFile; }
  QPushButton *generateScriptToClipboardButton() const override { return m_ui.pbGenerateScriptToClipboard; }

private slots:
  void notifyADSDeleteEvent(std::string const &workspaceName);
  void notifyADSClearEvent();
  void notifyADSRenameEvent(std::string const &workspaceName, std::string const &newName);

  void addWorkspaceDialogAccepted(MantidWidgets::IAddWorkspaceDialog *dialog);

  void onRemoveDomainClicked();
  void onAddDomainClicked();
  void onCellChanged(int row, int column);
  void onItemSelected();
  void onFunctionRemoved(std::string const &function);
  void onFunctionAdded(std::string const &function);
  void onFunctionReplaced(std::string const &function);
  void onParameterChanged(std::string const &parameter);
  void onAttributeChanged(std::string const &attribute);
  void onParameterTieChanged(std::string const &parameter, std::string const &tie);
  void onParameterConstraintRemoved(std::string const &parameter);
  void onParameterConstraintChanged(std::string const &functionIndex, std::string const &constraint);
  void onGlobalParametersChanged(std::vector<std::string> const &globalParameters);
  void onCopyFunctionToClipboard();
  void onFunctionHelpRequested();
  void onOutputBaseNameChanged(std::string const &outputBaseName);
  void onFittingModeChanged(FittingMode fittingMode);
  void onEditLocalParameterClicked(std::string const &parameter);
  void onEditLocalParameterFinished(int result);
  void onGenerateScriptToFileClicked();
  void onGenerateScriptToClipboardClicked();
  void onHelpClicked();

private:
  void connectUiSignals();

  void setFitBrowserOptions(QMap<QString, QString> const &fitOptions);
  void setFittingMode(FittingMode fittingMode);

  IFitScriptGeneratorPresenter *m_presenter;
  std::unique_ptr<FitScriptGeneratorDataTable> m_dataTable;
  std::unique_ptr<FunctionTreeView> m_functionTreeView;
  std::unique_ptr<FitScriptOptionsBrowser> m_fitOptionsBrowser;
  EditLocalParameterDialog *m_editLocalParameterDialog;
  Ui::FitScriptGenerator m_ui;
};

} // namespace MantidWidgets
} // namespace MantidQt
