// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_SIMPLEGUIVIEWWIDGETS_VSI_APPLY_BEHAVIOUR_H
#define MANTID_VATES_SIMPLEGUIVIEWWIDGETS_VSI_APPLY_BEHAVIOUR_H

#include "MantidVatesAPI/ColorScaleGuard.h"
#include <QObject>
#include <pqApplyBehavior.h>
#include <pqPropertiesPanel.h>
#include <pqProxy.h>

namespace Mantid {
namespace Vates {
namespace SimpleGui {
class VsiApplyBehaviour : public pqApplyBehavior {
public:
  Q_OBJECT
  using Superclass = QObject;

public:
  VsiApplyBehaviour(Mantid::VATES::ColorScaleLock *lock,
                    QObject *parent = nullptr);
  ~VsiApplyBehaviour() override{};

  /// Register/unregister pqPropertiesPanel instances to monitor.
  void registerPanel(pqPropertiesPanel *panel);
  void unregisterPanel(pqPropertiesPanel *panel);
protected slots:
  void applied(pqPropertiesPanel *, pqProxy *) override;
  void applied(pqPropertiesPanel *) override;

private:
  Mantid::VATES::ColorScaleLock *m_colorScaleLock;
};
} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid
#endif
