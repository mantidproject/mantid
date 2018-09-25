#ifndef RECOVERYFAILUREVIEW_H
#define RECOVERYFAILUREVIEW_H

#include <QWidget>
#include "ProjectRecoveryPresenter.h"

namespace Ui {
class RecoveryFailure;
}

class RecoveryFailureView : public QWidget
{
    Q_OBJECT

public:
    explicit RecoveryFailureView(QWidget *parent = 0, ProjectRecoveryPresenter *presenter = new ProjectRecoveryPresenter);
    ~RecoveryFailureView();

private slots:
    void onClickLastCheckpoint();
    void onClickSelectedCheckpoint();
    void onClickOpenSelectedInScriptWindow();
    void onClickStartMantidNormally();

private:
    void addDataToTable(Ui::RecoveryFailure *ui);

    Ui::RecoveryFailure *ui;
    ProjectRecoveryPresenter *m_presenter;
};

#endif // RECOVERYFAILUREVIEW_H
