#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORIMPORTTABLECOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORIMPORTTABLECOMMAND_H

#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCommandBase.h"

namespace MantidQt {
namespace MantidWidgets {
/** @class DataProcessorImportTableCommand

DataProcessorImportTableCommand defines the action "Import .TBL"

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
class DataProcessorImportTableCommand : public DataProcessorCommandBase {
public:
  DataProcessorImportTableCommand(DataProcessorPresenter *tablePresenter)
      : DataProcessorCommandBase(tablePresenter){};
  DataProcessorImportTableCommand(const QDataProcessorWidget &widget)
      : DataProcessorCommandBase(widget){};
  virtual ~DataProcessorImportTableCommand(){};

  void execute() override {
    m_presenter->notify(DataProcessorPresenter::ImportTableFlag);
  };
  QString name() override { return QString("Import .TBL"); }
  QString icon() override { return QString("://open_template.png"); }
  QString tooltip() override { return QString("Import .TBL file"); }
  QString whatsthis() override {
    return QString("Opens a dialog to select a .TBL file to import");
  }
  QString shortcut() override { return QString(); }
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORIMPORTTABLECOMMAND_H*/
