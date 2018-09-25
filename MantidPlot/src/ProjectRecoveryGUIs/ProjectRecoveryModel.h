#ifndef PROJECTRECOVERYMODEL_H
#define PROJECTRECOVERYMODEL_H

#include <vector>
#include <string>

class ProjectRecoveryModel
{
public:
    ProjectRecoveryModel();
    std::vector<std::string> getRow(int i);
    void recoverLast();
    void openLastInEditor();
    void startMantidNormally();
    void recoverSelectedCheckpoint(std::string &selected);
    void openSelectedInEditor(std::string &selected);

private:
    //ProjectRecovery *m_projRec
};

#endif // PROJECTRECOVERYMODEL_H
