#ifndef PROJECTRECOVERYPRESENTER_H
#define PROJECTRECOVERYPRESENTER_H

#include "ProjectRecoveryModel.h"
#include <QStringList>

class ProjectRecoveryPresenter
{
public:
    ProjectRecoveryPresenter();
    int startView(int argc, char *argv[]);
    int startRecoveryFailure(int argc, char *argv[]);
    QStringList getRow (int i);
    void recoverLast();
    void openLastInEditor();
    void startMantidNormally();
    void recoverSelectedCheckpoint(std::string &selected);
    void openSelectedInEditor(std::string &selected);

private:
    ProjectRecoveryModel m_model;
};

#endif // PROJECTRECOVERYPRESENTER_H
