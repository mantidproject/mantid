// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTAPI_PYTHONRUNNER_H_
#define MANTIDQTAPI_PYTHONRUNNER_H_

#include "DllOption.h"
#include <QObject>
#include <QStringList>

/** The ase class from which mantid custom widgets are derived it contains
 *  some useful functions
 */
namespace MantidQt {
namespace API {
/**
This is the base class all customised user interfaces that do not wish to be
tied
to a specific Mantid algorithm but rather customised for user's requirements

@author Steve Williams, Rutherford Appleton Laboratory
@date 10/11/2010
*/
class EXPORT_OPT_MANTIDQT_COMMON PythonRunner : public QObject {
  Q_OBJECT

public:
  /// Default constructor
  PythonRunner() : QObject() {}

  /// Run python code
  QString runPythonCode(const QString &code, bool no_output = false);
  /// Converts a list of strings into a string recognised by Python as a tuple
  static const QString stringList2Tuple(const QStringList &list);
signals:
  void runAsPythonScript(const QString &code, bool /*_t2*/);
};
} // namespace API
} // namespace MantidQt

#endif // MANTIDQTAPI_PYTHONRUNNER_H_
