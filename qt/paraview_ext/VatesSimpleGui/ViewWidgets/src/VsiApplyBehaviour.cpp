#include "MantidVatesSimpleGuiViewWidgets/VsiApplyBehaviour.h"

namespace Mantid {
namespace Vates {
namespace SimpleGui {

VsiApplyBehaviour::VsiApplyBehaviour(Mantid::VATES::ColorScaleLock *lock,
                                     QObject *parent)
    : pqApplyBehavior(parent), m_colorScaleLock(lock) {}

/**
 * Forward the register request
 * @param panel: the properies panel
 */
void VsiApplyBehaviour::registerPanel(pqPropertiesPanel *panel) {
  this->pqApplyBehavior::registerPanel(panel);
}

/* Forward the unregister request
 * @param panel: the properties panel
 */
void VsiApplyBehaviour::unregisterPanel(pqPropertiesPanel *panel) {
  this->pqApplyBehavior::unregisterPanel(panel);
}

/// React to the apply button press. We forward the request, but we add a lock
void VsiApplyBehaviour::applied(pqPropertiesPanel *, pqProxy *pqproxy) {
  Mantid::VATES::ColorScaleLockGuard colorScaleLockGuard(m_colorScaleLock);
  this->pqApplyBehavior::applied(nullptr, pqproxy);
}

/// React to the apply button press. We forward the request, but we add a lock
void VsiApplyBehaviour::applied(pqPropertiesPanel *) {
  Mantid::VATES::ColorScaleLockGuard colorScaleLockGuard(m_colorScaleLock);
  this->pqApplyBehavior::applied(nullptr);
}
} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid
