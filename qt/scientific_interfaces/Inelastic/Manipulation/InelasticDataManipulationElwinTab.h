// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Analysis/IndirectFitDataModel.h"
#include "IAddWorkspaceDialog.h"
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

class IElwinPresenter {
public:
  virtual void handleValueChanged(std::string const &propName, double value) = 0;
  virtual void handleValueChanged(std::string const &propName, bool value) = 0;
  virtual void handleRunClicked() = 0;
  virtual void handleSaveClicked() = 0;
  virtual void handlePlotPreviewClicked() = 0;
  virtual void handleFilesFound() = 0;
  virtual void handlePreviewSpectrumChanged(int spectrum) = 0;
  virtual void handlePreviewIndexChanged(int index) = 0;
  virtual void handleAddData(IAddWorkspaceDialog const *dialog) = 0;
  virtual void handleAddDataFromFile(IAddWorkspaceDialog const *dialog) = 0;
  virtual void handleRemoveSelectedData() = 0;
  virtual void updateAvailableSpectra() = 0;
};

class MANTIDQT_INELASTIC_DLL InelasticDataManipulationElwinTab : public InelasticDataManipulationTab,
                                                                 public IElwinPresenter {
public:
  InelasticDataManipulationElwinTab(QWidget *parent, IElwinView *view);
  ~InelasticDataManipulationElwinTab();

  // base Manipulation tab methods
  void run() override;
  void setup() override;
  bool validate() override;

  // Elwin interface methods
  void handleValueChanged(std::string const &propName, double) override;
  void handleValueChanged(std::string const &propName, bool) override;
  void handleRunClicked() override;
  void handleSaveClicked() override;
  void handlePlotPreviewClicked() override;
  void handleFilesFound() override;
  void handlePreviewSpectrumChanged(int spectrum) override;
  void handlePreviewIndexChanged(int index) override;
  void handleAddData(IAddWorkspaceDialog const *dialog) override;
  void handleAddDataFromFile(IAddWorkspaceDialog const *dialog) override;
  void handleRemoveSelectedData() override;
  void updateAvailableSpectra() override;

protected:
  void runComplete(bool error) override;
  void newInputFilesFromDialog(IAddWorkspaceDialog const *dialog);
  virtual void addDataToModel(IAddWorkspaceDialog const *dialog);

private:
  void runFileInput();
  void runWorkspaceInput();

  void setFileExtensionsByName(bool filter) override;
  void updateTableFromModel();
  void newInputFiles();
  void updateIntegrationRange();

  int getSelectedSpectrum() const;
  virtual void setSelectedSpectrum(int spectrum);

  std::vector<std::string> getOutputWorkspaceNames();
  std::string getOutputBasename();
  MatrixWorkspace_sptr getInputWorkspace() const;
  MatrixWorkspace_sptr getPreviewPlotWorkspace();
  void checkForELTWorkspace();
  void setInputWorkspace(MatrixWorkspace_sptr inputWorkspace);
  void setPreviewPlotWorkspace(const MatrixWorkspace_sptr &previewPlotWorkspace);
  void newPreviewFileSelected(const std::string &workspaceName, const std::string &filename);
  void newPreviewWorkspaceSelected(const std::string &workspaceName);
  size_t findWorkspaceID();

  IElwinView *m_view;
  std::unique_ptr<InelasticDataManipulationElwinTabModel> m_model;
  std::unique_ptr<IndirectFitDataModel> m_dataModel;
  int m_selectedSpectrum;
  std::weak_ptr<MatrixWorkspace> m_previewPlotWorkspace;
  MatrixWorkspace_sptr m_inputWorkspace;
};
} // namespace CustomInterfaces
} // namespace MantidQt
