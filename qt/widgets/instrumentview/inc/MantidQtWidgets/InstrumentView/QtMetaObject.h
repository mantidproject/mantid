// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0

#pragma once

#include <QMetaObject>

namespace MantidQt::MantidWidgets {

struct QtMetaObject {
  virtual ~QtMetaObject() = default;

  virtual bool invokeMethod(QObject *obj, const char *member, Qt::ConnectionType type, QGenericReturnArgument ret,
                            QGenericArgument val0 = QGenericArgument(nullptr),
                            QGenericArgument val1 = QGenericArgument(), QGenericArgument val2 = QGenericArgument(),
                            QGenericArgument val3 = QGenericArgument(), QGenericArgument val4 = QGenericArgument(),
                            QGenericArgument val5 = QGenericArgument(), QGenericArgument val6 = QGenericArgument(),
                            QGenericArgument val7 = QGenericArgument(), QGenericArgument val8 = QGenericArgument(),
                            QGenericArgument val9 = QGenericArgument()) const {
    return QMetaObject::invokeMethod(obj, member, type, ret, val0, val1, val2, val3, val4, val5, val6, val7, val8,
                                     val9);
  }

  virtual bool invokeMethod(QObject *obj, const char *member, QGenericReturnArgument ret,
                            QGenericArgument val0 = QGenericArgument(nullptr),
                            QGenericArgument val1 = QGenericArgument(), QGenericArgument val2 = QGenericArgument(),
                            QGenericArgument val3 = QGenericArgument(), QGenericArgument val4 = QGenericArgument(),
                            QGenericArgument val5 = QGenericArgument(), QGenericArgument val6 = QGenericArgument(),
                            QGenericArgument val7 = QGenericArgument(), QGenericArgument val8 = QGenericArgument(),
                            QGenericArgument val9 = QGenericArgument()) const {
    return QMetaObject::invokeMethod(obj, member, ret, val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
  }

  virtual bool invokeMethod(QObject *obj, const char *member, Qt::ConnectionType type,
                            QGenericArgument val0 = QGenericArgument(nullptr),
                            QGenericArgument val1 = QGenericArgument(), QGenericArgument val2 = QGenericArgument(),
                            QGenericArgument val3 = QGenericArgument(), QGenericArgument val4 = QGenericArgument(),
                            QGenericArgument val5 = QGenericArgument(), QGenericArgument val6 = QGenericArgument(),
                            QGenericArgument val7 = QGenericArgument(), QGenericArgument val8 = QGenericArgument(),
                            QGenericArgument val9 = QGenericArgument()) const {
    return QMetaObject::invokeMethod(obj, member, type, val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
  }

  virtual bool invokeMethod(QObject *obj, const char *member, QGenericArgument val0 = QGenericArgument(nullptr),
                            QGenericArgument val1 = QGenericArgument(), QGenericArgument val2 = QGenericArgument(),
                            QGenericArgument val3 = QGenericArgument(), QGenericArgument val4 = QGenericArgument(),
                            QGenericArgument val5 = QGenericArgument(), QGenericArgument val6 = QGenericArgument(),
                            QGenericArgument val7 = QGenericArgument(), QGenericArgument val8 = QGenericArgument(),
                            QGenericArgument val9 = QGenericArgument()) const {
    return QMetaObject::invokeMethod(obj, member, val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
  }
};
} // namespace MantidQt::MantidWidgets
