#include "MantidQtWidgets/Common/DropEventHelper.h"

#include <QFileInfo>
#include <QMimeData>
#include <QStringList>
#include <QUrl>

// Compile on OSX only.
#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#endif // defined(__APPLE__)

using namespace MantidQt::MantidWidgets;

/** Workaround for file path bug on OSX >=10.10 with Qt4
 *
 * On Windows/Linux this simply returns the URL unchanged.
 *
 * For more information see this bug report:
 * https://bugreports.qt.io/browse/QTBUG-40449
 *
 * @param url :: the url to correct the path for
 * @returns a valid url to a file path
 */
QUrl fixupURL(const QUrl &url) {
#if defined(__APPLE__)
  QString localFileQString = url.toLocalFile();
  // Compile on OSX only.
  if (localFileQString.startsWith("/.file/id=")) {
    CFStringRef relCFStringRef = CFStringCreateWithCString(
        kCFAllocatorDefault, localFileQString.toUtf8().constData(),
        kCFStringEncodingUTF8);
    CFURLRef relCFURL = CFURLCreateWithFileSystemPath(
        kCFAllocatorDefault, relCFStringRef, kCFURLPOSIXPathStyle,
        false // isDirectory
    );
    CFErrorRef error = nullptr;
    CFURLRef absCFURL =
        CFURLCreateFilePathURL(kCFAllocatorDefault, relCFURL, &error);
    if (!error) {
      static const CFIndex maxAbsPathCStrBufLen = 4096;
      char absPathCStr[maxAbsPathCStrBufLen];
      if (CFURLGetFileSystemRepresentation(
              absCFURL,
              true, // resolveAgainstBase
              reinterpret_cast<UInt8 *>(&absPathCStr[0]),
              maxAbsPathCStrBufLen)) {
        localFileQString = QString(absPathCStr);
      }
    }
    CFRelease(absCFURL);
    CFRelease(relCFURL);
    CFRelease(relCFStringRef);
  }
  return QUrl(localFileQString);
#else
  return url;
#endif // defined(__APPLE__)
}

/** Extract a list of file names from a drop event.
 *
 * This is a special OSX version because the OSX broke the way Qt decoded
 * the file path in OSX 10.10. This is fixed in Qt5 but not backported to Qt4.
 *
 * @param event :: the event to extract file names from
 * @return a list of file names as a QStringList
 */
QStringList DropEventHelper::getFileNames(const QDropEvent *event) {
  QStringList filenames;
  const auto mimeData = event->mimeData();
  if (mimeData->hasUrls()) {
    const auto urlList = mimeData->urls();
    for (const auto &url : urlList) {
      const auto fileUrl = fixupURL(url);
      const auto fName = fileUrl.toLocalFile();
      if (fName.size() > 0) {
        filenames.append(fName);
      }
    }
  }
  return filenames;
}

/** Extract python file names from a drop event
 *
 * This will filter the list of file names extracted from a QDropEvent that
 * end with the extension .py
 *
 * @param event :: the QDropEvent to filter filenames from
 * @return a list of python file names
 */
QStringList DropEventHelper::extractPythonFiles(const QDropEvent *event) {
  QStringList filenames;

  for (const auto &name : getFileNames(event)) {
    QFileInfo fi(name);

    if (fi.suffix().toUpper() == "PY") {
      filenames.append(name);
    }
  }

  return filenames;
}
