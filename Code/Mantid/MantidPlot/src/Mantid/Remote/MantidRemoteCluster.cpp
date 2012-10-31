#include "MantidRemoteCluster.h"
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


Mantid::Kernel::Logger& RemoteClusterDockWidget::logObject=Mantid::Kernel::Logger::get("remoteClusterDockWidget");

//----------------- RemoteClusterDockWidget --------------------//
RemoteClusterDockWidget::RemoteClusterDockWidget(MantidUI *mui, ApplicationWindow *w):
QDockWidget(w),m_mantidUI(mui)
{
    logObject.warning("Inside RemoteTaskDockWidget constructor");

    setWindowTitle(tr("Remote Clusters"));
    setObjectName("exploreRemoteTasks"); // this is needed for QMainWindow::restoreState()
    setMinimumHeight(150);
    setMinimumWidth(200);
    w->addDockWidget( Qt::RightDockWidgetArea, this );//*/

    QFrame *f = new QFrame(this);
    QLabel *clusterLabel = new QLabel( tr("Known clusters:"), f);
    m_clusterCombo = new QComboBox( f);
    m_clusterCombo->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    QPushButton *newCluster = new QPushButton( tr("New Cluster"), f);       
    
    QPushButton *showJobs= new QPushButton( tr("Show Jobs"), f);
    QVBoxLayout *vbLayout = new QVBoxLayout();
    QHBoxLayout *hbLayout = new QHBoxLayout();

    hbLayout->addWidget(m_clusterCombo);
    hbLayout->addWidget( newCluster);  

    vbLayout->addWidget(clusterLabel);
    vbLayout->addLayout(hbLayout);
    vbLayout->addWidget( showJobs);

    f->setLayout(vbLayout);

    m_netManager = new QNetworkAccessManager();

    QObject::connect( newCluster, SIGNAL( clicked()), this, SLOT( addNewCluster()));
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

RemoteClusterDockWidget::~RemoteClusterDockWidget()
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

void RemoteClusterDockWidget::update()
{

    if (m_configReply)
    {
        QDomDocument doc("ServerSettings");
        QString parseErrMsg;
        int errLine = 0;
        int errCol = 0;
        if (!doc.setContent(m_configReply, &parseErrMsg, &errLine, &errCol))
        {
            QString mbText = QString( tr("Failed to parse XML configuration file.")) + QString("\n") +
                             QString( tr("Error type: ")) + parseErrMsg + QString("\n") +
                             QString( tr("Line: ")) + QString::number(errLine) + QString("\n") +
                             QString( tr("Col: ")) + QString::number(errCol);
            QMessageBox ( QMessageBox::Warning, "XML Error", mbText, QMessageBox::Ok).exec();
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
            else if (e.tagName() == "task")
            {
              // HACK!  For now, do nothing.  The new style config files don't explicitly list every
              // task, but until I get rid of the config files, I don't want us throwing parse errors.
              // Once all the config files are converted to the new style (which implies that we've
              // actually decided what that new style is....), we need to delete this else if test.
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
void RemoteClusterDockWidget::addNewCluster()
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

void RemoteClusterDockWidget::clusterChoiceChanged(int index)
{
    // connect to the cluster and download the XML config file
    QNetworkRequest request;
    QUrl configFileUrl( QString::fromStdString(m_clusterList[index]->getConfigFileUrl()));

    request.setUrl( configFileUrl);
    if (request.url().isValid())
    {
        m_configReply = m_netManager->get(request);
        // update() function will parse the downloaded XML file and populate the task list
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
void RemoteClusterDockWidget::showJobs()
{
  JobStatusDialog jsd( m_clusterList[ m_clusterCombo->currentIndex()], m_mantidUI);
  if (jsd.readyToDisplay())
  {
    // If there was an error creating the dialog box (couldn't connect to the server,
    // for example) there's no point in displaying the box.
    jsd.exec();
  }

}

// HACK!  Job submission is changing radically.  For now, I'm replacing
// the submitJob function with a no-op just so I can get things compiling.
// Very shortly, I expect all of this will be deleted (with job submission
// moved into the algorithm class or similar...)
void RemoteClusterDockWidget::submitJob() { return; }
/***********************************************************
// Someone clicked the "Submit Job" button.  Pop up a dialog to grab any needed inputs
// then hand everything over to the job manager.
void RemoteClusterDockWidget::submitJob()
{
    QDialog *d = new QDialog;
    
    QListWidgetItem *selectedTask = m_taskList->currentItem();
    d->setWindowTitle( tr("Submit Job: ") + selectedTask->text());
    QDialogButtonBox *bb = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QVBoxLayout *vbLayout = new QVBoxLayout();
    
    QString clusterName = QString::fromStdString( m_clusterList[ m_clusterCombo->currentIndex()]->getDisplayName());
    vbLayout->addWidget( new QLabel( tr( "Submit job to ") + clusterName));
    
    RemoteTask &task = m_taskHash[ selectedTask];
    QList <QWidget *> widgetList;
    if (task.getNumSubstitutionParams() > 0)
    {
        // Need to add labels and text inputs to the dialog so the user can fill in
        // the necessary parameters.  If the user has already entered values (ie, from
        // a previous job), we'll pre-load the edit boxes with those values.
              
        QFormLayout *form = new QFormLayout;
        for (unsigned i = 0; i < task.getNumSubstitutionParams(); i++)
        {
            if (task.getSubstitutionParamName(i).size() > 0)
            {
                QLabel *label = new QLabel( QString::fromStdString(task.getSubstitutionParamName( i)));
                QComboBox *combo;
                QLineEdit *edit;
                QCheckBox *checkbox;
                std::string choices;
                size_t startPos = 0;
                size_t endPos = 0;
                switch (task.getSubstitutionParamType(i))
                {
                    case RemoteTask::TEXT_BOX:
                        edit = new QLineEdit( QString::fromStdString(task.getSubstitutionParamValue( i)));
                        form->addRow( label, edit);
                        widgetList.append( edit);  // save the pointers so we can access them below
                        break;

                    case RemoteTask::CHOICE_BOX:
                        combo = new QComboBox();
                        choices = task.getSubstitutionChoiceString(i);
                        // choices is a semi-colon delimited string.  parse it and add all the
                        // choices to the combo box
                        startPos = 0;
                        endPos = 0;
                        do
                        {
                            endPos = choices.find( ';', startPos);
                            combo->addItem( QString::fromStdString(choices.substr(startPos, (endPos-startPos))));
                            startPos = endPos + 1;
                        } while (endPos != std::string::npos);
                        form->addRow( label, combo);
                        widgetList.append( combo);  // save the pointers so we can access them below
                        break;

                    case RemoteTask::CHECK_BOX:
                        checkbox = new QCheckBox( );
                        if ( task.getSubstitutionParamValue( i) == "TRUE")
                        {
                          checkbox->setChecked( true);
                        }
                        else
                        {
                          checkbox->setChecked( false);
                        }

                        form->addRow( label, checkbox);
                        widgetList.append( checkbox);  // save the pointers so we can access them below
                        break;

                    case RemoteTask::UNKNOWN_TYPE:
                    default:
                        // Should never happen....
                        logObject.error("Skipping user parameter of unknown type.  Check cluster's XML config!");
                }
            }
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
        for (int i=0; i < widgetList.size(); i++)
        {
            QComboBox *combo = NULL;
            QLineEdit *edit = NULL;
            QCheckBox *checkbox = NULL;
            QWidget *widget = widgetList[i];


            if ( (edit = dynamic_cast<QLineEdit *>(widget)) )
            {
                task.setSubstitutionParamValue( i, edit->text().toStdString());
            }
            else if ( (combo = dynamic_cast<QComboBox *>(widget)) )
            {
                task.setSubstitutionParamValue(i, combo->currentText().toStdString());
            }
            else if ( (checkbox = dynamic_cast<QCheckBox *>(widget)) )
            {
                if (checkbox->isChecked())
                {
                    task.setSubstitutionParamValue(i, "TRUE");
                }
                else
                {
                    task.setSubstitutionParamValue(i, "FALSE");
                }
            }
        }

        // Next, generate a value for the 'outfile' parameter.  We'll base it on the
        // algorithm name and the current time so that it's unique.  The user should never
        // actually see this file, so we don't need to make the string particularly friendly
        // or convenient to deal with.  An admin might see it, though, so it ought to be
        // pretty obvious what it is.
        std::string outfile = m_outfilePrefix + task.getName() + "_" + Mantid::Kernel::DateAndTime::getCurrentTime().toFormattedString( "%Y-%b-%d_%H-%M-%S");
        task.setSubstitutionParamValue("outfile", outfile);

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
***********************************************************/


void RemoteClusterDockWidget::xmlParseServerAttributes( QDomElement &elm)
{
    // At the moment, the only tag we recognize is the outfile prefix

  QDomNode n = elm.firstChild();
  while (!n.isNull())
  {
    QDomElement e = n.toElement();
    if (!e.isNull())
    {
      if (e.tagName() == "outfile_prefix")
      {
        m_outfilePrefix = e.text().toStdString();
      }
    }
    n = n.nextSibling();
  }


    return;
}

/******************************
  This function is now obsolete and will be deleted once I'm sure there's
  nothing in it I want to save

void RemoteClusterDockWidget::xmlParseTask( QDomElement &elm)
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
                        QString name;
                        QString id;
                        RemoteTask::ParamType type = RemoteTask::TEXT_BOX;  // text box is the default
                        QString choiceString;  // default choice string is empty
                        if (e2.hasAttribute( "name"))
                        {
                            name = e2.attribute( "name");
                        }

                        if (e2.hasAttribute( "id"))
                        {
                            id = e2.attribute( "id");
                        }

                        if (e2.hasAttribute( "type"))
                        {
                            QString idString = e2.attribute( "type");
                            idString.toLower();  // we'll be nice and make the types case insensitive
                            if (idString == "text")
                            {
                                type = RemoteTask::TEXT_BOX;
                            }
                            else if (idString == "choice")
                            {
                                type = RemoteTask::CHOICE_BOX;
                            }
                            else if (idString == "boolean")
                            {
                                type = RemoteTask::CHECK_BOX;
                            }
                            else
                            {
                                type = RemoteTask::UNKNOWN_TYPE;
                            }
                        }

                        if (e2.hasAttribute( "choices"))
                        {
                            choiceString = e2.attribute( "choices");
                        }

                        // Now some error & sanity checks...
                        // name and id are required
                        if ( ! e2.hasAttribute( "name") || !e2.hasAttribute( "id"))
                        {
                            QMessageBox msgBox;
                            msgBox.setText("Invalid User Parameter");
                            msgBox.setInformativeText( e2.tagName() + QString(tr(" tags must contain 'name' and 'id' attributes.")));
                            msgBox.exec();
                        }
                        // type must be recognized
                        else if (type == RemoteTask::UNKNOWN_TYPE)
                        {
                            QMessageBox msgBox;
                            msgBox.setText("Unrecognized User Parameter Type");
                            msgBox.setInformativeText( QString( "'") + e2.attribute("type") + QString(tr("' is not a recognized parameter type.")));
                            msgBox.exec();
                        }
                        // if type is type CHOICE_BOX, then we need a choice string
                        else if (type == RemoteTask::CHOICE_BOX && choiceString.size() == 0)
                        {
                            QMessageBox msgBox;
                            msgBox.setText("Unrecognized User Parameter Type");
                            msgBox.setInformativeText( QString(tr("Parameter of type 'choice' has no choices specified.")));
                            msgBox.exec();
                        }
                        // OK, it passed its sanity checks - add it
                        else
                        {
                            task.appendSubstitutionParam( name.toStdString(), id.toStdString(), type, choiceString.toStdString());
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
        // evey task needs a substition parameter called 'outfile'.  This parameter's value is set automatically
        // just before a job is submitted (see the comments in submitJob() above).  It also doesn't have a name,
        // which keeps it from being displayed in the dialog box with the user-selectable parameters
        task.appendSubstitutionParam("", "outfile");

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
******************************/


