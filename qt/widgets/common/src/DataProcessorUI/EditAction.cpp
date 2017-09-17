#include "MantidQtWidgets/Common/DataProcessorUI/EditAction.h"
namespace MantidQt {
namespace MantidWidgets {
std::array<EditAction, 9> EXPORT_OPT_MANTIDQT_COMMON
modificationActions() {
  return {
      {EditAction::INSERT_ROW_AFTER,
       EditAction::INSERT_GROUP_AFTER,
       EditAction::GROUP_SELECTED, EditAction::COPY_SELECTED,
       EditAction::CUT_SELECTED, EditAction::PASTE_SELECTED,
       EditAction::CLEAR_SELECTED, EditAction::DELETE_ROW,
       EditAction::DELETE_GROUP}};
}
}
}
