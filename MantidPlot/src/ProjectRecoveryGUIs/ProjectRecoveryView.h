// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PROJECTRECOVERYVIEW_H
#define PROJECTRECOVERYVIEW_H

#include "ProjectRecoveryPresenter.h"
#include <QDialog>
#include <QWidget>

namespace Ui {
class ProjectRecoveryWidget;
}

class ProjectRecoveryView : public QDialog {
  Q_OBJECT

public:
  explicit ProjectRecoveryView(QWidget *parent = 0,
                               ProjectRecoveryPresenter *presenter = nullptr);
  ~ProjectRecoveryView();
  void reject() override;

private slots:
  void onClickLastCheckpoint();
  void onClickOpenLastInScriptWindow();
  void onClickStartMantidNormally();

private:
  void addDataToTable(Ui::ProjectRecoveryWidget *ui);
  Ui::ProjectRecoveryWidget *ui;
  ProjectRecoveryPresenter *m_presenter;
};

#endif // PROJECTRECOVERYVIEW_H
