// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ui_FitOutputOptions.h"

#include "DllConfig.h"
#include "IFitOutputOptionsView.h"
#include "MantidQtWidgets/Common/MantidWidget.h"

#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

class IFitOutputOptionsPresenter;
class EditResultsDialog;

class MANTIDQT_INELASTIC_DLL FitOutputOptionsView final : public API::MantidWidget, public IFitOutputOptionsView {
  Q_OBJECT

public:
  FitOutputOptionsView(QWidget *parent = nullptr);

  void subscribePresenter(IFitOutputOptionsPresenter *presenter) override;

  void setGroupWorkspaceComboBoxVisible(bool visible) override;
  void setWorkspaceComboBoxVisible(bool visible) override;

  void clearPlotWorkspaces() override;
  void clearPlotTypes() override;
  void setAvailablePlotWorkspaces(std::vector<std::string> const &workspaceNames) override;
  void setAvailablePlotTypes(std::vector<std::string> const &parameterNames) override;

  void setPlotGroupWorkspaceIndex(int index) override;
  void setPlotWorkspacesIndex(int index) override;
  void setPlotTypeIndex(int index) override;

  std::string getSelectedGroupWorkspace() const override;
  std::string getSelectedWorkspace() const override;
  std::string getSelectedPlotType() const override;

  void setPlotText(std::string const &text) override;
  void setSaveText(std::string const &text) override;

  void setPlotExtraOptionsEnabled(bool enable) override;
  void setPlotEnabled(bool enable) override;
  void setEditResultEnabled(bool enable) override;
  void setSaveEnabled(bool enable) override;

  void setEditResultVisible(bool visible) override;

  void displayWarning(std::string const &message) override;

private slots:
  void notifyGroupWorkspaceChanged(QString const &group);
  void notifyPlotClicked();
  void notifySaveClicked();
  void notifyReplaceSingleFitResult();
  void handleEditResultClicked();

private:
  EditResultsDialog *m_editResultsDialog;
  std::unique_ptr<Ui::FitOutputOptions> m_outputOptions;
  IFitOutputOptionsPresenter *m_presenter;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
