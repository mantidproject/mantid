#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORSAVETABLECOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORSAVETABLECOMMAND_H

#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCommandBase.h"

namespace MantidQt {
namespace MantidWidgets {
/** @class DataProcessorSaveTableCommand

DataProcessorSaveTableCommand defines the action "Save Table"

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
class DataProcessorSaveTableCommand : public DataProcessorCommandBase {
public:
  DataProcessorSaveTableCommand(DataProcessorPresenter *tablePresenter)
      : DataProcessorCommandBase(tablePresenter){};
  DataProcessorSaveTableCommand(const QDataProcessorWidget &widget)
      : DataProcessorCommandBase(widget){};
  virtual ~DataProcessorSaveTableCommand(){};

  void execute() override {
    m_presenter->notify(DataProcessorPresenter::SaveFlag);
  };
  QString name() override { return QString("Save Table"); }
  QString icon() override { return QString("://filesave.png"); }
  QString tooltip() override { return QString("Save Table"); }
  QString whatsthis() override {
    return QString("Saves current table as a table workspace");
  }
  QString shortcut() override { return QString(); }
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORSAVETABLECOMMAND_H*/
