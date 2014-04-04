#ifndef MANTIDAPPLICATION_H_
#define MANTIDAPPLICATION_H_

//============================================================
// Inheriting class to make sure we catch any top level errors
//============================================================
#include <QApplication>

class MantidApplication:public QApplication
{
  Q_OBJECT
public:
  MantidApplication(int & argc, char ** argv );
  /// Reimplement notify to catch exceptions from event handlers
  virtual bool notify(QObject * receiver, QEvent * event );
};

#endif // MANTIDAPPLICATION_H_

