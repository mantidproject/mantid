#include "MantidRemoteAlg.h"
#include "../MantidDock.h"
#include "NewClusterDialog.h"
#include "../MantidUI.h"
#include "RemoteJobManager.h"
#include <QtGui>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QDomDocument>

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


Mantid::Kernel::Logger& RemoteAlgorithmDockWidget::logObject=Mantid::Kernel::Logger::get("remoteAlgorithmDockWidget");

//----------------- RemoteAlgorithmDockWidget --------------------//
RemoteAlgorithmDockWidget::RemoteAlgorithmDockWidget(MantidUI *mui, ApplicationWindow *w):
QDockWidget(w),m_progressBar(NULL),m_mantidUI(mui)
{
    logObject.warning("Inside RemoteAlgorithmDockWidget constructor");

    setWindowTitle(tr("Remote Algorithms"));
    setObjectName("exploreRemoteAlgorithms"); // this is needed for QMainWindow::restoreState()
    setMinimumHeight(150);
    setMinimumWidth(200);
    w->addDockWidget( Qt::RightDockWidgetArea, this );//*/

    QFrame *f = new QFrame(this);
    QLabel *chooseLabel = new QLabel( tr("Choose cluster:"), f);
    m_clusterCombo = new QComboBox( f);
    m_clusterCombo->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    QPushButton *newCluster = new QPushButton( tr("New Cluster"), f);
    QLabel *statusLabel = new QLabel( tr(""), f);  // Status is blank until user chooses a cluster
    m_tree = new MantidTreeWidget(f,mui);
    m_tree->setHeaderLabel("Remote Algorithms");
    QVBoxLayout *vbLayout = new QVBoxLayout();
    QHBoxLayout *hbLayout = new QHBoxLayout();

    hbLayout->addWidget(m_clusterCombo);
    hbLayout->addWidget( newCluster);
    vbLayout->addWidget(chooseLabel);
    vbLayout->addLayout(hbLayout);
    vbLayout->addWidget(statusLabel);
    vbLayout->addWidget(m_tree);
    f->setLayout(vbLayout);


    QTreeWidgetItem *catItem = new QTreeWidgetItem(QStringList("Update() hasn't been called yet."));
    m_tree->addTopLevelItem(catItem);

    m_netManager = new QNetworkAccessManager();

    QObject::connect( newCluster, SIGNAL( clicked()), this, SLOT( addNewCluster()));
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


  // -------------------------------------------------------------------- //
  
/*******************************
  m_tree = new AlgorithmTreeWidget(f,mui);
  m_tree->setHeaderLabel("Algorithms");
  connect(m_tree,SIGNAL(itemSelectionChanged()),this,SLOT(treeSelectionChanged()));

  QHBoxLayout * buttonLayout = new QHBoxLayout();
  buttonLayout->setName("testC");
  QPushButton *execButton = new QPushButton("Execute");
  m_findAlg = new FindAlgComboBox;
  m_findAlg->setEditable(true);
  m_findAlg->completer()->setCompletionMode(QCompleter::PopupCompletion);

  connect(m_findAlg,SIGNAL(editTextChanged(const QString&)),this,SLOT(findAlgTextChanged(const QString&)));
  connect(m_findAlg,SIGNAL(enterPressed()),m_mantidUI,SLOT(executeAlgorithm()));
  connect(execButton,SIGNAL(clicked()),m_mantidUI,SLOT(executeAlgorithm()));

  buttonLayout->addWidget(execButton);
  buttonLayout->addWidget(m_findAlg);
  buttonLayout->addStretch();

  m_runningLayout = new QHBoxLayout();
  m_runningLayout->setName("testA");

  m_runningButton = new QPushButton("Details");
  m_runningLayout->addStretch();
  m_runningLayout->addWidget(m_runningButton);
  connect(m_runningButton,SIGNAL(clicked()),m_mantidUI,SLOT(showAlgMonitor()));
  //
  QVBoxLayout * layout = new QVBoxLayout();
  f->setLayout(layout);
  layout->addLayout(buttonLayout);
  layout->addWidget(m_tree);
  layout->addLayout(m_runningLayout);
  //

  m_treeChanged = false;
  m_findAlgChanged = false;
***************************/
  setWidget(f);

}

RemoteAlgorithmDockWidget::~RemoteAlgorithmDockWidget()
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

void RemoteAlgorithmDockWidget::update()
{
    m_tree->clear();


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


        QTreeWidgetItem *catItem;
        if (m_configReply)
        {
          catItem = new QTreeWidgetItem(QStringList(m_configReply->read(25)));
        }
        else
        {
            catItem = new QTreeWidgetItem(QStringList("Update() has just been called."));
        }
        m_tree->addTopLevelItem(catItem);
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
void RemoteAlgorithmDockWidget::addNewCluster()
{
  NewClusterDialog *theDialog = new NewClusterDialog();
  if (theDialog->exec() == QDialog::Accepted)
  {
    // Grab the values the user entered
    MwsRemoteJobManager *manager = new  MwsRemoteJobManager( theDialog->getDisplayName().toStdString(),
                                                             theDialog->getConfigFileURL().toString().toStdString(),
                                                             theDialog->getServiceBaseURL().toString().toStdString(),
                                                             theDialog->getUserName().toStdString());
    m_clusterList.append( manager);
    
    // Add the Display name to the combo box
    m_clusterCombo->addItem( theDialog->getDisplayName());
  }
  
}

void RemoteAlgorithmDockWidget::clusterChoiceChanged(int index)
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

void RemoteAlgorithmDockWidget::findAlgTextChanged(const QString& text)
{
  
}

void RemoteAlgorithmDockWidget::treeSelectionChanged()
{
  
}

void RemoteAlgorithmDockWidget::selectionChanged(const QString& algName)
{
  
}

void RemoteAlgorithmDockWidget::updateProgress(void* alg, const double p, const QString& msg,
                                               double estimatedTime, int progressPrecision)
{
  
}

void RemoteAlgorithmDockWidget::algorithmStarted(void* alg)
{
  
}

void RemoteAlgorithmDockWidget::algorithmFinished(void* alg)
{
  
}


void RemoteAlgorithmDockWidget::xmlParseServerAttributes( QDomElement &elm)
{
    // We don't actually do anything with the server attributes yet...
    return;
}


/*
  NOTE: All I'm doing right now is parsing this silly thing for the name tag so I can
  add it to the algorithm tree.  I'll add more code once I have a better idea of what
  actually needs to be done.
  */
void RemoteAlgorithmDockWidget::xmlParseAlgorithm( QDomElement &elm)
{

    QDomNode n = elm.firstChild();
    while( !n.isNull() )
    {
      QDomElement e = n.toElement();
      if (!e.isNull())
      {
        if (e.tagName() == "name" )
        {
            QString algName = e.text();
            QTreeWidgetItem *catItem;
            catItem = new QTreeWidgetItem(QStringList(algName));
            m_tree->addTopLevelItem(catItem);
        }

/*
        else if (e.tagName() == "executable")
        else if (e.tagName() == "parameter_list")
        else
        {
            QMessageBox msgBox;
            msgBox.setText("Unrecognized XML Element");
            msgBox.setInformativeText( e.tagName() + QString(tr(" is not a recognized XML element.  It will be ignored.")));
            msgBox.exec();
        }
*/
      }

      n = n.nextSibling();
    }




    QTreeWidgetItem *catItem;
    catItem = new QTreeWidgetItem(QStringList(m_configReply->read(25)));

}

