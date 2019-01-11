// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDAPPLICATION_H_
#define MANTIDAPPLICATION_H_

//============================================================
// Inheriting class to make sure we catch any top level errors
//============================================================
#include <QApplication>

class MantidApplication : public QApplication {
  Q_OBJECT
public:
  MantidApplication(int &argc, char **argv);
  /// Reimplement notify to catch exceptions from event handlers
  bool notify(QObject *receiver, QEvent *event) override;
signals:
  bool runAsPythonScript(const QString &code);
public slots:
  void errorHandling(bool continueWork, int sharing, QString name,
                     QString email, QString textbox);
};

#endif // MANTIDAPPLICATION_H_
