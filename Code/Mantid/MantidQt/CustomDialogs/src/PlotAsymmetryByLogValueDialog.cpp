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
#include <QSignalMapper>

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomDialogs
{
  DECLARE_DIALOG(PlotAsymmetryByLogValueDialog)
}
}

using namespace MantidQt::CustomDialogs;
using namespace MantidQt::API; 

//---------------------------------------
// Public member functions
//---------------------------------------
/**
 * Constructor
 */
PlotAsymmetryByLogValueDialog::PlotAsymmetryByLogValueDialog(QWidget *parent) : AlgorithmDialog(parent)
{
  browseButtonMapper = new QSignalMapper();

  connect(browseButtonMapper, SIGNAL(mapped(const QString&)), this, SLOT(openFileDialog(const QString&)));
}

/**
  *Destructor
  */
PlotAsymmetryByLogValueDialog::~PlotAsymmetryByLogValueDialog()
{	
  delete browseButtonMapper;
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

  // Tie all the properties
  tie(m_uiForm.firstRunBox, "FirstRun", m_uiForm.FirstRunLayout);
  tie(m_uiForm.lastRunBox, "LastRun", m_uiForm.LastRunLayout);
  tie(m_uiForm.logBox, "LogValue");
  tie(m_uiForm.outWSBox, "OutputWorkspace", m_uiForm.OutputWSLayout);
  tie(m_uiForm.typeBox, "Type");
  tie(m_uiForm.redBox, "Red");
  tie(m_uiForm.greenBox, "Green");
  tie(m_uiForm.forwardBox, "ForwardSpectra");
  tie(m_uiForm.backwardBox, "BackwardSpectra");
  tie(m_uiForm.timeMinBox, "TimeMin");
  tie(m_uiForm.timeMaxBox, "TimeMax");
  tie(m_uiForm.dtcType, "DeadTimeCorrType");
  tie(m_uiForm.dtcFile, "DeadTimeCorrFile");

  // Set-up browse button mapping
  browseButtonMapper->setMapping(m_uiForm.browseFirstButton, "FirstRun");
  browseButtonMapper->setMapping(m_uiForm.browseLastButton,  "LastRun");
  browseButtonMapper->setMapping(m_uiForm.dtcFileBrowseButton, "DeadTimeCorrFile");

  // Connect Browse buttons to the mapper
  connect(m_uiForm.browseFirstButton, SIGNAL(clicked()), browseButtonMapper, SLOT(map()));
  connect(m_uiForm.browseLastButton, SIGNAL(clicked()), browseButtonMapper, SLOT(map()));
  connect(m_uiForm.dtcFileBrowseButton, SIGNAL(clicked()), browseButtonMapper, SLOT(map()));

  connect( m_uiForm.firstRunBox, SIGNAL(textChanged(const QString&)), this, SLOT(fillLogBox(const QString&)) );
  connect( m_uiForm.btnOK,SIGNAL(clicked()),this,SLOT(accept()));
  connect( m_uiForm.btnCancel,SIGNAL(clicked()),this,SLOT(reject()));
  connect( m_uiForm.btnHelp,SIGNAL(clicked()),this,SLOT(helpClicked()));

  connect( m_uiForm.dtcType, SIGNAL(currentIndexChanged(int)), this, SLOT(showHideDeadTimeFileWidget(int)));

  // Fill ComboBoxes with allowed values
  fillAndSetComboBox("Type", m_uiForm.typeBox);
  fillAndSetComboBox("DeadTimeCorrType", m_uiForm.dtcType);

  // Fill log values from the file
  if ( !m_uiForm.firstRunBox->text().isEmpty() )
    fillLogBox(m_uiForm.firstRunBox->text());

  // So user can enter a custom value
  m_uiForm.logBox->setEditable(true);
}

/**
 * Opens a file dialog. Updates the QLineEdit provided when the dialog is closed.
 */
void PlotAsymmetryByLogValueDialog::openFileDialog(const QString& filePropName)
{
  QString selectedPath = AlgorithmDialog::openFileDialog(filePropName);

  if(!selectedPath.isEmpty())
  {
    // Save used directory for the next time
    AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(selectedPath).absoluteDir().path());

    // Get the widget for the file property
    QLineEdit* lineEdit = dynamic_cast<QLineEdit*>(m_tied_properties[filePropName]);

    if(!lineEdit)
      throw std::runtime_error("Widget of the file property was not found");

    lineEdit->setText(selectedPath.trimmed());
  }
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

  m_uiForm.logBox->clear();

  Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmFactory::Instance().create("LoadMuonNexus",-1);
  alg->initialize();
  try
  {
    alg->setPropertyValue("Filename",nexusFileName.toStdString());
    alg->setPropertyValue("OutputWorkspace","PlotAsymmetryByLogValueDialog_tmp");
    alg->setPropertyValue("DeadTimeTable", ""); // Don't need it for now
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
  catch(std::exception& )
  {
  }
}

/**
 * Show or hide Dead Time file widget depending on which Dead Time type is selected.
 * @param deadTimeTypeIndex Selected Dead Time Correction type index
 */
void PlotAsymmetryByLogValueDialog::showHideDeadTimeFileWidget(int deadTimeTypeIndex)
{
  // Show only if "Using specified file" selected
  m_uiForm.dtcFileContainer->setVisible(deadTimeTypeIndex == 2);
}
