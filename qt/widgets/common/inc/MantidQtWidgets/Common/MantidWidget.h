// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTAPI_MANTIDWIDGET_H_
#define MANTIDQTAPI_MANTIDWIDGET_H_

#include "DllOption.h"
#include "MantidQtWidgets/Common/PythonRunner.h"
#include <QVariant>
#include <QWidget>

namespace MantidQt {
namespace API {
/**
This is the base class all customised widgets that do not wish to be tied
to a specific Mantid algorithm but rather customised for user's requirements.

The virtual function getUserInput() must be implemented to return what the
widget considers
as user input.

@author Martyn Gigg, Tessella Support Services plc
@date 18/03/2009
*/
class EXPORT_OPT_MANTIDQT_COMMON MantidWidget : public QWidget {
  Q_OBJECT

public:
  /// Returns a QVariant containing what the widget classes as user input so
  /// that
  /// input can be returned through a common interface.
  virtual QVariant getUserInput() const { return QVariant(); }
  /**
   * Sets a value on a mantid widget through a common interface
   * @param value :: The value as a QVariant
   */
  virtual void setUserInput(const QVariant &value) { Q_UNUSED(value); }

signals:
  void runAsPythonScript(const QString &code, bool /*_t2*/);

protected:
  /// Default constructor
  MantidWidget(QWidget *parent = nullptr);
  /// Run python code that is passed to it and, optionally, return
  /// anything it wrote to standard output as a string
  QString runPythonCode(const QString &code, bool no_output = false);

private:
  /// This object implements the runPythonCode() function, by emitting the code
  /// as a runAsPythonScript signal
  PythonRunner m_pyRunner;
};
} // namespace API
} // namespace MantidQt

#endif // MANTIDQTAPI_MANTIDWIDGET_H_
