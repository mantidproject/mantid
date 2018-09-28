#ifndef PROJECTRECOVERYVIEW_H
#define PROJECTRECOVERYVIEW_H

#include "ProjectRecoveryPresenter.h"
#include <QWidget>
#include <QDialog>

namespace Ui {
class ProjectRecoveryWidget;
}

class ProjectRecoveryView : public QDialog {
  Q_OBJECT

public:
  explicit ProjectRecoveryView(
      QWidget *parent = 0,
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
