#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSOROPENTABLECOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSOROPENTABLECOMMAND_H

#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCommandBase.h"

namespace MantidQt {
namespace MantidWidgets {
/** @class DataProcessorOpenTableCommand

DataProcessorOpenTableCommand defines the action "Open Table"

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
class DataProcessorOpenTableCommand : public DataProcessorCommandBase {
public:
  DataProcessorOpenTableCommand(DataProcessorPresenter *tablePresenter)
      : DataProcessorCommandBase(tablePresenter){};
  DataProcessorOpenTableCommand(const QDataProcessorWidget &widget)
      : DataProcessorCommandBase(widget){};
  virtual ~DataProcessorOpenTableCommand(){};

  void execute() override{
      // This action should do nothing
  };
  QString name() override { return QString("Open Table"); }
  QString icon() override { return QString("://multiload.png"); }
  QString tooltip() override { return QString("Open Table"); }
  QString whatsthis() override {
    return QString("Loads a table into the interface. Table must exist in "
                   "the ADS and be compatible in terms of the number and "
                   "type of columns");
  }
  QString shortcut() override { return QString(); }
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSOROPENTABLECOMMAND_H*/
