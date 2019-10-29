// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ALFVIEW_VIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_ALFVIEW_VIEW_H_

#include "DllConfig.h"
#include "MantidQtWidgets/Common/MWRunFiles.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"

#include <QObject>
#include <QSplitter>
#include <QString>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class ALFView_view : public QSplitter {
  Q_OBJECT

public:
  explicit ALFView_view(const std::string &instrument,
                        QWidget *parent = nullptr);
  std::string getFile();
  void setRunQuietly(const std::string &runNumber);
  void observeLoadRun(Observer *listener) {
    m_loadRunObservable->attach(listener);
  };
  void warningBox(const std::string &message);

public slots:
  void fileLoaded();

private:
  QWidget *generateLoadWidget();
  void warningBox(const QString &message);

  Observable *m_loadRunObservable;
  API::MWRunFiles *m_files;
  QString m_instrument;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_ALFVIEW_VIEW_H_ */
