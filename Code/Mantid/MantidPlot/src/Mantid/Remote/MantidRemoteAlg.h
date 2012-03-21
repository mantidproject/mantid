#ifndef MANTIDREMOTEALG_H
#define MANTIDREMOTEALG_H

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include <QComboBox>
#include <QDockWidget>
#include <QPoint>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVector>
#include <QList>
#include <QHash>
#include <QActionGroup>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QUrl>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <set>

class MantidUI;
class ApplicationWindow;
class MantidTreeWidgetItem;
class MantidTreeWidget;
class QDomElement;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QMenu;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QProgressBar;
class QVBoxLayout;
class QHBoxLayout;
class QSignalMapper;
class QSortFilterProxyModel;

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
    void findAlgTextChanged(const QString& text);
    void treeSelectionChanged();
    void selectionChanged(const QString& algName);
//    void updateProgress(void* alg, const double p, const QString& msg, double estimatedTime, int progressPrecision);
    void algorithmStarted(void* alg);
    void algorithmFinished(void* alg);
    void addNewCluster();
    void clusterChoiceChanged(int index);
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
    
    // Maps item pointers from m_tree to their associated RemoteAlg objects
    QHash <QListWidgetItem *, RemoteAlg> m_algorithmHash;  

    friend class MantidUI;
private:
    MantidUI *m_mantidUI;
    
    static Mantid::Kernel::Logger& logObject;
};




#endif
