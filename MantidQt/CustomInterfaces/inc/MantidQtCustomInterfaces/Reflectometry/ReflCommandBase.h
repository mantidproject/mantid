#ifndef MANTID_CUSTOMINTERFACES_REFLCOMMANDBASE_H
#define MANTID_CUSTOMINTERFACES_REFLCOMMANDBASE_H

#include "MantidQtCustomInterfaces/Reflectometry/ReflCommand.h"

namespace MantidQt {
namespace CustomInterfaces {
class IReflTablePresenter;
/** @class ReflCommandBase

ReflCommandBase is an interface which defines the functions any data processor
action needs to support. Defines a IReflTablePresenter that will be notified.

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
class ReflCommandBase : public ReflCommand {
public:
  ReflCommandBase(IReflTablePresenter *tablePresenter)
      : m_tablePresenter(tablePresenter) {
    if (!tablePresenter) {
      throw std::invalid_argument("Invalid abstract presenter");
    }
  };
  virtual ~ReflCommandBase(){};

  virtual void execute() = 0;
  virtual std::string name() = 0;
  virtual std::string icon() = 0;

protected:
  IReflTablePresenter *const m_tablePresenter;
};
}
}
#endif /*MANTID_CUSTOMINTERFACES_REFLCOMMANDBASE_H*/