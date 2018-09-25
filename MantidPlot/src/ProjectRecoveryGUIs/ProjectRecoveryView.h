#ifndef PROJECTRECOVERYVIEW_H
#define PROJECTRECOVERYVIEW_H

#include <QWidget>
#include "ProjectRecoveryPresenter.h"

namespace Ui {
class ProjectRecoveryWidget;
}

class ProjectRecoveryView : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectRecoveryView(QWidget *parent = 0, ProjectRecoveryPresenter *presenter = new ProjectRecoveryPresenter);
    ~ProjectRecoveryView();

private slots:
    void onClickLastCheckpoint();
    void onClickOpenLastInScriptWindow();
    void onClickStartMantidNormally();

private:
    Ui::ProjectRecoveryWidget *ui;
    ProjectRecoveryPresenter *m_presenter;
};

#endif // PROJECTRECOVERYVIEW_H
