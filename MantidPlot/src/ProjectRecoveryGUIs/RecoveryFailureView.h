// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef RECOVERYFAILUREVIEW_H
#define RECOVERYFAILUREVIEW_H

#include "ProjectRecoveryPresenter.h"
#include <QDialog>
#include <QWidget>

namespace Ui {
class RecoveryFailure;
}

class RecoveryFailureView : public QDialog {
  Q_OBJECT

public:
  explicit RecoveryFailureView(QWidget *parent = 0,
                               ProjectRecoveryPresenter *presenter = nullptr);
  ~RecoveryFailureView();
  void reject() override;
  void updateProgressBar(int newValue);
  void setProgressBarMaximum(int newValue);

private slots:
  void onClickLastCheckpoint();
  void onClickSelectedCheckpoint();
  void onClickOpenSelectedInScriptWindow();
  void onClickStartMantidNormally();

private:
  void addDataToTable(Ui::RecoveryFailure *ui);

  unsigned int m_progressBarCounter;
  Ui::RecoveryFailure *ui;
  ProjectRecoveryPresenter *m_presenter;
};

#endif // RECOVERYFAILUREVIEW_H
