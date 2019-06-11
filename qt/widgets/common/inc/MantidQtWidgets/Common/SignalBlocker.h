// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_SIGNALBLOCKER_H_
#define MANTID_API_SIGNALBLOCKER_H_

#include "DllOption.h"
#include <QObject>
#include <QtGlobal>

namespace MantidQt {
namespace API {

#if QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
using SignalBlocker = QSignalBlocker;
#else

/**
 * SignalBlocker : RAII signal blocker for Qt < v5.3
 */
class EXPORT_OPT_MANTIDQT_COMMON SignalBlocker {

private:
  /// Object to manage blocking
  QObject *m_obj;

public:
  explicit SignalBlocker(QObject *obj);
  ~SignalBlocker();
  SignalBlocker(const SignalBlocker &) = delete;
};

#endif

} // namespace API
} // namespace MantidQt

#endif /* MANTID_API_SIGNALBLOCKER_H_ */
