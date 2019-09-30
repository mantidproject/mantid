// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
 */
class EXPORT_OPT_MANTIDQT_COMMON MantidDesktopServices {
public:
  static bool openUrl(const QUrl &url);
  static bool openUrl(const QString &url);
  static void setUrlHandler(const QString &scheme, QObject *receiver,
                            const char *method);
  static void unsetUrlHandler(const QString &scheme);
};
} // namespace API
} // namespace MantidQt

#endif // MANTIDQT_API_MANTIDDESKTOPSERVICES_H_
