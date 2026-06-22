// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0

#pragma once

#include <QMetaObject>
#include <QtGlobal>

namespace MantidQt::MantidWidgets {

// Q_ARG / Q_RETURN_ARG produce QGenericArgument/QGenericReturnArgument in Qt5 but
// QMetaMethodArgument/QMetaMethodReturnArgument in Qt6. Alias to the right type so
// the wrapper (and its mock) match what the macros generate.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
using QtInvokeArgument = QMetaMethodArgument;
using QtInvokeReturnArgument = QMetaMethodReturnArgument;

namespace detail {
// In Qt5 QMetaObject::invokeMethod accepted a fixed list of arguments terminated
// by the first default-constructed (null) QGenericArgument, so the wrapper below
// could always forward all ten slots and let Qt ignore the trailing padding.
//
// In Qt6 invokeMethod is variadic and counts its arguments at compile time; it has
// no notion of a null terminator. Forwarding the padding would make Qt believe the
// slot takes ten parameters, the method lookup fails, and Qt crashes while trying to
// report the missing method (printMethodNotFoundWarning). So only forward the leading,
// populated arguments. Q_ARG sets .name to the type string; default QtInvokeArgument()
// leaves it null, which marks the end of the real arguments.
inline int realArgCount(const QtInvokeArgument *args) {
  int n = 0;
  while (n < 10 && args[n].name != nullptr)
    ++n;
  return n;
}

inline bool forwardInvoke(QObject *obj, const char *member, Qt::ConnectionType type, const QtInvokeArgument *a) {
  switch (realArgCount(a)) {
  case 0:
    return QMetaObject::invokeMethod(obj, member, type);
  case 1:
    return QMetaObject::invokeMethod(obj, member, type, a[0]);
  case 2:
    return QMetaObject::invokeMethod(obj, member, type, a[0], a[1]);
  case 3:
    return QMetaObject::invokeMethod(obj, member, type, a[0], a[1], a[2]);
  case 4:
    return QMetaObject::invokeMethod(obj, member, type, a[0], a[1], a[2], a[3]);
  case 5:
    return QMetaObject::invokeMethod(obj, member, type, a[0], a[1], a[2], a[3], a[4]);
  case 6:
    return QMetaObject::invokeMethod(obj, member, type, a[0], a[1], a[2], a[3], a[4], a[5]);
  case 7:
    return QMetaObject::invokeMethod(obj, member, type, a[0], a[1], a[2], a[3], a[4], a[5], a[6]);
  case 8:
    return QMetaObject::invokeMethod(obj, member, type, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
  case 9:
    return QMetaObject::invokeMethod(obj, member, type, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8]);
  default:
    return QMetaObject::invokeMethod(obj, member, type, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9]);
  }
}
} // namespace detail
#else
using QtInvokeArgument = QGenericArgument;
using QtInvokeReturnArgument = QGenericReturnArgument;
#endif

struct QtMetaObject {
  virtual ~QtMetaObject() = default;

  virtual bool invokeMethod(QObject *obj, const char *member, Qt::ConnectionType type, QtInvokeReturnArgument ret,
                            QtInvokeArgument val0 = QtInvokeArgument(), QtInvokeArgument val1 = QtInvokeArgument(),
                            QtInvokeArgument val2 = QtInvokeArgument(), QtInvokeArgument val3 = QtInvokeArgument(),
                            QtInvokeArgument val4 = QtInvokeArgument(), QtInvokeArgument val5 = QtInvokeArgument(),
                            QtInvokeArgument val6 = QtInvokeArgument(), QtInvokeArgument val7 = QtInvokeArgument(),
                            QtInvokeArgument val8 = QtInvokeArgument(),
                            QtInvokeArgument val9 = QtInvokeArgument()) const {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt6's variadic invokeMethod only accepts a typed QTemplatedMetaMethodReturnArgument,
    // which cannot be reconstructed from the type-erased base passed here. Nothing in the
    // instrument view captures a return value, so the return argument is unused under Qt6.
    (void)ret;
    const QtInvokeArgument args[10] = {val0, val1, val2, val3, val4, val5, val6, val7, val8, val9};
    return detail::forwardInvoke(obj, member, type, args);
#else
    return QMetaObject::invokeMethod(obj, member, type, ret, val0, val1, val2, val3, val4, val5, val6, val7, val8,
                                     val9);
#endif
  }

  virtual bool invokeMethod(QObject *obj, const char *member, QtInvokeReturnArgument ret,
                            QtInvokeArgument val0 = QtInvokeArgument(), QtInvokeArgument val1 = QtInvokeArgument(),
                            QtInvokeArgument val2 = QtInvokeArgument(), QtInvokeArgument val3 = QtInvokeArgument(),
                            QtInvokeArgument val4 = QtInvokeArgument(), QtInvokeArgument val5 = QtInvokeArgument(),
                            QtInvokeArgument val6 = QtInvokeArgument(), QtInvokeArgument val7 = QtInvokeArgument(),
                            QtInvokeArgument val8 = QtInvokeArgument(),
                            QtInvokeArgument val9 = QtInvokeArgument()) const {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // See the note above: the type-erased return argument is unused under Qt6.
    (void)ret;
    const QtInvokeArgument args[10] = {val0, val1, val2, val3, val4, val5, val6, val7, val8, val9};
    return detail::forwardInvoke(obj, member, Qt::AutoConnection, args);
#else
    return QMetaObject::invokeMethod(obj, member, ret, val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
#endif
  }

  virtual bool invokeMethod(QObject *obj, const char *member, Qt::ConnectionType type,
                            QtInvokeArgument val0 = QtInvokeArgument(), QtInvokeArgument val1 = QtInvokeArgument(),
                            QtInvokeArgument val2 = QtInvokeArgument(), QtInvokeArgument val3 = QtInvokeArgument(),
                            QtInvokeArgument val4 = QtInvokeArgument(), QtInvokeArgument val5 = QtInvokeArgument(),
                            QtInvokeArgument val6 = QtInvokeArgument(), QtInvokeArgument val7 = QtInvokeArgument(),
                            QtInvokeArgument val8 = QtInvokeArgument(),
                            QtInvokeArgument val9 = QtInvokeArgument()) const {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QtInvokeArgument args[10] = {val0, val1, val2, val3, val4, val5, val6, val7, val8, val9};
    return detail::forwardInvoke(obj, member, type, args);
#else
    return QMetaObject::invokeMethod(obj, member, type, val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
#endif
  }

  virtual bool invokeMethod(QObject *obj, const char *member, QtInvokeArgument val0 = QtInvokeArgument(),
                            QtInvokeArgument val1 = QtInvokeArgument(), QtInvokeArgument val2 = QtInvokeArgument(),
                            QtInvokeArgument val3 = QtInvokeArgument(), QtInvokeArgument val4 = QtInvokeArgument(),
                            QtInvokeArgument val5 = QtInvokeArgument(), QtInvokeArgument val6 = QtInvokeArgument(),
                            QtInvokeArgument val7 = QtInvokeArgument(), QtInvokeArgument val8 = QtInvokeArgument(),
                            QtInvokeArgument val9 = QtInvokeArgument()) const {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QtInvokeArgument args[10] = {val0, val1, val2, val3, val4, val5, val6, val7, val8, val9};
    return detail::forwardInvoke(obj, member, Qt::AutoConnection, args);
#else
    return QMetaObject::invokeMethod(obj, member, val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
#endif
  }
};
} // namespace MantidQt::MantidWidgets
