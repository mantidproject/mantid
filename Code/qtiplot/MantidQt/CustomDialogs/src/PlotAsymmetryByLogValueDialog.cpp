#include "MantidQtCustomDialogs/PlotAsymmetryByLogValueDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"

#include "MantidKernel/Property.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Algorithm.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QFileInfo>
#include <QDir>
#include <QCheckBox>

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomDialogs
{
  DECLARE_DIALOG(PlotAsymmetryByLogValueDialog)
}
}

// Just to save writing this everywhere 
using namespace MantidQt::CustomDialogs;

//---------------------------------------
// Public member functions
//---------------------------------------
/**
 * Constructor
 */
PlotAsymmetryByLogValueDialog::PlotAsymmetryByLogValueDialog(QWidget *parent) : AlgorithmDialog(parent)
{
}

/**
  *Destructor
  */
PlotAsymmetryByLogValueDialog::~PlotAsymmetryByLogValueDialog()
{	
}

//---------------------------------------
// Private member functions
//---------------------------------------
/**
 * Reimplemented virtual function to set up the dialog
 */
void PlotAsymmetryByLogValueDialog::initLayout()
{
  m_uiForm.setupUi(this);
  connect( m_uiForm.browseFirstButton, SIGNAL(clicked()), this, SLOT(browseFirstClicked()) );
  connect( m_uiForm.browseLastButton, SIGNAL(clicked()), this, SLOT(browseLastClicked()) );
  connect( m_uiForm.firstRunBox, SIGNAL(textChanged(const QString&)), this, SLOT(fillLogBox(const QString&)) );
  connect( m_uiForm.btnOK,SIGNAL(clicked()),this,SLOT(accept()));
  connect( m_uiForm.btnCancel,SIGNAL(clicked()),this,SLOT(reject()));
  connect( m_uiForm.btnHelp,SIGNAL(clicked()),this,SLOT(helpClicked()));

  fillLineEdit("FirstRun", m_uiForm.firstRunBox);
  fillLineEdit("LastRun", m_uiForm.lastRunBox);
  m_uiForm.logBox->setEditable(true);
  fillLineEdit("OutputWorkspace", m_uiForm.outWSBox);
  fillAndSetComboBox("Type",m_uiForm.typeBox);
  fillLineEdit("Red", m_uiForm.redBox);
  fillLineEdit("Green", m_uiForm.greenBox);
  fillLineEdit("ForwardSpectra", m_uiForm.forwardBox);
  fillLineEdit("BackwardSpectra", m_uiForm.backwardBox);
  fillLineEdit("TimeMin", m_uiForm.timeMinBox);
  fillLineEdit("TimeMax", m_uiForm.timeMaxBox);
}

/**
 * Retrieve the input from the dialog
 */
void PlotAsymmetryByLogValueDialog::parseInput()
{
  storePropertyValue("FirstRun", m_uiForm.firstRunBox->text());
  storePropertyValue("LastRun", m_uiForm.lastRunBox->text());
  storePropertyValue("LogValue", m_uiForm.logBox->currentText());
  storePropertyValue("OutputWorkspace", m_uiForm.outWSBox->text());
  storePropertyValue("Type", m_uiForm.typeBox->currentText());
  storePropertyValue("Red", m_uiForm.redBox->text());
  storePropertyValue("Green", m_uiForm.greenBox->text());
  storePropertyValue("ForwardSpectra", m_uiForm.forwardBox->text());
  storePropertyValue("BackwardSpectra", m_uiForm.backwardBox->text());
  storePropertyValue("TimeMin", m_uiForm.timeMinBox->text());
  storePropertyValue("TimeMax", m_uiForm.timeMaxBox->text()); 
}

/**
  * A slot for the browse button "clicked" signal
  */
void PlotAsymmetryByLogValueDialog::browseFirstClicked()
{
  if( !m_uiForm.firstRunBox->text().isEmpty() )
  {
    MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(m_uiForm.firstRunBox->text()).absoluteDir().path());
  }

  QString filepath = this->openFileDialog("FirstRun");
  if( !filepath.isEmpty() )
  {
    m_uiForm.firstRunBox->clear();
    m_uiForm.firstRunBox->setText(filepath.trimmed());
  }

//  //Add a suggestion for workspace name
//  if( m_wsBox->isEnabled() && !filepath.isEmpty() ) m_wsBox->setText(QFileInfo(filepath).baseName());
}

/**
  * A slot for the browse button "clicked" signal
  */
void PlotAsymmetryByLogValueDialog::browseLastClicked()
{
  if( !m_uiForm.firstRunBox->text().isEmpty() )
  {
    MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(m_uiForm.firstRunBox->text()).absoluteDir().path());
  }

  QString filepath = this->openFileDialog("LastRun");
  if( !filepath.isEmpty() )
  {
    m_uiForm.lastRunBox->clear();
    m_uiForm.lastRunBox->setText(filepath.trimmed());
  }

//  //Add a suggestion for workspace name
//  if( m_wsBox->isEnabled() && !filepath.isEmpty() ) m_wsBox->setText(QFileInfo(filepath).baseName());
}

/**
 * Fill m_uiForm.logBox with names of the log values read from one of the input files
 */
void PlotAsymmetryByLogValueDialog::fillLogBox(const QString&)
{
  QString nexusFileName = m_uiForm.firstRunBox->text();
  QFileInfo file(nexusFileName);
  if (!file.exists())
  {
    return;
  }

  Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmFactory::Instance().create("LoadNexus",-1);
  alg->initialize();
  try
  {
    alg->setPropertyValue("Filename",nexusFileName.toStdString());
    alg->setPropertyValue("OutputWorkspace","PlotAsymmetryByLogValueDialog_tmp");
    alg->setPropertyValue("SpectrumMin","0");
    alg->setPropertyValue("SpectrumMax","0");
    alg->execute();
    if (alg->isExecuted())
    {
      std::string wsName = alg->getPropertyValue("OutputWorkspace");
      Mantid::API::Workspace_sptr ws = Mantid::API::AnalysisDataService::Instance().retrieve(wsName);
      if ( !ws )
      {
        return;
      }
      Mantid::API::MatrixWorkspace_sptr mws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws);
      Mantid::API::WorkspaceGroup_sptr gws = boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(ws);
      if (gws)
      {
        if (gws->getNumberOfEntries() < 2) return;
        mws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
          Mantid::API::AnalysisDataService::Instance().retrieve(gws->getNames()[1])
        );
      }
      const std::vector< Mantid::Kernel::Property* >& props = mws->run().getLogData();
      if (gws)
      {
        std::vector<std::string> wsNames = gws->getNames();
        for(std::vector<std::string>::iterator it=wsNames.begin();it!=wsNames.end();++it)
        {
          Mantid::API::AnalysisDataService::Instance().remove(*it);
        }
      }
      else
      {
        Mantid::API::AnalysisDataService::Instance().remove("PlotAsymmetryByLogValueDialog_tmp");
      }
      for(size_t i=0;i<props.size();i++)
      {
        m_uiForm.logBox->addItem(QString::fromStdString(props[i]->name()));
      }
      // Display the appropriate value
      QString displayed("");
      if( !isForScript() )
      {
        displayed = MantidQt::API::AlgorithmInputHistory::Instance().previousInput("PlotAsymmetryByLogValue", "LogValue");
      }
      if( !displayed.isEmpty() )
      {
        int index = m_uiForm.logBox->findText(displayed);
        if( index >= 0 )
        {
          m_uiForm.logBox->setCurrentIndex(index);
        }
      }
    }
    
  }
  catch(std::exception& e)
  {
  }
}
