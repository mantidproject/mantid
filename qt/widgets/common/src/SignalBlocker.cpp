// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/SignalBlocker.h"
#if QT_VERSION < QT_VERSION_CHECK(5, 3, 0)

#include <cassert>

namespace MantidQt {
namespace API {

/**
 * Enable signals for the wrapped object
 * @param obj : QObject to block signals for.
 */
SignalBlocker::SignalBlocker(QObject *obj) : m_obj(obj) {
  assert(m_obj != nullptr);
  m_obj->blockSignals(true);
}

/**
 * Enable signals for the wrapped object
 */
SignalBlocker::~SignalBlocker() { m_obj->blockSignals(false); }

} // namespace API
} // namespace MantidQt

#endif
