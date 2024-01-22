// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ui_IndirectFitOutputOptions.h"

#include "DllConfig.h"
#include "IIndirectFitOutputOptionsView.h"
#include "MantidQtWidgets/Common/MantidWidget.h"

#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class IIndirectFitOutputOptionsPresenter;
class IndirectEditResultsDialog;

class MANTIDQT_INELASTIC_DLL IndirectFitOutputOptionsView final : public API::MantidWidget,
                                                                  public IIndirectFitOutputOptionsView {
  Q_OBJECT

public:
  IndirectFitOutputOptionsView(QWidget *parent = nullptr);

  void subscribePresenter(IIndirectFitOutputOptionsPresenter *presenter) override;

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
  IndirectEditResultsDialog *m_editResultsDialog;
  std::unique_ptr<Ui::IndirectFitOutputOptions> m_outputOptions;
  IIndirectFitOutputOptionsPresenter *m_presenter;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt