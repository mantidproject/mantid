// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IAddWorkspaceDialog.h"
#include "IndirectDataAnalysisTab.h"
#include "IndirectFitDataModel.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/FunctionModelSpectra.h"
#include "ui_IndirectDataAnalysisElwinTab.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
using namespace MantidWidgets;

class MANTIDQT_INDIRECT_DLL IndirectDataAnalysisElwinTab : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  IndirectDataAnalysisElwinTab(QWidget *parent = nullptr);
  ~IndirectDataAnalysisElwinTab();
  void updateTableFromModel();
  QTableWidget *getDataTable() const;

  void setAvailableSpectra(WorkspaceIndex minimum, WorkspaceIndex maximum);
  void setAvailableSpectra(const std::vector<WorkspaceIndex>::const_iterator &from,
                           const std::vector<WorkspaceIndex>::const_iterator &to);

public slots:
  void removeSelectedData();
  void updateAvailableSpectra();

protected:
  void addData(IAddWorkspaceDialog const *dialog);
  void checkData(IAddWorkspaceDialog const *dialog);
  void addDataFromFile(IAddWorkspaceDialog const *dialog);
  void newInputFilesFromDialog(IAddWorkspaceDialog const *dialog);
  virtual void addDataToModel(IAddWorkspaceDialog const *dialog);
  virtual void addTableEntry(FitDomainIndex row);
  void setCell(std::unique_ptr<QTableWidgetItem> cell, FitDomainIndex row, int column);
  void setCellText(const QString &text, FitDomainIndex row, int column);

protected slots:
  void showAddWorkspaceDialog();
  virtual void closeDialog();

signals:
  void dataAdded();
  void dataRemoved();
  void dataChanged();

private:
  void run() override;
  void runFileInput();
  void runWorkspaceInput();
  void setup() override;
  bool validate() override;
  void loadTabSettings(const QSettings &settings);
  void setFileExtensionsByName(bool filter) override;
  void setBrowserWorkspace() override{};
  void setDefaultResolution(const Mantid::API::MatrixWorkspace_const_sptr &ws, const QPair<double, double> &range);
  void setDefaultSampleLog(const Mantid::API::MatrixWorkspace_const_sptr &ws);

  void checkForELTWorkspace();

  std::vector<std::string> getOutputWorkspaceNames();
  QString getOutputBasename();

  void setRunIsRunning(const bool &running);
  void setButtonsEnabled(const bool &enabled);
  void setRunEnabled(const bool &enabled);
  void setSaveResultEnabled(const bool &enabled);

  virtual std::unique_ptr<IAddWorkspaceDialog> getAddWorkspaceDialog(QWidget *parent) const;

  std::unique_ptr<IAddWorkspaceDialog> m_addWorkspaceDialog;
  Ui::IndirectDataAnalysisElwinTab m_uiForm;
  QtTreePropertyBrowser *m_elwTree;
  IndirectDataAnalysis *m_parent;
  QTableWidget *m_dataTable;
  std::unique_ptr<IndirectFitDataModel> m_dataModel;

  bool m_emitCellChanged = true;

  virtual int workspaceIndexColumn() const;

  void setHorizontalHeaders(const QStringList &headers);

  void newPreviewFileSelected(const QString &workspaceName, const QString &filename);
  void newPreviewWorkspaceSelected(const QString &workspaceName);
  size_t findWorkspaceID();
  void newInputFiles();
  void plotInput();
  void updateIntegrationRange();

private slots:
  void checkNewPreviewSelected(int index);
  void handlePreviewSpectrumChanged();
  void twoRanges(QtProperty *prop, bool enabled);
  void minChanged(double val);
  void maxChanged(double val);
  void updateRS(QtProperty *prop, double val);
  void unGroupInput(bool error);
  void runClicked();
  void saveClicked();
  void addData();
  void checkLoadedFiles();
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
