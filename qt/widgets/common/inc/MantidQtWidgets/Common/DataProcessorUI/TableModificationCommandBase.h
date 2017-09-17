#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORTABLEMODIFICATIONCOMMANDBASE_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORTABLEMODIFICATIONCOMMANDBASE_H

#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorCommandBase.h"
#include "MantidQtWidgets/Common/DllOption.h"

namespace MantidQt {
namespace MantidWidgets {

/** @class TableModificationCommandBase

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
class EXPORT_OPT_MANTIDQT_COMMON TableModificationCommandBase : public DataProcessorCommandBase {
public:
  using DataProcessorCommandBase::DataProcessorCommandBase;
  TableModificationCommandBase(const TableModificationCommandBase&) = delete;
  TableModificationCommandBase &operator=(const TableModificationCommandBase&) = delete;
  bool modifiesTable() const override;
};
}
}
#endif // MANTIDQTMANTIDWIDGETS_DATAPROCESSORTABLEMODIFICATIONCOMMANDBASE_H
