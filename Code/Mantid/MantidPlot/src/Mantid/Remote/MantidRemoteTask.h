#ifndef MANTIDREMOTETASK_H
#define MANTIDREMOTETASK_H

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
class RemoteTask;

// Note: This supposed to look and feel like the AlgorithmDockWidget.  It doesn't have
// enough in common with it to make inheriting from it useful, though.
class RemoteTaskDockWidget: public QDockWidget
{
    Q_OBJECT
public:
    RemoteTaskDockWidget(MantidUI *mui, ApplicationWindow *w);
    ~RemoteTaskDockWidget();
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
    
    QComboBox *m_clusterCombo;

    QNetworkAccessManager *m_netManager;
    QNetworkReply * m_configReply;
    
    QList <RemoteJobManager *> m_clusterList;  // these are in the same order as they're listed in the combo box

    // Server Attributes. These are specified in the config.xml file for each cluster and
    // are updated when we parse that file.  (ie: every time the user selects a cluster
    // from m_clusterCombo)
    std::string m_outfilePrefix;

    /***************************
    HACK: members I don't think we need now that we're switching to the new job submission design
    //QListWidget *m_taskList;  // Lists the tasks that can be launched on the cluster
    // Maps item pointers from m_taskList to their associated RemoteTask objects
    // QHash <QListWidgetItem *, RemoteTask> m_taskHash;

    // List of all the jobs we've submitted to any cluster
    // HACK: I don't think we need this any more
    //QList <RemoteJob> m_jobList;
    ***********************************************/

    friend class MantidUI;
private:
    MantidUI *m_mantidUI;
    
    static Mantid::Kernel::Logger& logObject;
};




#endif
