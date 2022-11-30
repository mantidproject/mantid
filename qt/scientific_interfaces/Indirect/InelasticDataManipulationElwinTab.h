// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IAddWorkspaceDialog.h"
#include "IndirectFitDataModel.h"
#include "InelasticDataManipulation.h"
#include "InelasticDataManipulationElwinTabModel.h"
#include "InelasticDataManipulationElwinTabView.h"
#include "InelasticDataManipulationTab.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/FunctionModelSpectra.h"
#include "ui_InelasticDataManipulationElwinTab.h"

namespace MantidQt {
namespace CustomInterfaces {
using namespace Mantid::API;
using namespace MantidWidgets;
using namespace IDA;

class MANTIDQT_INDIRECT_DLL InelasticDataManipulationElwinTab : public InelasticDataManipulationTab {
  Q_OBJECT

public:
  InelasticDataManipulationElwinTab(QWidget *parent = nullptr);
  ~InelasticDataManipulationElwinTab();
  void updateTableFromModel();

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
  void setFileExtensionsByName(bool filter) override;

  /// Retrieve the selected spectrum
  int getSelectedSpectrum() const;
  /// Sets the selected spectrum
  virtual void setSelectedSpectrum(int spectrum);

  /// Retrieve input workspace
  MatrixWorkspace_sptr getInputWorkspace() const;
  /// Set input workspace
  void setInputWorkspace(MatrixWorkspace_sptr inputWorkspace);

  void checkForELTWorkspace();

  std::vector<std::string> getOutputWorkspaceNames();
  QString getOutputBasename();

  virtual std::unique_ptr<IAddWorkspaceDialog> getAddWorkspaceDialog(QWidget *parent) const;
  MatrixWorkspace_sptr getPreviewPlotWorkspace();
  void setPreviewPlotWorkspace(const MatrixWorkspace_sptr &previewPlotWorkspace);

  std::unique_ptr<InelasticDataManipulationElwinTabView> m_view;
  std::unique_ptr<InelasticDataManipulationElwinTabModel> m_model;
  InelasticDataManipulation *m_parent;
  QTableWidget *m_dataTable;
  std::unique_ptr<IndirectFitDataModel> m_dataModel;
  std::unique_ptr<IAddWorkspaceDialog> m_addWorkspaceDialog;
  std::weak_ptr<MatrixWorkspace> m_previewPlotWorkspace;
  int m_selectedSpectrum;
  MatrixWorkspace_sptr m_inputWorkspace;
  bool m_emitCellChanged = true;

  virtual int workspaceIndexColumn() const;

  void setHorizontalHeaders(const QStringList &headers);

  void newPreviewFileSelected(const QString &workspaceName, const QString &filename);
  void newPreviewWorkspaceSelected(const QString &workspaceName);
  size_t findWorkspaceID();
  void newInputFiles();
  void updateIntegrationRange();

private slots:
  void handleValueChanged(QtProperty *, double);
  void handleValueChanged(QtProperty *, bool);
  void checkNewPreviewSelected(int index);
  void handlePreviewSpectrumChanged(int spectrum);
  void unGroupInput(bool error);
  void runClicked();
  void saveClicked();
  void addData();
  void checkLoadedFiles();
  void plotCurrentPreview();
};

} // namespace CustomInterfaces
} // namespace MantidQt
