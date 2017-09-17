#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORWORKSPACECOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORWORKSPACECOMMAND_H

#include "MantidQtWidgets/Common/DataProcessorUI/TableQueryCommandBase.h"

namespace MantidQt {
namespace MantidWidgets {
/** @class DataProcessorWorkspaceCommand

DataProcessorWorkspaceCommand defines a workspace action

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
class DataProcessorWorkspaceCommand : public TableQueryCommandBase {
public:
  DataProcessorWorkspaceCommand(DataProcessorPresenter *tablePresenter,
                                const QString &name)
      : TableQueryCommandBase(tablePresenter), m_name(name){};
  DataProcessorWorkspaceCommand(const QDataProcessorWidget &widget,
                                const QString &name)
      : TableQueryCommandBase(widget), m_name(name){};
  virtual ~DataProcessorWorkspaceCommand(){};

  void execute() override {
    // Tell the presenter which of the available workspaces was selected
    m_presenter->setModel(m_name);
  };
  QString name() const override { return m_name; }
  QString icon() const override { return QString("://worksheet.png"); }
  QString tooltip() const override { return QString("Table Workspace"); }
  QString whatsthis() const override { return QString("Table Workspace"); }
  QString shortcut() const override { return QString(); }

private:
  QString m_name;
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORWORKSPACECOMMAND_H*/
