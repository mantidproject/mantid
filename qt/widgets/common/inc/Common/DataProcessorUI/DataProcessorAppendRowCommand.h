#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORAPPENDROWCOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORAPPENDROWCOMMAND_H

#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCommandBase.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPresenter.h"

namespace MantidQt {
namespace MantidWidgets {
/** @class DataProcessorAppendRowCommand

DataProcessorAppendRowCommand defines the action "Insert Row After"

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
class DataProcessorAppendRowCommand : public DataProcessorCommandBase {
public:
  DataProcessorAppendRowCommand(DataProcessorPresenter *tablePresenter)
      : DataProcessorCommandBase(tablePresenter){};
  DataProcessorAppendRowCommand(const QDataProcessorWidget &widget)
      : DataProcessorCommandBase(widget){};
  virtual ~DataProcessorAppendRowCommand(){};

  void execute() override {
    m_presenter->notify(DataProcessorPresenter::AppendRowFlag);
  };
  QString name() override { return QString("Insert Row After"); }
  QString icon() override { return QString("://insert_row.png"); }
  QString tooltip() override { return QString("Inserts row after"); }
  QString whatsthis() override {
    return QString("Inserts a new row after the last selected row. If "
                   "groups exist and a group is selected, the new row is "
                   "appended to the selected group. If nothing is selected "
                   "then a new row is added to the last group");
  }
  QString shortcut() override { return QString(); }
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORAPPENDROWCOMMAND_H*/
