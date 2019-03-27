// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTAPI_SELECTION_NOTIFICATION_SERVICE_H_
#define MANTIDQTAPI_SELECTION_NOTIFICATION_SERVICE_H_

#include "DllOption.h"
#include "MantidKernel/SingletonHolder.h"
#include <QObject>

namespace MantidQt {
namespace API {
//---------------------------------------------------------------------
//
//---------------------------------------------------------------------

/**
Provides a simple loosely coupled mechanism for passing information
about a selected point from several possible sources to several
possible destinations.  Neither the sources, or the destinations
need exist or have references to each other.  Currently the only
"message" supported is a QPointSelection message.  To send the message,
an object just needs to call the sendQPointSelection() method on the
single Instance() of this class.
*/
class EXPORT_OPT_MANTIDQT_COMMON SelectionNotificationServiceImpl
    : public QObject {
  Q_OBJECT

public:
  /// Emit the QPointSelection_signal, callable from any thread
  void sendQPointSelection(bool lab_coords, double qx, double qy, double qz);

signals:
  void QPointSelection_signal(bool /*_t1*/, double /*_t2*/, double /*_t3*/, double /*_t4*/);

private:
  /// private constructor, since SelectionNotificationService is a singleton
  SelectionNotificationServiceImpl();

  /// private constructor, since SelectionNotificationService is a singleton
  ~SelectionNotificationServiceImpl() override;

  friend struct Mantid::Kernel::CreateUsingNew<
      SelectionNotificationServiceImpl>;
};

using SelectionNotificationService =
    Mantid::Kernel::SingletonHolder<SelectionNotificationServiceImpl>;
} // namespace API
} // namespace MantidQt
namespace Mantid {
namespace Kernel {
EXTERN_MANTIDQT_COMMON template class EXPORT_OPT_MANTIDQT_COMMON Mantid::
    Kernel::SingletonHolder<MantidQt::API::SelectionNotificationServiceImpl>;
}
} // namespace Mantid

#endif // MANTIDQTAPI_SELECTION_NOTIFICATION_SERVICE_H_
