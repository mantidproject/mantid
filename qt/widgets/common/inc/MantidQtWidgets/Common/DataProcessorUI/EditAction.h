#ifndef MANTIDQTMANTIDWIDGETS_EDITACTION_H
#define MANTIDQTMANTIDWIDGETS_EDITACTION_H
#include "MantidQtWidgets/Common/DllOption.h"
#include <array>
namespace MantidQt {
namespace MantidWidgets {
/** @enum EditAction

DataProcesorAction is an enumeration of commands which can be enabled or
disabled on the GenericDataProcessor interface.

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
enum class EditAction {
  PROCESS,
  PAUSE,
  SELECT_GROUP,
  EXPAND_GROUP,
  COLAPSE_GROUP,
  PLOT_RUNS,
  PLOT_GROUP,
  INSERT_ROW_AFTER,
  INSERT_GROUP_AFTER,
  GROUP_SELECTED,
  COPY_SELECTED,
  CUT_SELECTED,
  PASTE_SELECTED,
  CLEAR_SELECTED,
  DELETE_ROW,
  DELETE_GROUP,
  WHATS_THIS
};

std::array<EditAction, 9> EXPORT_OPT_MANTIDQT_COMMON
modificationActions();

template <typename DisableAction>
void disableModificationActions(DisableAction disableAction) {
  auto actionsToDisable = modificationActions();
  for (auto action : actionsToDisable)
    disableAction(action);
}

template <typename EnableAction>
void enableModificationActions(EnableAction enableAction) {
  auto actionsToEnable = modificationActions();
  for (auto action : actionsToEnable)
    enableAction(action);
}
}
}
#endif /*MANTIDQTMANTIDWIDGETS_EDITACTION_H*/
