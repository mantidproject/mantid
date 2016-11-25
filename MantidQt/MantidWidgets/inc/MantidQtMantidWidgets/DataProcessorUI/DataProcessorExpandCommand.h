#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSOREXPANDCOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSOREXPANDCOMMAND_H

#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCommandBase.h"

namespace MantidQt {
namespace MantidWidgets {
/** @class DataProcessorExpandCommand

DataProcessorExpandCommand defines the action "Expand Selection"

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
class DataProcessorExpandCommand : public DataProcessorCommandBase {
public:
  DataProcessorExpandCommand(DataProcessorPresenter *tablePresenter)
      : DataProcessorCommandBase(tablePresenter){};
  virtual ~DataProcessorExpandCommand(){};

  void execute() override {
    m_presenter->notify(DataProcessorPresenter::ExpandSelectionFlag);
  };
  std::string name() override { return std::string("Expand Selection"); }
  std::string icon() override { return std::string("://fit_frame.png"); }
  std::string tooltip() override {
    return std::string("Selects an entire group");
  }
  std::string whatsthis() override {
    return std::string("Expands the current selection to include any runs that "
                       "are in the same group as any selected run. This "
                       "effectively means selecting the group to which the "
                       "selected run belongs");
  }
  std::string shortcut() override { return std::string(); }
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSOREXPANDCOMMAND_H*/
