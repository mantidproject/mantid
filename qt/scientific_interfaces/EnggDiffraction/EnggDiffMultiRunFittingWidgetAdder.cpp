#include "EnggDiffMultiRunFittingWidgetAdder.h"
#include "IEnggDiffMultiRunFittingWidgetView.h"

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffMultiRunFittingWidgetAdder::EnggDiffMultiRunFittingWidgetAdder(
    IEnggDiffMultiRunFittingWidgetView *widget)
    : m_widget(widget) {}

void EnggDiffMultiRunFittingWidgetAdder::
operator()(IEnggDiffMultiRunFittingWidgetOwner &owner) {
  owner.addWidget(m_widget);
}

} // CustomInterfaces
} // MantidQt
