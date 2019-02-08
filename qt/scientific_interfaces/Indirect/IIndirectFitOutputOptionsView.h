// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_IINDIRECTFITOUTPUTOPTIONSVIEW_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IINDIRECTFITOUTPUTOPTIONSVIEW_H_

#include "DllConfig.h"
#include "MantidQtWidgets/Common/MantidWidget.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IIndirectFitOutputOptionsView
    : public API::MantidWidget {
  Q_OBJECT

public:
  IIndirectFitOutputOptionsView(QWidget *parent = nullptr)
      : API::MantidWidget(parent){};
  virtual ~IIndirectFitOutputOptionsView(){};

  virtual void setGroupWorkspaceComboBoxVisible(bool visible) = 0;
  virtual void setWorkspaceComboBoxVisible(bool visible) = 0;

  virtual void clearPlotWorkspaces() = 0;
  virtual void clearPlotTypes() = 0;
  virtual void setAvailablePlotWorkspaces(
      std::vector<std::string> const &workspaceNames) = 0;
  virtual void
  setAvailablePlotTypes(std::vector<std::string> const &parameterNames) = 0;

  virtual void setPlotGroupWorkspaceIndex(int index) = 0;
  virtual void setPlotWorkspacesIndex(int index) = 0;
  virtual void setPlotTypeIndex(int index) = 0;

  virtual std::string getSelectedGroupWorkspace() const = 0;
  virtual std::string getSelectedWorkspace() const = 0;
  virtual std::string getSelectedPlotType() const = 0;

  virtual void setPlotText(QString const &text) = 0;
  virtual void setSaveText(QString const &text) = 0;

  virtual void setPlotExtraOptionsEnabled(bool enable) = 0;
  virtual void setPlotEnabled(bool enable) = 0;
  virtual void setEditResultEnabled(bool enable) = 0;
  virtual void setSaveEnabled(bool enable) = 0;

  virtual void setEditResultVisible(bool visible) = 0;

  virtual void displayWarning(std::string const &message) = 0;

signals:
  void groupWorkspaceChanged(std::string const &group);
  void plotClicked();
  void saveClicked();
  void editResultClicked();
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
