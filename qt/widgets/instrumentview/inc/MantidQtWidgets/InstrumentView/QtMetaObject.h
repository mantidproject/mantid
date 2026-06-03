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
    return QMetaObject::invokeMethod(obj, member, type, ret, val0, val1, val2, val3, val4, val5, val6, val7, val8,
                                     val9);
  }

  virtual bool invokeMethod(QObject *obj, const char *member, QtInvokeReturnArgument ret,
                            QtInvokeArgument val0 = QtInvokeArgument(), QtInvokeArgument val1 = QtInvokeArgument(),
                            QtInvokeArgument val2 = QtInvokeArgument(), QtInvokeArgument val3 = QtInvokeArgument(),
                            QtInvokeArgument val4 = QtInvokeArgument(), QtInvokeArgument val5 = QtInvokeArgument(),
                            QtInvokeArgument val6 = QtInvokeArgument(), QtInvokeArgument val7 = QtInvokeArgument(),
                            QtInvokeArgument val8 = QtInvokeArgument(),
                            QtInvokeArgument val9 = QtInvokeArgument()) const {
    return QMetaObject::invokeMethod(obj, member, ret, val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
  }

  virtual bool invokeMethod(QObject *obj, const char *member, Qt::ConnectionType type,
                            QtInvokeArgument val0 = QtInvokeArgument(), QtInvokeArgument val1 = QtInvokeArgument(),
                            QtInvokeArgument val2 = QtInvokeArgument(), QtInvokeArgument val3 = QtInvokeArgument(),
                            QtInvokeArgument val4 = QtInvokeArgument(), QtInvokeArgument val5 = QtInvokeArgument(),
                            QtInvokeArgument val6 = QtInvokeArgument(), QtInvokeArgument val7 = QtInvokeArgument(),
                            QtInvokeArgument val8 = QtInvokeArgument(),
                            QtInvokeArgument val9 = QtInvokeArgument()) const {
    return QMetaObject::invokeMethod(obj, member, type, val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
  }

  virtual bool invokeMethod(QObject *obj, const char *member, QtInvokeArgument val0 = QtInvokeArgument(),
                            QtInvokeArgument val1 = QtInvokeArgument(), QtInvokeArgument val2 = QtInvokeArgument(),
                            QtInvokeArgument val3 = QtInvokeArgument(), QtInvokeArgument val4 = QtInvokeArgument(),
                            QtInvokeArgument val5 = QtInvokeArgument(), QtInvokeArgument val6 = QtInvokeArgument(),
                            QtInvokeArgument val7 = QtInvokeArgument(), QtInvokeArgument val8 = QtInvokeArgument(),
                            QtInvokeArgument val9 = QtInvokeArgument()) const {
    return QMetaObject::invokeMethod(obj, member, val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
  }
};
} // namespace MantidQt::MantidWidgets
