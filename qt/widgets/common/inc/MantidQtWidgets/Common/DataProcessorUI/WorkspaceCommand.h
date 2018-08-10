#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORWORKSPACECOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORWORKSPACECOMMAND_H

#include "MantidQtWidgets/Common/DataProcessorUI/CommandBase.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** @class WorkspaceCommand

WorkspaceCommand defines a workspace action

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
class WorkspaceCommand : public CommandBase {
public:
  WorkspaceCommand(DataProcessorPresenter *tablePresenter, const QString &name)
      : CommandBase(tablePresenter), m_name(name){};
  WorkspaceCommand(const QDataProcessorWidget &widget, const QString &name)
      : CommandBase(widget), m_name(name){};
  virtual ~WorkspaceCommand(){};

  void execute() override {
    // Tell the presenter which of the available workspaces was selected
    m_presenter->setModel(m_name);
  };
  QString name() override { return m_name; }
  QString icon() override { return QString("://worksheet.png"); }
  QString tooltip() override { return QString("Table Workspace"); }
  QString whatsthis() override { return QString("Table Workspace"); }
  QString shortcut() override { return QString(); }

private:
  QString m_name;
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORWORKSPACECOMMAND_H*/
