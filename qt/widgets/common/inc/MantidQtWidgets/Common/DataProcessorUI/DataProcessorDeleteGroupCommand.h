#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORDELETEGROUPCOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORDELETEGROUPCOMMAND_H

#include "MantidQtWidgets/Common/DataProcessorUI/TableModificationCommandBase.h"

namespace MantidQt {
namespace MantidWidgets {

/** @class DataProcessorDeleteGroupCommand

DataProcessorDeleteGroupCommand defines the action "Delete Group"

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
class DataProcessorDeleteGroupCommand : public TableModificationCommandBase {
public:
  using TableModificationCommandBase::TableModificationCommandBase;
  virtual ~DataProcessorDeleteGroupCommand(){};

  void execute() override {
    m_presenter->notify(DataProcessorPresenter::DeleteGroupFlag);
  };
  QString name() const override { return QString("Delete Group"); }
  QString icon() const override { return QString("://delete_group.png"); }
  QString tooltip() const override { return QString("Deletes selected group"); }
  QString whatsthis() const override {
    return QString("Deletes the selected groups");
  }
  QString shortcut() const override { return QString(); }
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORDELETEGROUPCOMMAND_H*/
