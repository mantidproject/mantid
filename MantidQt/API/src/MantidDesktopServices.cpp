#include "MantidQtAPI/MantidDesktopServices.h"

#include <QDesktopServices>

#ifdef __linux__
#include <QProcessEnvironment>
#include <cstdlib>

namespace {
// String name of LD_PRELOAD environment variable
constexpr const char *LDPRELOAD_ENV = "LD_PRELOAD";
}
#endif

#include <iostream>

namespace MantidQt {
namespace API {

/**
 * Opens a url in the appropriate web browser. On Linux systems if LD_PRELOAD is
 * defined as an environment variable then it is removed for the duration of the
 * call
 * to the web browser. This is to avoid known issues with LD_PRELOAD libraries
 * and some
 * web browsers, e.g. firefox. On all other systems the method simply passes
 * through to
 * QDesktopServies
 * @param url Address to be opened
 */
bool MantidDesktopServices::openUrl(const QUrl &url) {
#ifndef __linux__
  return QDesktopServices::openUrl(url);
#else
  // Remove LD_PRELOAD if present
  auto systemEnv = QProcessEnvironment::systemEnvironment();
  auto ldpreload = systemEnv.value(LDPRELOAD_ENV, QString());
  if (!ldpreload.isEmpty()) {
    unsetenv(LDPRELOAD_ENV);
  }
  auto status = QDesktopServices::openUrl(url);
  if (!ldpreload.isEmpty()) {
    setenv(LDPRELOAD_ENV, ldpreload.toLatin1().constData(), 1 /* overwrite*/);
  }
  return status;
#endif
}

/**
 * Pass through method to MantidDesktopServices::setUrlHandler. See Qt
 * documentation
 * for
 * further details.
 * @param scheme Name of scheme to handle
 * @param receiver Handler object
 * @param method Method called on the receiver object
 */
void MantidDesktopServices::setUrlHandler(const QString &scheme,
                                          QObject *receiver,
                                          const char *method) {
  QDesktopServices::setUrlHandler(scheme, receiver, method);
}

/**
 * Pass through method to MantidDesktopServices::unsetUrlHandler. See Qt
 * documentation for
 * further details.
 * @param scheme Name of scheme to drop
 */
void MantidDesktopServices::unsetUrlHandler(const QString &scheme) {
  QDesktopServices::unsetUrlHandler(scheme);
}

/**
 * Pass through method to MantidDesktopServices::storageLocation. See Qt
 * documentation for
 * further details.
 * @param type File type
 */
QString MantidDesktopServices::storageLocation(
    QDesktopServices::StandardLocation type) {
  return QDesktopServices::storageLocation(type);
}

/**
 * Pass through method to MantidDesktopServices::displayName. See Qt
 * documentation
 * for
 * further details.
 * @param type Location type
 */
QString
MantidDesktopServices::displayName(QDesktopServices::StandardLocation type) {
  return QDesktopServices::displayName(type);
}
}
}
