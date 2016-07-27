#include "MantidQtMantidWidgets/LogValueSelector.h"

namespace MantidQt {
namespace MantidWidgets {

/**
 * Constructor for the widget
 * @param parent :: [input] Parent dialog for the widget
 */
LogValueSelector::LogValueSelector(QWidget *parent)
    : API::MantidWidget(parent) {
  m_ui.setupUi(this);
}

} // namespace MantidWidgets
} // namespace MantidQt
