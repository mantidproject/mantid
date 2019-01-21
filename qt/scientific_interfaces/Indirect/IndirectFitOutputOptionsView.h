// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTFITOUTPUTOPTIONSVIEW_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTFITOUTPUTOPTIONSVIEW_H_

#include "ui_IndirectFitOutputOptions.h"

#include "DllConfig.h"
#include "MantidQtWidgets/Common/MantidWidget.h"

#include <QMessageBox>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectFitOutputOptionsView
    : public API::MantidWidget {
  Q_OBJECT

public:
  IndirectFitOutputOptionsView(QWidget *parent = nullptr);
  ~IndirectFitOutputOptionsView() override;

  void setGroupWorkspaceComboBoxVisible(bool visible);
  void setWorkspaceComboBoxVisible(bool visible);

  void clearPlotWorkspaces();
  void clearPlotTypes();
  void
  setAvailablePlotWorkspaces(std::vector<std::string> const &workspaceNames);
  void setAvailablePlotTypes(std::vector<std::string> const &parameterNames);

  void setPlotGroupWorkspaceIndex(int index);
  void setPlotWorkspacesIndex(int index);
  void setPlotTypeIndex(int index);

  std::string getSelectedGroupWorkspace() const;
  std::string getSelectedWorkspace() const;
  std::string getSelectedPlotType() const;

  void setPlotText(QString const &text);
  void setSaveText(QString const &text);

	void setPlotExtraOptionsEnabled(bool enable);
  void setPlotEnabled(bool enable);
  void setSaveEnabled(bool enable);

  void displayWarning(std::string const &message);

signals:
  void groupWorkspaceChanged(std::string const &group);
  void plotClicked();
  void saveClicked();

private slots:
  void emitGroupWorkspaceChanged(QString const &group);
  void emitPlotClicked();
  void emitSaveClicked();

private:
  std::unique_ptr<Ui::IndirectFitOutputOptions> m_outputOptions;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
#endif
