#ifndef MANTID_CUSTOMINTERFACES_REFLCOMMAND_H
#define MANTID_CUSTOMINTERFACES_REFLCOMMAND_H

#include <memory>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
/** @class ReflCommand

ReflCommand is an interface which defines the functions any data processor
action needs to support.

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
class ReflCommand {
public:
  ReflCommand() : m_child(){};
  virtual ~ReflCommand(){};

  virtual void execute() = 0;
  virtual std::string name() = 0;
  virtual std::string icon() = 0;
  virtual bool hasChild() final { return !m_child.empty(); };
  virtual void setChild(std::vector<std::unique_ptr<ReflCommand>> child) final {
    m_child = std::move(child);
  }
  virtual std::vector<std::unique_ptr<ReflCommand>> &getChild() final {
    return m_child;
  }
	virtual bool isSeparator() final { return name().empty() && icon().empty(); }

protected:
  std::vector<std::unique_ptr<ReflCommand>> m_child;
};
}
}
#endif /*MANTID_CUSTOMINTERFACES_REFLCOMMAND_H*/