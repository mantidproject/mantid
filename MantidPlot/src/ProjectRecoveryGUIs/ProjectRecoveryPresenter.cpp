#include "ProjectRecoveryPresenter.h"
#include "ProjectRecoveryView.h"
#include <QApplication>
#include "ProjectRecoveryModel.h"
#include "RecoveryFailureView.h"

ProjectRecoveryPresenter::ProjectRecoveryPresenter()
{

}

int ProjectRecoveryPresenter::startView(int argc, char *argv[]){
    QApplication a(argc, argv);
    ProjectRecoveryView w(0, this);
    w.show();

    return a.exec();
}

int ProjectRecoveryPresenter::startRecoveryFailure(int argc, char *argv[]){
    QApplication a(argc, argv);
    RecoveryFailureView w(0, this);
    w.show();

    return a.exec();
}

QStringList ProjectRecoveryPresenter::getRow(int i){
    auto vec = m_model.getRow(i);
    QStringList returnVal;
    for (auto i = 0; i < 3; ++i){
        QString newString = QString::fromStdString(vec[i]);
        returnVal << newString;
    }
    return returnVal;
}

void ProjectRecoveryPresenter::recoverLast(){
    m_model.recoverLast();
}

void ProjectRecoveryPresenter::openLastInEditor(){
    m_model.openLastInEditor();
}

void ProjectRecoveryPresenter::startMantidNormally(){
    m_model.startMantidNormally();
}

void ProjectRecoveryPresenter::recoverSelectedCheckpoint(std::string &selected){
    m_model.recoverSelectedCheckpoint(selected);
}

void ProjectRecoveryPresenter::openSelectedInEditor(std::string &selected){
    m_model.openSelectedInEditor(selected);
}
