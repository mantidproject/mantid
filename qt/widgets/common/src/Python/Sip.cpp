// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/Python/Sip.h"
#include <QtGlobal>
#include <sip.h>

namespace MantidQt {
namespace Widgets {
namespace Common {
namespace Python {

namespace Detail {
/**
 * @return A pointer to the C++ sip api object
 */
const sipAPIDef *sipAPI() {
  static const sipAPIDef *sip_API = nullptr;
  if (sip_API)
    return sip_API;

    // Some configs have a private sip module inside PyQt. Try this first
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  sip_API = (const sipAPIDef *)PyCapsule_Import("PyQt4.sip._C_API", 0);
#elif QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) &&                               \
    QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  sip_API = (const sipAPIDef *)PyCapsule_Import("PyQt5.sip._C_API", 0);
#else
#error "Unknown sip module for Qt >= 6"
#endif
  // Try plain sip module
  if (!sip_API) {
    PyErr_Clear();
    sip_API = (const sipAPIDef *)PyCapsule_Import("sip._C_API", 0);
  }

  assert(sip_API);
  return sip_API;
}
} // namespace Detail

} // namespace Python
} // namespace Common
} // namespace Widgets
} // namespace MantidQt
