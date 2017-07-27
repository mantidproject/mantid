#ifndef MANTIDQT_API_MANTIDDESKTOPSERVICES_H_
#define MANTIDQT_API_MANTIDDESKTOPSERVICES_H_

#include "DllOption.h"
#include <QDesktopServices>

namespace MantidQt {
namespace API {

/**
  This class provides a wrapper around QDesktopServices to fix a bug in opening
  URLs in firefox when tcmalloc is in the LD_PRELOAD environment variable on
  Linux. All other methods are simply passed through to QDesktopServices.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class EXPORT_OPT_MANTIDQT_API MantidDesktopServices {
public:
  static bool openUrl(const QUrl &url);
  static void setUrlHandler(const QString &scheme, QObject *receiver,
                            const char *method);
  static void unsetUrlHandler(const QString &scheme);

  static QString storageLocation(QDesktopServices::StandardLocation type);
  static QString displayName(QDesktopServices::StandardLocation type);
};
}
}

#endif // MANTIDQT_API_MANTIDDESKTOPSERVICES_H_
