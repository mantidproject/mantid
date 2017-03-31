#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOLLAPSEGROUPSCOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOLLAPSEGROUPSCOMMAND_H

#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCommandBase.h"

namespace MantidQt {
namespace MantidWidgets {
/** @class DataProcessorCollapseGroupsCommand

DataProcessorCollapseGroupsCommand defines the action "Collapse All Groups"

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
class DataProcessorCollapseGroupsCommand : public DataProcessorCommandBase {
public:
  DataProcessorCollapseGroupsCommand(DataProcessorPresenter *tablePresenter)
      : DataProcessorCommandBase(tablePresenter){};
  virtual ~DataProcessorCollapseGroupsCommand(){};

  void execute() override {
    m_presenter->notify(DataProcessorPresenter::CollapseAllGroupsFlag);
  };
  std::string name() override { return std::string("Collapse All Groups"); }
  std::string icon() override { return std::string("://collapse_all.png"); }
  std::string tooltip() override { return std::string("Collapse all groups"); }
  std::string whatsthis() override {
    return std::string(
        "If any groups in the table are currently expanded this will collapse "
        "all expanded groups, hiding their individual runs.");
  }
  std::string shortcut() override { return std::string(); }
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOLLAPSEGROUPSCOMMAND_H*/
