// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DataProcessor.h"
#include "DataProcessorInterface.h"
#include "MantidQtWidgets/Spectroscopy/DataModel.h"
#include "MantidQtWidgets/Spectroscopy/IDataModel.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h"

#include "ElwinModel.h"
#include "ElwinView.h"
#include "IElwinView.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/FunctionModelSpectra.h"
#include "MantidQtWidgets/Common/IAddWorkspaceDialog.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "ui_ElwinTab.h"

namespace MantidQt {
namespace CustomInterfaces {

using namespace Mantid::API;
using namespace MantidWidgets;
using namespace Inelastic;

class IElwinPresenter {
public:
  virtual void handleValueChanged(std::string const &propName, double value) = 0;
  virtual void handleValueChanged(std::string const &propName, bool value) = 0;
  virtual void handleSaveClicked() = 0;
  virtual void handlePlotPreviewClicked() = 0;
  virtual void handlePreviewSpectrumChanged(int spectrum) = 0;
  virtual void handlePreviewIndexChanged(int index) = 0;
  virtual void handleAddData(MantidWidgets::IAddWorkspaceDialog const *dialog) = 0;
  virtual void handleRemoveSelectedData() = 0;
  virtual void handleRowModeChanged() = 0;
  virtual void updateAvailableSpectra() = 0;
  virtual MatrixWorkspace_sptr getInputWorkspace() const = 0;
};

class MANTIDQT_INELASTIC_DLL ElwinPresenter : public DataProcessor, public IElwinPresenter, public IRunSubscriber {
public:
  ElwinPresenter(QWidget *parent, std::unique_ptr<MantidQt::API::IAlgorithmRunner> algorithmRunner, IElwinView *view,
                 std::unique_ptr<IElwinModel> model);
  ElwinPresenter(QWidget *parent, std::unique_ptr<MantidQt::API::IAlgorithmRunner> algorithmRunner, IElwinView *view,
                 std::unique_ptr<IElwinModel> model, std::unique_ptr<IDataModel> dataModel);
  ~ElwinPresenter();

  // runWidget
  void handleRun() override;
  void handleValidation(IUserInputValidator *validator) const override;
  const std::string getSubscriberName() const override { return "Elwin"; }

  // Elwin interface methods
  void handleValueChanged(std::string const &propName, double) override;
  void handleValueChanged(std::string const &propName, bool) override;
  void handleSaveClicked() override;
  void handlePlotPreviewClicked() override;
  void handlePreviewSpectrumChanged(int spectrum) override;
  void handlePreviewIndexChanged(int index) override;
  void handleAddData(MantidWidgets::IAddWorkspaceDialog const *dialog) override;
  void handleRemoveSelectedData() override;
  void handleRowModeChanged() override;
  void updateAvailableSpectra() override;

  void setInputWorkspace(const MatrixWorkspace_sptr &inputWorkspace);
  virtual void setSelectedSpectrum(int spectrum);
  int getSelectedSpectrum() const;
  MatrixWorkspace_sptr getInputWorkspace() const override;
  MatrixWorkspace_sptr getPreviewPlotWorkspace();
  void setPreviewPlotWorkspace(const MatrixWorkspace_sptr &previewPlotWorkspace);

protected:
  void runComplete(bool error) override;
  void newInputDataFromDialog();
  virtual void addDataToModel(MantidWidgets::IAddWorkspaceDialog const *dialog);

private:
  void updateTableFromModel();
  void updateIntegrationRange();

  std::vector<std::string> getOutputWorkspaceNames();
  std::string getOutputBasename();
  bool checkForELTWorkspace();
  void newPreviewWorkspaceSelected(int index);
  WorkspaceID findWorkspaceID();

  IElwinView *m_view;
  std::unique_ptr<IElwinModel> m_model;
  std::unique_ptr<IDataModel> m_dataModel;
  int m_selectedSpectrum;
  std::weak_ptr<MatrixWorkspace> m_previewPlotWorkspace;
  MatrixWorkspace_sptr m_inputWorkspace;
};
} // namespace CustomInterfaces
} // namespace MantidQt
