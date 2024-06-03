// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL SaveViewSubscriber {
public:
  virtual void notifySettingsChanged() = 0;
  virtual void notifyPopulateWorkspaceList() = 0;
  virtual void notifyFilterWorkspaceList() = 0;
  virtual void notifyPopulateParametersList() = 0;
  virtual void notifySaveSelectedWorkspaces() = 0;
  virtual void notifyAutosaveDisabled() = 0;
  virtual void notifyAutosaveEnabled() = 0;
  virtual void notifySaveIndividualRowsEnabled() = 0;
  virtual void notifySaveIndividualRowsDisabled() = 0;
  virtual void notifySavePathChanged() = 0;
};

/** @class ISaveView

ISaveView is the base view class for the tab "Save ASCII" in the
Reflectometry Interface. It contains no QT specific functionality as that should
be handled by a subclass.
*/

class MANTIDQT_ISISREFLECTOMETRY_DLL ISaveView {
public:
  virtual ~ISaveView() = default;
  virtual void subscribe(SaveViewSubscriber *notifyee) = 0;

  virtual void connectSaveSettingsWidgets() = 0;
  virtual std::string getSavePath() const = 0;
  virtual void setSavePath(const std::string &path) const = 0;
  virtual std::string getPrefix() const = 0;
  virtual std::string getFilter() const = 0;
  virtual bool getRegexCheck() const = 0;
  virtual std::string getCurrentWorkspaceName() const = 0;
  virtual std::vector<std::string> getSelectedWorkspaces() const = 0;
  virtual std::vector<std::string> getSelectedParameters() const = 0;
  virtual int getFileFormatIndex() const = 0;
  virtual bool getHeaderCheck() const = 0;
  virtual bool getQResolutionCheck() const = 0;
  virtual bool getAdditionalColumnsCheck() const = 0;
  virtual std::string getSeparator() const = 0;
  virtual bool getSaveToSingleFileCheck() const = 0;

  virtual void clearWorkspaceList() const = 0;
  virtual void clearParametersList() const = 0;
  virtual void setWorkspaceList(const std::vector<std::string> &) const = 0;
  virtual void setParametersList(const std::vector<std::string> &) const = 0;
  virtual void disallowAutosave() = 0;

  virtual void disableAutosaveControls() = 0;
  virtual void enableAutosaveControls() = 0;

  virtual void enableFileFormatControls() = 0;
  virtual void disableFileFormatControls() = 0;
  virtual void enableLocationControls() = 0;
  virtual void disableLocationControls() = 0;

  virtual void enableLogList() = 0;
  virtual void disableLogList() = 0;
  virtual void enableHeaderCheckBox() = 0;
  virtual void disableHeaderCheckBox() = 0;
  virtual void enableQResolutionCheckBox() = 0;
  virtual void disableQResolutionCheckBox() = 0;
  virtual void enableAdditionalColumnsCheckBox() = 0;
  virtual void disableAdditionalColumnsCheckBox() = 0;
  virtual void enableSeparatorButtonGroup() = 0;
  virtual void disableSeparatorButtonGroup() = 0;
  virtual void enableSaveToSingleFileCheckBox() = 0;
  virtual void disableSaveToSingleFileCheckBox() = 0;
  virtual void enableSaveIndividualRowsCheckbox() = 0;
  virtual void disableSaveIndividualRowsCheckbox() = 0;

  virtual void showFilterEditValid() = 0;
  virtual void showFilterEditInvalid() = 0;
  virtual void errorInvalidSaveDirectory() = 0;
  virtual void warnInvalidSaveDirectory() = 0;
  virtual void noWorkspacesSelected() = 0;
  virtual void cannotSaveWorkspaces() = 0;
  virtual void cannotSaveWorkspaces(std::string const &fullError) = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
