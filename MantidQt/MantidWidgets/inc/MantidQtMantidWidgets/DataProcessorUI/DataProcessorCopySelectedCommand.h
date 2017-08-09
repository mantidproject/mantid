#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOPYSELECTEDCOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOPYSELECTEDCOMMAND_H

#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCommandBase.h"

namespace MantidQt {
namespace MantidWidgets {
/** @class DataProcessorCopySelectedCommand

DataProcessorCopySelectedCommand defines the action "Copy Selected"

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
class DataProcessorCopySelectedCommand : public DataProcessorCommandBase {
public:
  DataProcessorCopySelectedCommand(DataProcessorPresenter *tablePresenter)
      : DataProcessorCommandBase(tablePresenter){};
  DataProcessorCopySelectedCommand(const QDataProcessorWidget &widget)
      : DataProcessorCommandBase(widget){};
  virtual ~DataProcessorCopySelectedCommand(){};

  void execute() override {
    m_presenter->notify(DataProcessorPresenter::CopySelectedFlag);
  };
  QString name() override { return QString("Copy Selected"); }
  QString icon() override { return QString("://copy.png"); }
  QString tooltip() override { return QString("Copy selected"); }
  QString whatsthis() override {
    return QString("Copies the selected rows to the clipboard. Each row is "
                   "placed on a new line, and each cell is separated by a "
                   "tab");
  }
  QString shortcut() override { return QString("Ctrl+C"); }
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOPYSELECTEDCOMMAND_H*/
