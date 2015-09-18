#ifndef MANTID_VATES_SIMPLEGUIVIEWWIDGETS_VSI_APPLY_BEHAVIOUR_H
#define MANTID_VATES_SIMPLEGUIVIEWWIDGETS_VSI_APPLY_BEHAVIOUR_H

#include <QObject>
#include "MantidVatesAPI/ColorScaleGuard.h"
#include <pqApplyBehavior.h>
#include <pqPropertiesPanel.h>
#include <pqProxy.h>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
class VsiApplyBehaviour : public pqApplyBehavior {
public:
  Q_OBJECT
  typedef QObject Superclass;
public:
  VsiApplyBehaviour(Mantid::VATES::ColorScaleLock* lock, QObject* parent=0);
  virtual ~VsiApplyBehaviour() {};

  /// Register/unregister pqPropertiesPanel instances to monitor.
  void registerPanel(pqPropertiesPanel* panel);
  void unregisterPanel(pqPropertiesPanel* panel);
protected slots:
  virtual void applied(pqPropertiesPanel*, pqProxy*);
  virtual void applied(pqPropertiesPanel*);

private:
  Mantid::VATES::ColorScaleLock* m_colorScaleLock;
};
}
}
}
#endif