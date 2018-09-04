#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORDELETEROWCOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORDELETEROWCOMMAND_H

#include "MantidQtWidgets/Common/DataProcessorUI/CommandBase.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

/** @class DeleteRowCommand

DeleteRowCommand defines the action "Delete Row"

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
class DeleteRowCommand : public CommandBase {
public:
  DeleteRowCommand(DataProcessorPresenter *tablePresenter)
      : CommandBase(tablePresenter){};
  DeleteRowCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~DeleteRowCommand(){};

  void execute() override {
    m_presenter->notify(DataProcessorPresenter::DeleteRowFlag);
  };
  QString name() override { return QString("Delete Row"); }
  QString icon() override { return QString("://delete_row.png"); }
  QString tooltip() override { return QString("Deletes a row"); }
  QString whatsthis() override { return QString("Deletes the selected row"); }
  QString shortcut() override { return QString(); }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORDELETEROWCOMMAND_H*/
