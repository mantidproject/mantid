// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLSAVETABVIEW_H
#define MANTID_ISISREFLECTOMETRY_IREFLSAVETABVIEW_H

#include "DllConfig.h"
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class IReflSaveTabPresenter;

/** @class IReflSaveTabView

IReflSaveTabView is the base view class for the tab "Save ASCII" in the
Reflectometry Interface. It contains no QT specific functionality as that should
be handled by a subclass.
*/

class MANTIDQT_ISISREFLECTOMETRY_DLL IReflSaveTabView {
public:
  /// Constructor
  IReflSaveTabView(){};
  /// Destructor
  virtual ~IReflSaveTabView(){};
  virtual void subscribe(IReflSaveTabPresenter *presenter) = 0;

  virtual std::string getSavePath() const = 0;
  virtual void setSavePath(const std::string &path) const = 0;
  virtual std::string getPrefix() const = 0;
  virtual std::string getFilter() const = 0;
  virtual bool getRegexCheck() const = 0;
  virtual std::string getCurrentWorkspaceName() const = 0;
  virtual std::vector<std::string> getSelectedWorkspaces() const = 0;
  virtual std::vector<std::string> getSelectedParameters() const = 0;
  virtual int getFileFormatIndex() const = 0;
  virtual bool getTitleCheck() const = 0;
  virtual bool getQResolutionCheck() const = 0;
  virtual std::string getSeparator() const = 0;

  virtual void clearWorkspaceList() const = 0;
  virtual void clearParametersList() const = 0;
  virtual void setWorkspaceList(const std::vector<std::string> &) const = 0;
  virtual void setParametersList(const std::vector<std::string> &) const = 0;
  virtual void disallowAutosave() = 0;

  virtual void disableAutosaveControls() = 0;
  virtual void enableAutosaveControls() = 0;

  virtual void enableFileFormatAndLocationControls() = 0;
  virtual void disableFileFormatAndLocationControls() = 0;
  virtual void giveUserCritical(const std::string &prompt,
                                const std::string &title) = 0;
  virtual void giveUserInfo(const std::string &prompt,
                            const std::string &title) = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLSAVETABVIEW_H */
