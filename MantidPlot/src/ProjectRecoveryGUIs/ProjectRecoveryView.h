// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PROJECTRECOVERYVIEW_H
#define PROJECTRECOVERYVIEW_H

#include "ProjectRecoveryPresenter.h"
#include "ui_ProjectRecoveryWidget.h"
#include <QDialog>
#include <QWidget>
#include <memory>

class ProjectRecoveryView : public QDialog {
  Q_OBJECT

public:
  explicit ProjectRecoveryView(QWidget *parent = 0,
                               ProjectRecoveryPresenter *presenter = nullptr);
  void reject() override;
  void setProgressBarMaximum(int newValue);
  void connectProgressBar();
  void emitAbortScript();
  void changeStartMantidButton(const QString &string);

signals:
  void abortProjectRecoveryScript();

public slots:
  void updateProgressBar(int newValue, bool err);

private slots:
  void onClickLastCheckpoint();
  void onClickOpenLastInScriptWindow();
  void onClickStartMantidNormally();

private:
  void addDataToTable();

  std::unique_ptr<Ui::ProjectRecoveryWidget> m_ui;
  ProjectRecoveryPresenter *m_presenter;
};

#endif // PROJECTRECOVERYVIEW_H
