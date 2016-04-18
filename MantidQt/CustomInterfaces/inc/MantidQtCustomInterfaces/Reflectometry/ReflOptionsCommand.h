#ifndef MANTID_CUSTOMINTERFACES_REFLOPTIONSTABLECOMMAND_H
#define MANTID_CUSTOMINTERFACES_REFLOPTIONSTABLECOMMAND_H

#include "MantidQtCustomInterfaces/Reflectometry/IReflTablePresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflCommandBase.h"

namespace MantidQt {
namespace CustomInterfaces {
/** @class ReflOptionsCommand

ReflOptionsCommand defines the action "Import .TBL"

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
class ReflOptionsCommand : public ReflCommandBase {
public:
  ReflOptionsCommand(IReflTablePresenter *tablePresenter)
      : ReflCommandBase(tablePresenter){};
  virtual ~ReflOptionsCommand(){};

  void execute() override {
    m_tablePresenter->notify(IReflTablePresenter::OptionsDialogFlag);
  };
  std::string name() override { return std::string("Options"); }
  std::string icon() override { return std::string("://configure.png"); }
};
}
}
#endif /*MANTID_CUSTOMINTERFACES_REFLOPTIONSTABLECOMMAND_H*/