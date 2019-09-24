// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ALFVIEW_VIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_ALFVIEW_VIEW_H_

#include "DllConfig.h"
#include "observerPattern.h"

#include <QLineEdit>
#include <QObject>
#include <QPushButton>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class ALFView_view : public QWidget {
  Q_OBJECT

public:
  ALFView_view(QWidget *parent = nullptr);
  int getRunNumber();
  void setRunQuietly(const QString runNumber);
  void observeLoadRun(observer *listener) {
    m_loadRunObservable->attach(listener);
  };
  void observeBrowse(observer *listner) {
    m_browseObservable->attach(listner);
  };

public slots:
  void runChanged();
  void browse();

signals:
  void newRun();
  void browsedToRun(std::string);

private:
  void generateLoadWidget(QWidget *loadBar);
  QLineEdit *m_run;
  QPushButton *m_browse;
  observable *m_loadRunObservable;
  observable *m_browseObservable;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_ALFVIEW_VIEW_H_ */
