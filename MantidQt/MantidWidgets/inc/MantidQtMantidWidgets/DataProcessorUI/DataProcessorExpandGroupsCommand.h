#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSOREXPANDGROUPSCOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSOREXPANDGROUPSCOMMAND_H

#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCommandBase.h"

namespace MantidQt {
namespace MantidWidgets {
/** @class DataProcessorExpandGroupsCommand

DataProcessorExpandGroupsCommand defines the action "Expand All Groups"

Copyright &copy; 2011-16 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DataProcessorExpandGroupsCommand : public DataProcessorCommandBase {
public:
  DataProcessorExpandGroupsCommand(DataProcessorPresenter *tablePresenter)
      : DataProcessorCommandBase(tablePresenter){};
  virtual ~DataProcessorExpandGroupsCommand(){};

  void execute() override {
    m_presenter->notify(DataProcessorPresenter::ExpandAllGroupsFlag);
  };
  std::string name() override { return std::string("Expand All Groups"); }
  std::string icon() override { return std::string("://expand_all.png"); }
  std::string tooltip() override { return std::string("Expands all groups"); }
  std::string whatsthis() override {
    return std::string(
        "If any groups in the table are currently collapsed this will expand "
        "all collapsed groups, revealing their individual runs.");
  }
  std::string shortcut() override { return std::string(); }
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSOREXPANDGROUPSCOMMAND_H*/
