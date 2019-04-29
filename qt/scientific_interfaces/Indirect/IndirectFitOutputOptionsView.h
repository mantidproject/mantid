// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTFITOUTPUTOPTIONSVIEW_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTFITOUTPUTOPTIONSVIEW_H_

#include "ui_IndirectFitOutputOptions.h"

#include "DllConfig.h"
#include "IIndirectFitOutputOptionsView.h"

#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectFitOutputOptionsView
    : public IIndirectFitOutputOptionsView {
  Q_OBJECT

public:
  IndirectFitOutputOptionsView(QWidget *parent = nullptr);
  virtual ~IndirectFitOutputOptionsView() override;

  void setGroupWorkspaceComboBoxVisible(bool visible) override;
  void setWorkspaceComboBoxVisible(bool visible) override;

  void clearPlotWorkspaces() override;
  void clearPlotTypes() override;
  void setAvailablePlotWorkspaces(
      std::vector<std::string> const &workspaceNames) override;
  void setAvailablePlotTypes(
      std::vector<std::string> const &parameterNames) override;

  void setPlotGroupWorkspaceIndex(int index) override;
  void setPlotWorkspacesIndex(int index) override;
  void setPlotTypeIndex(int index) override;

  std::string getSelectedGroupWorkspace() const override;
  std::string getSelectedWorkspace() const override;
  std::string getSelectedPlotType() const override;

  void setPlotText(QString const &text) override;
  void setSaveText(QString const &text) override;

  void setPlotExtraOptionsEnabled(bool enable) override;
  void setPlotEnabled(bool enable) override;
  void setEditResultEnabled(bool enable) override;
  void setSaveEnabled(bool enable) override;

  void setEditResultVisible(bool visible) override;

  void displayWarning(std::string const &message) override;

private slots:
  void emitGroupWorkspaceChanged(QString const &group);
  void emitPlotClicked();
  void emitSaveClicked();
  void emitEditResultClicked();

private:
  std::unique_ptr<Ui::IndirectFitOutputOptions> m_outputOptions;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
#endif
