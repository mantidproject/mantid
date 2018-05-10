#ifndef MANTIDQT_API_QSTRINGUTILS_H
#define MANTIDQT_API_QSTRINGUTILS_H

#include <QString>
#include <string>

namespace MantidQt {
namespace API {
/**
 * Internal version of QString::fromStdWString. On MSVC Qt4 rquired
 * /Z:wchar_t- but Qt5 does not. For simplicity we
 * remove the /Z:wchar_t- compiler option and
 * define the functionality interally.
 * @param str A pointer to the raw wchar_t string
 */
inline QString toQStringInternal(const wchar_t *str) {
  return sizeof(wchar_t) == sizeof(QChar)
             ? QString::fromUtf16(reinterpret_cast<const ushort *>(str),
                                  static_cast<int>(wcslen(str)))
             : QString::fromUcs4(reinterpret_cast<const uint *>(str),
                                 static_cast<int>(wcslen(str)));
}

/**
 * Internal version of QString::fromStdWString. On MSVC Qt4 rquired
 * /Z:wchar_t- but Qt5 does not. For simplicity we
 * remove the /Z:wchar_t- compiler option and
 * define the functionality interally.
 * @param str A std::wstring object
 */
inline QString toQStringInternal(const std::wstring &str) {
  return sizeof(wchar_t) == sizeof(QChar)
             ? QString::fromUtf16(reinterpret_cast<const ushort *>(str.data()),
                                  static_cast<int>(str.size()))
             : QString::fromUcs4(reinterpret_cast<const uint *>(str.data()),
                                 static_cast<int>(str.size()));
}
} // namespace API
} // namespace MantidQt

#endif // MANTIDQT_API_QSTRINGUTILS_H