#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOMMANDBASE_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOMMANDBASE_H

#include "MantidQtWidgets/Common/DataProcessorUI/Command.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/QDataProcessorWidget.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

/** @class ReflCommandBase

ReflCommandBase is an interface which defines the functions any data processor
action needs to support. Defines a IReflTablePresenter that will be notified.

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
class CommandBase : public Command {
public:
  CommandBase(DataProcessorPresenter *tablePresenter)
      : m_presenter(tablePresenter) {
    if (!tablePresenter) {
      throw std::invalid_argument("Invalid abstract presenter");
    }
  };
  CommandBase(const QDataProcessorWidget &widget)
      : CommandBase(widget.getPresenter()) {}

protected:
  DataProcessorPresenter *const m_presenter;
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOMMANDBASE_H*/
