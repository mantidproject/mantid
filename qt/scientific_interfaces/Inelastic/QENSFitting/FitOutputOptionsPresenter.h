// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "FitOutputOptionsModel.h"
#include "FitOutputOptionsView.h"

#include "DllConfig.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Plotting/ExternalPlotter.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

class IFitTab;

class MANTIDQT_INELASTIC_DLL IFitOutputOptionsPresenter {
public:
  virtual void handleGroupWorkspaceChanged(std::string const &selectedGroup) = 0;
  virtual void handlePlotClicked() = 0;
  virtual void handleSaveClicked() = 0;
  virtual void handleReplaceSingleFitResult(std::string const &inputName, std::string const &singleBinName,
                                            std::string const &outputName) = 0;
};

class MANTIDQT_INELASTIC_DLL FitOutputOptionsPresenter final : public IFitOutputOptionsPresenter {
public:
  FitOutputOptionsPresenter(IFitOutputOptionsView *view, std::unique_ptr<IFitOutputOptionsModel> model,
                            std::unique_ptr<Widgets::MplCpp::IExternalPlotter> plotter);

  void enableOutputOptions(bool const enable, Mantid::API::WorkspaceGroup_sptr resultWorkspace,
                           std::optional<std::string> const &basename, std::string const &minimizer);

  void setEditResultVisible(bool visible);

  void handleGroupWorkspaceChanged(std::string const &selectedGroup) override;
  void handlePlotClicked() override;
  void handleSaveClicked() override;
  void handleReplaceSingleFitResult(std::string const &inputName, std::string const &singleBinName,
                                    std::string const &outputName) override;

  // Public for testing purposes
  void setPlotting(bool plotting);
  void setPlotWorkspaces();
  void setPlotTypes(std::string const &selectedGroup);
  void setPlotEnabled(bool enable);

private:
  bool isSelectedGroupPlottable() const;

  void setMultiWorkspaceOptionsVisible(bool visible);
  void setPDFWorkspace(std::string const &workspaceName, std::string const &minimizer);

  void setEditResultEnabled(bool enable);
  void setSaveEnabled(bool enable);

  std::vector<SpectrumToPlot> getSpectraToPlot(std::string const &selectedGroup) const;
  void setSaving(bool saving);

  void setEditingResult(bool editing);

  void replaceSingleFitResult(std::string const &inputName, std::string const &singleBinName,
                              std::string const &outputName);

  void displayWarning(std::string const &message);

  IFitOutputOptionsView *m_view;
  std::unique_ptr<IFitOutputOptionsModel> m_model;
  std::unique_ptr<Widgets::MplCpp::IExternalPlotter> m_plotter;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
