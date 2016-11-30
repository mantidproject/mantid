#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORDELETEROWCOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORDELETEROWCOMMAND_H

#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCommandBase.h"

namespace MantidQt {
namespace MantidWidgets {

/** @class DataProcessorDeleteRowCommand

DataProcessorDeleteRowCommand defines the action "Delete Row"

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
class DataProcessorDeleteRowCommand : public DataProcessorCommandBase {
public:
  DataProcessorDeleteRowCommand(DataProcessorPresenter *tablePresenter)
      : DataProcessorCommandBase(tablePresenter){};
  virtual ~DataProcessorDeleteRowCommand(){};

  void execute() override {
    m_presenter->notify(DataProcessorPresenter::DeleteRowFlag);
  };
  std::string name() override { return std::string("Delete Row"); }
  std::string icon() override { return std::string("://delete_row.png"); }
  std::string tooltip() override { return std::string("Deletes a row"); }
  std::string whatsthis() override {
    return std::string("Deletes the selected row");
  }
  std::string shortcut() override { return std::string(); }
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORDELETEROWCOMMAND_H*/
