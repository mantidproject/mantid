#ifndef MANTIDREMOTEALG_H
#define MANTIDREMOTEALG_H

//#include "MantidAPI/ExperimentInfo.h"
//#include "MantidAPI/IMDEventWorkspace.h"
//#include "MantidAPI/IMDWorkspace.h"
//#include "MantidAPI/IPeaksWorkspace.h"
//#include "MantidAPI/ITableWorkspace.h"
//#include "MantidAPI/MatrixWorkspace.h"
//#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/Logger.h"

#include "RemoteJob.h"
#include <QDockWidget>
#include <QList>
#include <QHash>

class MantidUI;
class ApplicationWindow;
class QComboBox;
class QDomElement;
class QListWidget;
class QListWidgetItem;
class QNetworkAccessManager;
class QNetworkReply;

class RemoteJobManager;
class RemoteAlg;

// Note: This is based closely on the AlgorithmDockWidget.  It might be
// better to have it actually inherit from that instead of QDockWidget...
class RemoteAlgorithmDockWidget: public QDockWidget
{
    Q_OBJECT
public:
    RemoteAlgorithmDockWidget(MantidUI *mui, ApplicationWindow *w);
    ~RemoteAlgorithmDockWidget();
public slots:
    void update();
    void addNewCluster();
    void clusterChoiceChanged(int index);
    void showJobs();
    void submitJob();
    
protected:
//    void showProgressBar();
    //void hideProgressBar();
    

    void xmlParseServerAttributes( QDomElement &elm);
    void xmlParseAlgorithm( QDomElement &elm);
    
    QComboBox *m_clusterCombo;
    QListWidget *m_algList;

    QNetworkAccessManager *m_netManager;
    QNetworkReply * m_configReply;
    
    QList <RemoteJobManager *> m_clusterList;  // these are in the same order as they're listed in the combo box
    
    // Maps item pointers from m_algList to their associated RemoteAlg objects
    QHash <QListWidgetItem *, RemoteAlg> m_algorithmHash;

    // List of all the jobs we've submitted to any cluster
    QList <RemoteJob> m_jobList;

    friend class MantidUI;
private:
    MantidUI *m_mantidUI;
    
    static Mantid::Kernel::Logger& logObject;
};




#endif
