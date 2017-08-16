#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorAction.h"
namespace MantidQt {
namespace MantidWidgets {
std::array<DataProcessorAction, 9> EXPORT_OPT_MANTIDQT_MANTIDWIDGETS
tableModificationActions() {
  return {
      {DataProcessorAction::INSERT_ROW_AFTER,
       DataProcessorAction::INSERT_GROUP_AFTER,
       DataProcessorAction::GROUP_SELECTED, DataProcessorAction::COPY_SELECTED,
       DataProcessorAction::CUT_SELECTED, DataProcessorAction::PASTE_SELECTED,
       DataProcessorAction::CLEAR_SELECTED, DataProcessorAction::DELETE_ROW,
       DataProcessorAction::DELETE_GROUP}};
}
}
}
