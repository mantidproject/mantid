#ifndef MANTID_CUSTOMINTERFACES_REFLWORKSPACECOMMAND_H
#define MANTID_CUSTOMINTERFACES_REFLWORKSPACECOMMAND_H

#include "MantidQtCustomInterfaces/Reflectometry/IReflTablePresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflCommandBase.h"

namespace MantidQt {
namespace CustomInterfaces {
/** @class ReflWorkspaceCommand

ReflWorkspaceCommand defines a workspace action

Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class ReflWorkspaceCommand : public ReflCommandBase {
public:
  ReflWorkspaceCommand(IReflTablePresenter *tablePresenter,
                       const std::string &name)
      : ReflCommandBase(tablePresenter), m_name(name){};
  virtual ~ReflWorkspaceCommand(){};

  void execute() override {
    // Tell the presenter which of the available workspaces was selected
    m_tablePresenter->setModel(m_name);
    // Now notify the presenter
    m_tablePresenter->notify(IReflTablePresenter::OpenTableFlag);
  };
  std::string name() override { return m_name; }
  std::string icon() override { return std::string("://worksheet.png"); }

private:
  std::string m_name;
};
}
}
#endif /*MANTID_CUSTOMINTERFACES_REFLWORKSPACECOMMAND_H*/