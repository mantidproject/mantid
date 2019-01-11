// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef RECOVERYFAILUREVIEW_H
#define RECOVERYFAILUREVIEW_H

#include "ProjectRecoveryPresenter.h"
#include "ui_RecoveryFailure.h"
#include <QDialog>
#include <QWidget>
#include <memory>

class RecoveryFailureView : public QDialog {
  Q_OBJECT

public:
  explicit RecoveryFailureView(QWidget *parent = 0,
                               ProjectRecoveryPresenter *presenter = nullptr);
  void reject() override;

  void setProgressBarMaximum(const int newValue);
  void connectProgressBar();
  void emitAbortScript();
  void changeStartMantidButton(const QString &string);

signals:
  void abortProjectRecoveryScript();

public slots:
  void updateProgressBar(const int newValue, const bool err);

private slots:
  void onClickLastCheckpoint();
  void onClickSelectedCheckpoint();
  void onClickOpenSelectedInScriptWindow();
  void onClickStartMantidNormally();

private:
  void addDataToTable();

  std::unique_ptr<Ui::RecoveryFailure> m_ui;
  ProjectRecoveryPresenter *m_presenter;
};

#endif // RECOVERYFAILUREVIEW_H
