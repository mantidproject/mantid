#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORSEPARATORCOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORSEPARATORCOMMAND_H

#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCommandBase.h"

namespace MantidQt {
namespace MantidWidgets {
/** @class DataProcessorSeparatorCommand

DataProcessorSeparatorCommand defines a separator. It has no name, no icon and
empty
execute() method

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
class DataProcessorSeparatorCommand : public DataProcessorCommandBase {
public:
  DataProcessorSeparatorCommand(DataProcessorPresenter *tablePresenter)
      : DataProcessorCommandBase(tablePresenter){};
  DataProcessorSeparatorCommand(const QDataProcessorWidget &widget)
      : DataProcessorCommandBase(widget){};
  virtual ~DataProcessorSeparatorCommand(){};

  void execute() override{};
  QString name() override { return QString(); }
  QString icon() override { return QString(); }
  QString tooltip() override { return QString(); }
  QString whatsthis() override { return QString(); }
  QString shortcut() override { return QString(); }
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORSEPARATORCOMMAND_H*/
