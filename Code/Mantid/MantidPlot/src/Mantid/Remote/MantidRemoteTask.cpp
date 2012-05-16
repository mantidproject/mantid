#include "MantidRemoteTask.h"
#include "JobStatusDialog.h"
#include "../MantidDock.h"
#include "NewClusterDialog.h"
#include "../MantidUI.h"
#include "RemoteJobManager.h"
#include "RemoteTask.h"
#include "RemoteJob.h"
#include <QtGui>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QDomDocument>
#include <QListWidget>
#include <QListWidgetItem>

#include "../MantidWSIndexDialog.h"
#include "../FlowLayout.h"

#include <map>
#include <vector>
#include <iostream>
#include <sstream>
#include <MantidGeometry/Crystal/OrientedLattice.h>
#include "MantidAPI/ExperimentInfo.h"
#include <iomanip>
#include <cstdio>
#include <Poco/Path.h>
#include <boost/algorithm/string.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;


Mantid::Kernel::Logger& RemoteTaskDockWidget::logObject=Mantid::Kernel::Logger::get("remoteTaskDockWidget");

//----------------- RemoteAlgorithmTaskWidget --------------------//
RemoteTaskDockWidget::RemoteTaskDockWidget(MantidUI *mui, ApplicationWindow *w):
QDockWidget(w),m_mantidUI(mui)
{
    logObject.warning("Inside RemoteAlgorithmTaskWidget constructor");

    setWindowTitle(tr("Remote Tasks"));
    setObjectName("exploreRemoteTasks"); // this is needed for QMainWindow::restoreState()
    setMinimumHeight(150);
    setMinimumWidth(200);
    w->addDockWidget( Qt::RightDockWidgetArea, this );//*/

    QFrame *f = new QFrame(this);
    QLabel *chooseLabel = new QLabel( tr("Choose cluster:"), f);
    m_clusterCombo = new QComboBox( f);
    m_clusterCombo->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    QPushButton *newCluster = new QPushButton( tr("New Cluster"), f);
    QLabel *statusLabel = new QLabel( tr(""), f);  // Status is blank until user chooses a cluster
    m_taskList = new QListWidget();
    m_taskList->setSelectionMode( QAbstractItemView::SingleSelection);
    
    
    QPushButton *submitJob = new QPushButton( tr("Submit Job"), f);
    QPushButton *showJobs= new QPushButton( tr("Show Jobs"), f);
    QVBoxLayout *vbLayout = new QVBoxLayout();
    QHBoxLayout *hbLayout = new QHBoxLayout();
    QHBoxLayout *hbLayoutForButtons = new QHBoxLayout(); 

    hbLayout->addWidget(m_clusterCombo);
    hbLayout->addWidget( newCluster);
    
    hbLayoutForButtons->addWidget( submitJob);
    hbLayoutForButtons->addWidget( showJobs);
    
    vbLayout->addWidget(chooseLabel);
    vbLayout->addLayout(hbLayout);
    vbLayout->addWidget(statusLabel);
    vbLayout->addWidget(m_taskList);
    vbLayout->addLayout(hbLayoutForButtons);
    
    f->setLayout(vbLayout);

    m_taskList->addItem( new QListWidgetItem( tr("Update() hasn't been called yet.")));

    m_netManager = new QNetworkAccessManager();

    QObject::connect( newCluster, SIGNAL( clicked()), this, SLOT( addNewCluster()));
    QObject::connect( submitJob, SIGNAL( clicked()), this, SLOT( submitJob()));
    QObject::connect( showJobs, SIGNAL( clicked()), this, SLOT( showJobs()));
    QObject::connect( m_clusterCombo, SIGNAL( currentIndexChanged(int)), this, SLOT( clusterChoiceChanged(int)));


    // Load the cluster info from the properties files
    Mantid::Kernel::ConfigServiceImpl& config = Mantid::Kernel::ConfigService::Instance();

    int numClusters;
    if ( config.getValue( std::string("Cluster.NumClusters"), numClusters) )
    {
        for (int i = 0; i < numClusters; i++)
        {
            RemoteJobManager * manager = RemoteJobManagerFactory::createFromProperties( i);
            if (manager != NULL)
            {
                m_clusterList.append( manager);
                m_clusterCombo->addItem( QString::fromStdString( manager->getDisplayName()));
            }
        }
    }
    std::ostringstream tempStr;
    tempStr << m_clusterList.size();
    config.setString( std::string("Cluster.NumClusters"), tempStr.str());

    setWidget(f);

}

RemoteTaskDockWidget::~RemoteTaskDockWidget()
{
    // save the cluster info in the combo box to the user config file
    // (Replace the values in the config file with what's in the combo box.)
    Mantid::Kernel::ConfigServiceImpl& config = Mantid::Kernel::ConfigService::Instance();

    std::ostringstream tempStr;
    tempStr << m_clusterList.size();
    config.setString( std::string("Cluster.NumClusters"), tempStr.str());
    for (int i = 0; i < m_clusterList.size(); i++)
    {
        m_clusterList[i]->saveProperties( i);
    }
    config.saveConfig( config.getUserFilename());

    // The cluster list only contains pointers.  We have to delete the objects they point to manually
    QList <RemoteJobManager *>::Iterator it = m_clusterList.begin();
    while (it != m_clusterList.end())
    {
        delete (*it);
        it++;
    }
   delete m_netManager;
}

void RemoteTaskDockWidget::update()
{
    m_taskList->clear();
    m_taskHash.clear();

    if (m_configReply)
    {
        QDomDocument doc("ServerSettings");
        if (!doc.setContent(m_configReply))
        {
            QMessageBox( QMessageBox::Warning, "XML Error", tr("Failed to read XML configuration file."), QMessageBox::Ok).exec();
            return;
        }

        QDomElement root = doc.documentElement();
        if( root.tagName() != "document" )
        {
            QMessageBox( QMessageBox::Warning, "XML Error", tr("Unexpected document root in the XML configuration file."), QMessageBox::Ok).exec();
            return;
        }

        QDomNode n = root.firstChild();
        while( !n.isNull() )
        {
          QDomElement e = n.toElement();
          if (!e.isNull())
          {
            if (e.tagName() == "server_attributes" )
                xmlParseServerAttributes( e);
            else if (e.tagName() == "algorithm")
                xmlParseAlgorithm( e);
            else
            {
                QMessageBox msgBox;
                msgBox.setText("Unrecognized XML Element");
                msgBox.setInformativeText( e.tagName() + QString(tr(" is not a recognized XML element.  It will be ignored.")));
                msgBox.exec();
            }
          }

          n = n.nextSibling();
        }
    }

    // We're done with the network reply (assuming we used it at all), so schedule it for deletion
    if (m_configReply)
    {
      m_configReply->deleteLater();
      m_configReply = NULL;
    }
}

// Shows a dialog box for the user to enter info about a cluster.  Adds that cluster to the
// combo box.
void RemoteTaskDockWidget::addNewCluster()
{
  NewClusterDialog *theDialog = new NewClusterDialog();
  if (theDialog->exec() == QDialog::Accepted)
  {
    // Grab the values the user entered
    // ToDo:  This will need to change if we ever implement any other type of job manager!
    // (Will probably want to use the job manager factory class....)
    QtMwsRemoteJobManager *manager = new  QtMwsRemoteJobManager( theDialog->getDisplayName().toStdString(),
                                                                 theDialog->getConfigFileURL().toString().toStdString(),
                                                                 theDialog->getServiceBaseURL().toString().toStdString(),
                                                                 theDialog->getUserName().toStdString());
    m_clusterList.append( manager);
    
    // Add the Display name to the combo box
    m_clusterCombo->addItem( theDialog->getDisplayName());
  }
  
}

void RemoteTaskDockWidget::clusterChoiceChanged(int index)
{
    // connect to the cluster and download the XML config file
    QNetworkRequest request;
    QUrl configFileUrl( QString::fromStdString(m_clusterList[index]->getConfigFileUrl()));

    request.setUrl( configFileUrl);
    if (request.url().isValid())
    {
        m_configReply = m_netManager->get(request);
        // update() function will parse the downloaded XML file and populate the algorithm tree
        QObject::connect(m_configReply, SIGNAL(finished()), this, SLOT(update()));
    }
    else
    {
        // Testing for a valid URL is done in the dialog box when it was first entered,
        // so in theory, we'll never get here.  But just in case we do (possibly
        // because the URL came from a corrupt properties file?), show an error dialog
        QMessageBox msgBox;
        msgBox.setText("Invalid URL.");
        msgBox.setInformativeText(QString(tr("The URL <")) + request.url().toString() + tr("> is invalid.  This cluster will be ignored."));
        msgBox.exec();
    }
}


// Someone clicked the "Show Jobs" button.  Pop up the dialog.
void RemoteTaskDockWidget::showJobs()
{
    JobStatusDialog *d = new JobStatusDialog;
    QList<RemoteJob>::iterator it = m_jobList.begin();
    while (it != m_jobList.end())
    {
        d->addRow( *it);
        it++;
    }



    d->exec();
}

// Someone clicked the "Submit Job" button.  Pop up a dialog to grab any needed inputs
// then hand everything over to the job manager.
void RemoteTaskDockWidget::submitJob()
{
    QDialog *d = new QDialog;
    
    QListWidgetItem *selectedTask = m_taskList->currentItem();
    d->setWindowTitle( tr("Submit Job: ") + selectedTask->text());
    QDialogButtonBox *bb = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QVBoxLayout *vbLayout = new QVBoxLayout();
    
    QString clusterName = QString::fromStdString( m_clusterList[ m_clusterCombo->currentIndex()]->getDisplayName());
    vbLayout->addWidget( new QLabel( tr( "Submit job to ") + clusterName));
    
    RemoteTask &task = m_taskHash[ selectedTask];
    QList <QLineEdit *> editList;
    if (task.getNumUserSuppliedParams() > 0)
    {
        // Need to add labels and text inputs to the dialog so the user can fill in
        // the necessary parameters.  If the user has already entered values (ie, from
        // a previous job), we'll pre-load the edit boxes with those values.
              
        QFormLayout *form = new QFormLayout;
        for (unsigned i = 0; i < task.getNumUserSuppliedParams(); i++)
        {
            QLabel *label = new QLabel( QString::fromStdString(task.getUserSuppliedParamName( i)));
            QLineEdit *edit = new QLineEdit( QString::fromStdString(task.getUserSuppliedParamValue( i)));
            form->addRow( label, edit);
            editList.append( edit);  // save the pointers so we can access them below
        }
        
        vbLayout->addLayout(form);
    }
    
    vbLayout->addWidget( bb);
    d->setLayout( vbLayout);
    
    QObject::connect( bb, SIGNAL( accepted()), d, SLOT( accept()));
    QObject::connect( bb, SIGNAL( rejected()), d, SLOT( reject()));
    
    
    if (d->exec() == QDialog::Accepted)
    {
        // First off, save the values for any user-specified params
        for (int i=0; i < editList.size(); i++)
        {
            task.setUserSuppliedParamValue( i, editList[i]->text().toStdString());
        }

        std::string jobId;
        if ( m_clusterList[ m_clusterCombo->currentIndex()]->submitJob( task, jobId) == true)
        {
            // job successfully submitted - save the job ID and pop up a message box saying
            // everything worked
            RemoteJob theJob( jobId, m_clusterList[ m_clusterCombo->currentIndex()], RemoteJob::JOB_STATUS_UNKNOWN, task.getName());
            m_jobList.append( theJob);

            QMessageBox msgBox;
            msgBox.setText(tr("Job submission successful."));
            msgBox.setInformativeText( QString(tr( "Job ID: ")) + QString::fromStdString(jobId));
            msgBox.exec();
        }
        else
        {
            // D'oh!  There was some kind of error submitting the job.  The jobId string
            // should have some kind of explanation.  Display it in a dialog box.
            QMessageBox msgBox;
            msgBox.setText(tr("Job submission failed."));
            msgBox.setInformativeText( QString::fromStdString(jobId));
            msgBox.exec();
        }

        // TODO: We need to save the job ID somewhere so we can display status info about it....
    }
        
    delete d;
}

void RemoteTaskDockWidget::xmlParseServerAttributes( QDomElement & /* elm */)
{
    // We don't actually do anything with the server attributes yet...
    return;
}


void RemoteTaskDockWidget::xmlParseAlgorithm( QDomElement &elm)
{

    RemoteTask task;
    QString taskDisplayName;  // Shows up in the GUI
    
    QDomNode n = elm.firstChild();
    while (!n.isNull())
    {
      QDomElement e = n.toElement();
      if (!e.isNull())
      {
        if (e.tagName() == "name" )
        {
            taskDisplayName = e.text();
            task.setName( taskDisplayName.toStdString());
        }
        
        else if (e.tagName() == "executable")
        {
            task.setExecutable(e.text().toStdString());
        }
        
        else if (e.tagName() == "parameter_list")
        {
            QDomNode n2 = e.firstChild();
            while (!n2.isNull())
            {
                QDomElement e2 = n2.toElement();
                if (!e2.isNull())
                {
                    if (e2.tagName() == "parameter")
                    {
                        task.appendCmdLineParam(e2.text().toStdString());
                    }
                    else
                    {
                        QMessageBox msgBox;
                        msgBox.setText("Unrecognized XML Element");
                        msgBox.setInformativeText( e2.tagName() + QString(tr(" is not a recognized XML element in <")) 
                                                    + e.tagName() + QString( tr(".  It will be ignored.")));
                        msgBox.exec();
                    }
                }
                
                n2 = n2.nextSibling();
            }
        }

        else if (e.tagName() == "user_parameter_list")
        {
            QDomNode n2 = e.firstChild();
            while (!n2.isNull())
            {
                QDomElement e2 = n2.toElement();
                if (!e2.isNull())
                {
                    if (e2.tagName() == "parameter")
                    {
                        if (e2.hasAttribute( "name") && e2.hasAttribute( "id"))
                        {
                            QString name = e2.attribute( "name");
                            QString id = e2.attribute( "id");
                            task.appendUserSuppliedParam( name.toStdString(), id.toStdString());
                        }
                        else
                        {
                            QMessageBox msgBox;
                            msgBox.setText("Invalid User Parameter");
                            msgBox.setInformativeText( e2.tagName() + QString(tr(" tags must contain 'name' and 'id' attributes.")));
                            msgBox.exec();
                        }
                    }
                    else
                    {
                        QMessageBox msgBox;
                        msgBox.setText("Unrecognized XML Element");
                        msgBox.setInformativeText( e2.tagName() + QString(tr(" is not a recognized XML element in <"))
                                                    + e.tagName() + QString( tr(">.  It will be ignored.")));
                        msgBox.exec();
                    }
                }
                
                n2 = n2.nextSibling();
            }
            
        }
        
        else if (e.tagName() == "resource_list")
        {
            QDomNode n2 = e.firstChild();
            while (!n2.isNull())
            {
                QDomElement e2 = n2.toElement();
                if (!e2.isNull())
                {
                    if (e2.tagName() == "resource")
                    {
                        if (e2.hasAttribute( "name") && e2.hasAttribute( "value"))
                        {
                            QString name = e2.attribute( "name");
                            QString value = e2.attribute( "value");
                            task.appendResource( name.toStdString(), value.toStdString());
                        }
                        else
                        {
                            QMessageBox msgBox;
                            msgBox.setText("Invalid Resource");
                            msgBox.setInformativeText( e2.tagName() + QString(tr(" tags must contain 'name' and 'value' attributes.")));
                            msgBox.exec();
                        }
                    }
                    else
                    {
                        QMessageBox msgBox;
                        msgBox.setText("Unrecognized XML Element");
                        msgBox.setInformativeText( e2.tagName() + QString(tr(" is not a recognized XML element in <"))
                                                    + e.tagName() + QString( tr(">.  It will be ignored.")));
                        msgBox.exec();
                    }
                }
                
                n2 = n2.nextSibling();
            }
            
        }
        
        else
        {
            QMessageBox msgBox;
            msgBox.setText("Unrecognized XML Element");
            msgBox.setInformativeText( e.tagName() + QString(tr(" is not a recognized XML element.  It will be ignored.")));
            msgBox.exec();
        }
      }

      n = n.nextSibling();
    }

    // Add to the view and the task hash table
    if (task.isValid())
    {
        QListWidgetItem *taskItem;
        taskItem = new QListWidgetItem( taskDisplayName);  // algDisplayName is set at the same time as alg::m_name, so if alg is valid, so is algDisplayName...
        m_taskList->addItem( taskItem);
        m_taskHash[taskItem] = task;
        
        if (m_taskList->count() == 1)
        {
            // If this is the first item to be added, select it (thus ensuring
            // that there's always a selected item).
            m_taskList->setCurrentItem( taskItem);
        }
    }
}

