#ifndef MANTID_ISISREFLECTOMETRY_IREFLSAVETABVIEW_H
#define MANTID_ISISREFLECTOMETRY_IREFLSAVETABVIEW_H

#include "DllConfig.h"
#include <string>
#include <vector>
#include "IReflSaveTabPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {


/** @class IReflSaveTabView

IReflSaveTabView is the base view class for the tab "Save ASCII" in the
Reflectometry Interface. It contains no QT specific functionality as that should
be handled by a subclass.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class MANTIDQT_ISISREFLECTOMETRY_DLL IReflSaveTabView {
public:
  virtual ~IReflSaveTabView() = default;
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

  virtual void invalidRegex() = 0;
  virtual void errorInvalidSaveDirectory() = 0;
  virtual void warnInvalidSaveDirectory() = 0;
  virtual void noWorkspacesSelected() = 0;
  virtual void cannotSaveWorkspaces() = 0;
  virtual void cannotSaveWorkspaces(std::string const& fullError) = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLSAVETABVIEW_H */
