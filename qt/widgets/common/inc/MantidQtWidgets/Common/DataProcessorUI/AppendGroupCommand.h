#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORAPPENDGROUPCOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORAPPENDGROUPCOMMAND_H

#include "MantidQtWidgets/Common/DataProcessorUI/CommandBase.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorPresenter.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** @class AppendGroupCommand

AppendGroupCommand defines the action "Insert Group"

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
class AppendGroupCommand : public CommandBase {
public:
  AppendGroupCommand(DataProcessorPresenter *tablePresenter)
      : CommandBase(tablePresenter){};
  AppendGroupCommand(const QDataProcessorWidget &widget)
      : CommandBase(widget){};
  virtual ~AppendGroupCommand(){};

  void execute() override {
    m_presenter->notify(DataProcessorPresenter::AppendGroupFlag);
  };
  QString name() override { return QString("Insert Group After"); }
  QString icon() override { return QString("://insert_group.png"); }
  QString tooltip() override { return QString("Inserts group after"); }
  QString whatsthis() override {
    return QString("Inserts a new group after the first selected group. If "
                   "no groups are selected then a new group is added at "
                   "the end of the table");
  }
  QString shortcut() override { return QString(); }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORAPPENDGROUPCOMMAND_H*/
