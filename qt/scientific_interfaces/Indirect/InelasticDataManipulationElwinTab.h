// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
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

public slots:
  void removeSelectedData();
  void updateAvailableSpectra();

protected:
  void addData(IAddWorkspaceDialog const *dialog);
  void checkData(IAddWorkspaceDialog const *dialog);
  void addDataFromFile(IAddWorkspaceDialog const *dialog);
  void newInputFilesFromDialog(IAddWorkspaceDialog const *dialog);
  virtual void addDataToModel(IAddWorkspaceDialog const *dialog);

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
  void updateTableFromModel();

  int getSelectedSpectrum() const;
  virtual void setSelectedSpectrum(int spectrum);
  MatrixWorkspace_sptr getInputWorkspace() const;
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
  std::unique_ptr<IndirectFitDataModel> m_dataModel;
  std::unique_ptr<IAddWorkspaceDialog> m_addWorkspaceDialog;
  std::weak_ptr<MatrixWorkspace> m_previewPlotWorkspace;
  int m_selectedSpectrum;
  MatrixWorkspace_sptr m_inputWorkspace;

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
