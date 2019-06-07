// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_API_MANTIDDIALOG_H_
#define MANTIDQT_API_MANTIDDIALOG_H_

//----------------------------------
// Includes
//----------------------------------
#include "DllOption.h"
#include "MantidQtWidgets/Common/PythonRunner.h"
#include <QDialog>
#include <boost/shared_ptr.hpp>

//----------------------------------
// Qt Forward declarations
//----------------------------------

// Top-level namespace for this library
namespace MantidQt {

namespace API {

/**
    Dialog derived from this class can capture and handle exceptions raised in
    its event handlers. To be able to do this override QAplication::notify
   method:

        bool MyApplication::notify( QObject * receiver, QEvent * event )
        {
            bool res = false;
            try
            {
                res = QApplication::notify(receiver,event);
            }
            catch(std::exception& e)
            {
                if (MantidQt::API::MantidDialog::handle(receiver,e))
                    return true; // stops event propagation
                else
                   // do somethig else ...
            }

            return res;
        }



    @author Roman Tolchenov, Tessella plc
    @date 24/04/2009
*/
class EXPORT_OPT_MANTIDQT_COMMON MantidDialog : public QDialog {

  Q_OBJECT

public:
  /// DefaultConstructor
  MantidDialog(QWidget *parent = nullptr);
  /// Destructor
  ~MantidDialog() override;

  /// Handles the exception caught in an event handler.
  static bool handle(QObject *receiver, const std::exception &e);

signals:
  void runAsPythonScript(const QString &code, bool /*_t2*/);

protected:
  /// Run python code that is passed to it and, optionally, return anything it
  /// wrote to standard output as a string
  QString runPythonCode(const QString &code, bool no_output = false);

  /// Override this method to handle an exception in a derived class.
  virtual void handleException(const std::exception &e);

private:
  /// This object implements the runPythonCode() function, by emitting the code
  /// as a runAsPythonScript signal
  PythonRunner m_pyRunner;
};
} // namespace API
} // namespace MantidQt

#endif // MANTIDQT_API_MANTIDDIALOG_H_
