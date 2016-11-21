#ifndef MANTID_CUSTOMINTERFACES_IREFLSAVETABVIEW_H
#define MANTID_CUSTOMINTERFACES_IREFLSAVETABVIEW_H

#include "MantidQtCustomInterfaces/DllConfig.h"
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class IReflSaveTabPresenter;

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

class DLLExport IReflSaveTabView {
public:
  /// Constructor
  IReflSaveTabView(){};
  /// Destructor
  virtual ~IReflSaveTabView(){};
  /// Returns the presenter managing this view
  virtual IReflSaveTabPresenter *getPresenter() const = 0;

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
};
}
}
#endif /* MANTID_CUSTOMINTERFACES_IREFLSAVETABVIEW_H */
