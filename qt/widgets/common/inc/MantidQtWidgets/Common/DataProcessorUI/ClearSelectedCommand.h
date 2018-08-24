#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORCLEARSELECTEDCOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORCLEARSELECTEDCOMMAND_H

#include "MantidQtWidgets/Common/DataProcessorUI/CommandBase.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** @class ClearSelectedCommand

ClearSelectedCommand defines the action "Clear Selected"

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
class ClearSelectedCommand : public CommandBase {
public:
  ClearSelectedCommand(DataProcessorPresenter *tablePresenter)
      : CommandBase(tablePresenter){};
  ClearSelectedCommand(const QDataProcessorWidget &widget)
      : CommandBase(widget){};
  virtual ~ClearSelectedCommand(){};

  void execute() override {
    m_presenter->notify(DataProcessorPresenter::ClearSelectedFlag);
  };
  QString name() override { return QString("Clear Selected"); }
  QString icon() override { return QString("://erase.png"); }
  QString tooltip() override { return QString("Clear selected"); }
  QString whatsthis() override {
    return QString("Clears the contents of the selected rows");
  }
  QString shortcut() override { return QString(); }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORCLEARSELECTEDCOMMAND_H*/
