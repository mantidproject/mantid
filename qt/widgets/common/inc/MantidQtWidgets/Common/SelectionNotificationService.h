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

Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class EXPORT_OPT_MANTIDQT_COMMON SelectionNotificationServiceImpl
    : public QObject {
  Q_OBJECT

public:
  /// Emit the QPointSelection_signal, callable from any thread
  void sendQPointSelection(bool lab_coords, double qx, double qy, double qz);

signals:
  void QPointSelection_signal(bool, double, double, double);

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
