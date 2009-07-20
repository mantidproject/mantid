#include "MantidQtCustomDialogs/LoadRawDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"

#include "MantidKernel/Property.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QFontMetrics>
#include <QFileInfo>
#include <QDir>

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomDialogs
{
  DECLARE_DIALOG(LoadRawDialog)
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
LoadRawDialog::LoadRawDialog(QWidget *parent) : AlgorithmDialog(parent),  m_fileFilter("")
{
}

/**
  *Destructor
  */
LoadRawDialog::~LoadRawDialog()
{	
}

//---------------------------------------
// Private member functions
//---------------------------------------
/**
 * Reimplemented virtual function to set up the dialog
 */
void LoadRawDialog::initLayout()
{
  m_mainLayout = new QVBoxLayout(this);

  if( isMessageAvailable() )
  {
    QLabel *inputMessage = new QLabel(this);
    inputMessage->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    inputMessage->setText(getOptionalMessage());
    QHBoxLayout *msgArea = new QHBoxLayout;
    msgArea->addWidget(inputMessage);
    m_mainLayout->addLayout(msgArea);
  }
  
  //Filename boxes
  addFilenameInput();

  ///Output workspace property
  addOutputWorkspaceInput();

  ///Spectra related properties
  addSpectraInput();
  
  ///Cache combo box 
  addCacheOptions();
  
  //Buttons 
  m_mainLayout->addLayout(createDefaultButtonLayout("?", "Load", "Cancel"));
  
}

void LoadRawDialog::parseInput()
{
  //Filename property
  storePropertyValue("Filename", m_pathBox->text());
  //workspace name
  storePropertyValue("OutputWorkspace", m_wsBox->text());
  //Spectra
  storePropertyValue("SpectrumMin", m_minSpec->text());
  storePropertyValue("SpectrumMax", m_maxSpec->text());
  storePropertyValue("SpectrumList", m_specList->text());
  
  //Cache
  storePropertyValue("Cache", m_cacheBox->currentText());
  
}

/**
 * Add the Filename property input
 */
void LoadRawDialog::addFilenameInput()
{
  QLabel *fileName = new QLabel("Select a file to load:");
  m_pathBox = new QLineEdit;

  m_browseBtn = new QPushButton("Browse");
  connect(m_browseBtn, SIGNAL(clicked()), this, SLOT(browseClicked()));
  fillLineEdit("Filename", m_pathBox);
  
  m_pathBox->setMinimumWidth(m_pathBox->fontMetrics().maxWidth()*13);
  QString docstring = QString::fromStdString(getAlgorithmProperty("Filename")->documentation());
  m_pathBox->setToolTip(docstring);
  m_browseBtn->setToolTip(docstring);

  QHBoxLayout *nameline = new QHBoxLayout;
  nameline->addWidget(fileName);  
  nameline->addWidget(m_pathBox);   
    
  QLabel *validLbl = getValidatorMarker("Filename");
  nameline->addWidget(validLbl);  
  nameline->addWidget(m_browseBtn);  

  bool isEnabled = isWidgetEnabled("Filename");
  m_pathBox->setEnabled(isEnabled);
  m_browseBtn->setEnabled(isEnabled);
  
  m_mainLayout->addLayout(nameline); 
}

///Output workspace property
void LoadRawDialog::addOutputWorkspaceInput()
{
  QLabel *wsName = new QLabel("Enter name for workspace:");
  m_wsBox = new QLineEdit;
  fillLineEdit("OutputWorkspace", m_wsBox);
  m_wsBox->setMaximumWidth(m_wsBox->fontMetrics().maxWidth()*7); 
  m_wsBox->setToolTip(  QString::fromStdString(getAlgorithmProperty("OutputWorkspace")->documentation()) );

  QHBoxLayout *wsline = new QHBoxLayout;
  wsline->addWidget(wsName);  
  wsline->addWidget(m_wsBox);  
  QLabel *validLbl = getValidatorMarker("OutputWorkspace");
  wsline->addWidget(validLbl);  
  wsline->addStretch();
    
  m_wsBox->setEnabled(isWidgetEnabled("OutputWorkspace"));

  m_mainLayout->addLayout(wsline); 
}

///Spectra related properties
void LoadRawDialog::addSpectraInput()
{
  QGroupBox *groupbox = new QGroupBox("Spectra Options");
  
  QVBoxLayout *spectra = new QVBoxLayout;
  
  QLabel *min = new QLabel("Start:");
  m_minSpec = new QLineEdit;
  m_minSpec->setToolTip(  QString::fromStdString(getAlgorithmProperty("SpectrumMin")->documentation()) );
  fillLineEdit("SpectrumMin", m_minSpec);

  int charWidth = m_minSpec->fontMetrics().maxWidth();
  m_minSpec->setMaximumWidth(charWidth*3);
  
  QLabel *validmin = getValidatorMarker("SpectrumMin");
  QLabel *validmax = getValidatorMarker("SpectrumMax");
  QLabel *validlist = getValidatorMarker("SpectrumList");
  
  QLabel *max = new QLabel("End:");
  m_maxSpec = new QLineEdit;
  m_maxSpec->setToolTip( QString::fromStdString(getAlgorithmProperty("SpectrumMax")->documentation()) );
  fillLineEdit("SpectrumMax", m_maxSpec);
  
  m_maxSpec->setMaximumWidth(charWidth*3);
  
  QLabel *list = new QLabel("List:");
  m_specList = new QLineEdit;
  fillLineEdit("SpectrumList", m_specList);
  
  m_specList->setMaximumWidth(charWidth*10);
  m_specList->setToolTip( QString::fromStdString(getAlgorithmProperty("SpectrumList")->documentation()) );

  QHBoxLayout *minmaxLine = new QHBoxLayout;
  minmaxLine->addWidget(min);
  minmaxLine->addWidget(m_minSpec);
  minmaxLine->addWidget(validmin);
  minmaxLine->addSpacing(charWidth);
  
  minmaxLine->addWidget(max);
  minmaxLine->addWidget(m_maxSpec);
  minmaxLine->addWidget(validmax);
  
  minmaxLine->addSpacing(charWidth);
  minmaxLine->addWidget(list);
  minmaxLine->addWidget(m_specList);
  minmaxLine->addWidget(validlist);
  
  minmaxLine->addStretch();

  m_minSpec->setEnabled(isWidgetEnabled("SpectrumMin"));
  m_maxSpec->setEnabled(isWidgetEnabled("SpectrumMax"));
  m_specList->setEnabled(isWidgetEnabled("SpectrumList"));
    
  spectra->addLayout(minmaxLine);
  
  groupbox->setLayout(spectra);
  m_mainLayout->addWidget(groupbox);
}

///Cache combo box 
void LoadRawDialog::addCacheOptions()
{
  QLabel *cacheLabel = new QLabel("Cache file locally:");
  m_cacheBox = new QComboBox;
  fillAndSetComboBox("Cache", m_cacheBox);
  m_cacheBox->setToolTip( QString::fromStdString(getAlgorithmProperty("Cache")->documentation()) );

  QHBoxLayout *cacheline = new QHBoxLayout;
  
  cacheline->addWidget(cacheLabel, 0, Qt::AlignRight);
  cacheline->addWidget(m_cacheBox, 0, Qt::AlignLeft);
  QLabel *validLbl = getValidatorMarker("Cache");
  cacheline->addWidget(validLbl, 0, Qt::AlignLeft);
  cacheline->addStretch();  
  
  m_mainLayout->addLayout(cacheline);
}

/**
  * A slot for the browse button "clicked" signal
  */
void LoadRawDialog::browseClicked()
{
  if( !m_pathBox->text().isEmpty() )
  {
    MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(m_pathBox->text()).absoluteDir().path());
  }  

  QString filepath = this->openLoadFileDialog("Filename");
  if( !filepath.isEmpty() ) 
  {
    m_pathBox->clear();
    m_pathBox->setText(filepath.trimmed());
  }

  //Add a suggestion for workspace name
  if( m_wsBox->isEnabled() && !filepath.isEmpty() ) m_wsBox->setText(QFileInfo(filepath).baseName());
}
