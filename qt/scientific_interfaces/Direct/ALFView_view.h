// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ALFVIEW_VIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_ALFVIEW_VIEW_H_

#include "DllConfig.h"

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

public slots:
  void runChanged();
  void browse();

signals:
  void newRun();
  void browsedToRun(const std::string &fileName);

private:
  void generateLoadWidget(QWidget *loadBar);
  QLineEdit *m_run;
  QPushButton *m_browse;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_ALFVIEW_VIEW_H_ */
