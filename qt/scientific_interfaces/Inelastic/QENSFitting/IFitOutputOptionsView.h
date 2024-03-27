// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"

#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

class IFitOutputOptionsPresenter;

class MANTIDQT_INELASTIC_DLL IFitOutputOptionsView {

public:
  virtual ~IFitOutputOptionsView() = default;

  virtual void subscribePresenter(IFitOutputOptionsPresenter *presenter) = 0;

  virtual void setGroupWorkspaceComboBoxVisible(bool visible) = 0;
  virtual void setWorkspaceComboBoxVisible(bool visible) = 0;

  virtual void clearPlotWorkspaces() = 0;
  virtual void clearPlotTypes() = 0;
  virtual void setAvailablePlotWorkspaces(std::vector<std::string> const &workspaceNames) = 0;
  virtual void setAvailablePlotTypes(std::vector<std::string> const &parameterNames) = 0;

  virtual void setPlotGroupWorkspaceIndex(int index) = 0;
  virtual void setPlotWorkspacesIndex(int index) = 0;
  virtual void setPlotTypeIndex(int index) = 0;

  virtual std::string getSelectedGroupWorkspace() const = 0;
  virtual std::string getSelectedWorkspace() const = 0;
  virtual std::string getSelectedPlotType() const = 0;

  virtual void setPlotText(std::string const &text) = 0;
  virtual void setSaveText(std::string const &text) = 0;

  virtual void setPlotExtraOptionsEnabled(bool enable) = 0;
  virtual void setPlotEnabled(bool enable) = 0;
  virtual void setEditResultEnabled(bool enable) = 0;
  virtual void setSaveEnabled(bool enable) = 0;

  virtual void setEditResultVisible(bool visible) = 0;

  virtual void displayWarning(std::string const &message) = 0;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
