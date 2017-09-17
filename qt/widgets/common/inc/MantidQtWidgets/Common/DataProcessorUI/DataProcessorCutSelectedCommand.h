#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORCUTSELECTEDCOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORCUTSELECTEDCOMMAND_H

#include "MantidQtWidgets/Common/DataProcessorUI/TableModificationCommandBase.h"

namespace MantidQt {
namespace MantidWidgets {
/** @class DataProcessorCutSelectedCommand

DataProcessorCutSelectedCommand defines the action "Cut Selected"

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
class DataProcessorCutSelectedCommand : public TableModificationCommandBase {
public:
  using TableModificationCommandBase::TableModificationCommandBase;
  virtual ~DataProcessorCutSelectedCommand(){};

  void execute() override {
    m_presenter->notify(DataProcessorPresenter::CutSelectedFlag);
  };
  QString name() const override { return QString("Cut Selected"); }
  QString icon() const override { return QString("://cut.png"); }
  QString tooltip() const override { return QString("Cut selected"); }
  QString whatsthis() const override {
    return QString("Copies the selected rows to the clipboard, and then "
                   "deletes them. Each row is placed on a new line, and "
                   "each cell is separated by a tab");
  }
  QString shortcut() const override { return QString("Ctrl+X"); }
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORCUTSELECTEDCOMMAND_H*/
