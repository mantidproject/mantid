#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSOROPTIONSTABLECOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSOROPTIONSTABLECOMMAND_H

#include "MantidQtWidgets/Common/DataProcessorUI/CommandBase.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** @class OptionsCommand

OptionsCommand defines the action "Import .TBL"

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
class OptionsCommand : public CommandBase {
public:
  OptionsCommand(DataProcessorPresenter *tablePresenter)
      : CommandBase(tablePresenter){};
  OptionsCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~OptionsCommand(){};

  void execute() override {
    m_presenter->notify(DataProcessorPresenter::OptionsDialogFlag);
  };
  QString name() override { return QString("Options"); }
  QString icon() override { return QString("://configure.png"); }
  QString tooltip() override { return QString("Options"); }
  QString whatsthis() override {
    return QString("Opens a dialog with some options for the table");
  }
  QString shortcut() override { return QString(); }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSOROPTIONSTABLECOMMAND_H*/
