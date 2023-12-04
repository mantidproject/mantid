// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectFitOutputOptionsModel.h"
#include "IndirectFitOutputOptionsView.h"

#include "DllConfig.h"
#include "MantidAPI/WorkspaceGroup.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class IIndirectDataAnalysisTab;

class MANTIDQT_INELASTIC_DLL IIndirectFitOutputOptionsPresenter {
public:
  virtual void handleGroupWorkspaceChanged(std::string const &selectedGroup) = 0;
  virtual void handlePlotClicked() = 0;
  virtual void handleSaveClicked() = 0;
  virtual void handleReplaceSingleFitResult(std::string const &inputName, std::string const &singleBinName,
                                            std::string const &outputName) = 0;
};

class MANTIDQT_INELASTIC_DLL IndirectFitOutputOptionsPresenter final : public IIndirectFitOutputOptionsPresenter {
public:
  IndirectFitOutputOptionsPresenter(IIndirectDataAnalysisTab *tab, IIndirectFitOutputOptionsView *view);
  IndirectFitOutputOptionsPresenter(IIndirectFitOutputOptionsModel *model, IIndirectFitOutputOptionsView *view);

  void setMultiWorkspaceOptionsVisible(bool visible);

  void setResultWorkspace(Mantid::API ::WorkspaceGroup_sptr groupWorkspace);
  void setPDFWorkspace(Mantid::API ::WorkspaceGroup_sptr groupWorkspace);
  void setPlotWorkspaces();
  void setPlotTypes(std::string const &selectedGroup);

  void removePDFWorkspace();

  bool isSelectedGroupPlottable() const;

  void setPlotting(bool plotting);
  void setPlotEnabled(bool enable);
  void setEditResultEnabled(bool enable);
  void setSaveEnabled(bool enable);

  void clearSpectraToPlot();
  std::vector<SpectrumToPlot> getSpectraToPlot() const;

  void setEditResultVisible(bool visible);

  void handleGroupWorkspaceChanged(std::string const &selectedGroup) override;
  void handlePlotClicked() override;
  void handleSaveClicked() override;
  void handleReplaceSingleFitResult(std::string const &inputName, std::string const &singleBinName,
                                    std::string const &outputName) override;

private:
  void plotResult(std::string const &selectedGroup);
  void setSaving(bool saving);

  void setEditingResult(bool editing);

  void replaceSingleFitResult(std::string const &inputName, std::string const &singleBinName,
                              std::string const &outputName);

  void displayWarning(std::string const &message);

  IIndirectDataAnalysisTab *m_tab;
  IIndirectFitOutputOptionsView *m_view;
  std::unique_ptr<IIndirectFitOutputOptionsModel> m_model;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
