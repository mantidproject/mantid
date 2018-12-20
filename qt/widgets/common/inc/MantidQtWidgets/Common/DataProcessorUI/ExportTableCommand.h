#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSOREXPORTTABLECOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSOREXPORTTABLECOMMAND_H

#include "MantidQtWidgets/Common/DataProcessorUI/CommandBase.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** @class ExportTableCommand

ExportTableCommand defines the action "Export .TBL"

processor interface presenter needs to support.

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
class ExportTableCommand : public CommandBase {
public:
  ExportTableCommand(DataProcessorPresenter *tablePresenter)
      : CommandBase(tablePresenter){};
  ExportTableCommand(const QDataProcessorWidget &widget)
      : CommandBase(widget){};
  virtual ~ExportTableCommand(){};

  void execute() override {
    m_presenter->notify(DataProcessorPresenter::ExportTableFlag);
  };
  QString name() override { return QString("Export .TBL"); }
  QString icon() override { return QString("://save_template.png"); }
  QString tooltip() override { return QString("Export .TBL file"); }
  QString whatsthis() override {
    return QString("Opens a dialog to export a table as .TBL file");
  }
  QString shortcut() override { return QString(); }
  bool modifiesSettings() override { return false; }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSOREXPORTTABLECOMMAND_H*/
