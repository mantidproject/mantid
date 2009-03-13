#ifndef MANTIDAPPLICATION_H_
#define MANTIDAPPLICATION_H_

//============================================================
// Inheriting class to make sure we catch any top level errors
//============================================================
#include <QApplication>

#include "MantidKernel/Logger.h"

class MantidApplication:public QApplication
{
  Q_OBJECT
public:
  MantidApplication(int & argc, char ** argv );
  /// Reimplement notify to catch exceptions from event handlers
  virtual bool notify(QObject * receiver, QEvent * event );
private:
  /// Static reference to the logger class
  static Mantid::Kernel::Logger& g_log;
};

#endif // MANTIDAPPLICATION_H_

