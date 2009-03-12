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
LoadRawDialog::LoadRawDialog(QWidget *parent) : AlgorithmDialog(parent),  m_fileFilter(""), m_oldValues()
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
 MantidQt::API::AlgorithmInputHistory::Instance().hasPreviousInput(QString::fromStdString(getAlgorithm()->name()),  m_oldValues);

  m_mainLayout = new QVBoxLayout(this);
  if( isMessageAvailable() )
  {
    QLabel inputMessage(this);
    inputMessage.setFrameStyle(QFrame::Panel | QFrame::Sunken);
    inputMessage.setText(getOptionalMessage());
    QHBoxLayout msgArea;
    msgArea.addWidget(&inputMessage);
    m_mainLayout->addLayout(&msgArea);
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
  QPushButton *loadButton = new QPushButton("Load");
  loadButton->setDefault(true);
  connect(loadButton, SIGNAL(clicked()), this, SLOT(accept()));

  QPushButton *cancelButton = new QPushButton("Cancel");
  connect(cancelButton, SIGNAL(clicked()), this, SLOT(close()));

  QHBoxLayout *buttonRowLayout = new QHBoxLayout;
  buttonRowLayout->addStretch();
  buttonRowLayout->addWidget(loadButton);
  buttonRowLayout->addWidget(cancelButton);
  m_mainLayout->addLayout(buttonRowLayout);
  
}

void LoadRawDialog::parseInput()
{
  //Filename property
  addPropertyValueToMap("Filename", m_pathBox->text());
  //workspace name
  addPropertyValueToMap("OutputWorkspace", m_wsBox->text());
  //Spectra
  addPropertyValueToMap("spectrum_min", m_minSpec->text());
  addPropertyValueToMap("spectrum_max", m_maxSpec->text());
  addPropertyValueToMap("spectrum_list", m_specList->text());
  
  //Cache
   addPropertyValueToMap("Cache", m_cacheBox->currentText());
  
}

/**
 * Add the Filename property input
 */
void LoadRawDialog::addFilenameInput()
{
  QLabel *fileName = new QLabel("Select a file to load:");
  m_pathBox = new QLineEdit;
  fileName->setBuddy(m_pathBox);
  m_browseBtn = new QPushButton("Browse");
  connect(m_browseBtn, SIGNAL(clicked()), this, SLOT(browseClicked()));
  setOldLineEditInput("Filename", m_pathBox);
  
  m_pathBox->setMinimumWidth(m_pathBox->fontMetrics().maxWidth()*13); 
  
  QHBoxLayout *nameline = new QHBoxLayout;
  nameline->addWidget(fileName);  
  nameline->addWidget(m_pathBox);
   
    
  QLabel *validLbl = getValidatorMarker("Filename");
  nameline->addWidget(validLbl);  
  nameline->addWidget(m_browseBtn);  

  m_mainLayout->addLayout(nameline); 
}

///Output workspace property
void LoadRawDialog::addOutputWorkspaceInput()
{
  QLabel *wsName = new QLabel("Enter name for workspace:");
  m_wsBox = new QLineEdit;
  setOldLineEditInput("OutputWorkspace", m_wsBox);
  m_wsBox->setMaximumWidth(m_wsBox->fontMetrics().maxWidth()*7); 

  QHBoxLayout *wsline = new QHBoxLayout;
  wsline->addWidget(wsName);  
  wsline->addWidget(m_wsBox);  
  QLabel *validLbl = getValidatorMarker("OutputWorkspace");
  wsline->addWidget(validLbl);  
  
  wsline->addStretch();
    
  m_mainLayout->addLayout(wsline); 
}

///Spectra related properties
void LoadRawDialog::addSpectraInput()
{
  QGroupBox *groupbox = new QGroupBox("Spectra Options");
  
  QVBoxLayout *spectra = new QVBoxLayout;
  
  QLabel *min = new QLabel("Start:");
  m_minSpec = new QLineEdit;
  setOldLineEditInput("spectrum_min", m_minSpec);

  int charWidth = m_minSpec->fontMetrics().maxWidth();
  m_minSpec->setMaximumWidth(charWidth*3);
  
  QLabel *validmin = getValidatorMarker("spectrum_min");
  QLabel *validmax = getValidatorMarker("spectrum_max");
  QLabel *validlist = getValidatorMarker("spectrum_list");
  
  QLabel *max = new QLabel("End:");
  m_maxSpec = new QLineEdit;
  setOldLineEditInput("spectrum_max", m_maxSpec);
  
  m_maxSpec->setMaximumWidth(charWidth*3);
  
  QLabel *list = new QLabel("List:");
  m_specList = new QLineEdit;
  setOldLineEditInput("spectrum_list", m_specList);
  
  m_specList->setMaximumWidth(charWidth*10);

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
    
  spectra->addLayout(minmaxLine);
  
  groupbox->setLayout(spectra);
  m_mainLayout->addWidget(groupbox);
}

///Cache combo box 
void LoadRawDialog::addCacheOptions()
{
  QLabel *cacheLabel = new QLabel("Cache file locally:");
  m_cacheBox = new QComboBox;
  cacheLabel->setBuddy(m_cacheBox);
  m_cacheBox->insertItem(0, "If slow");
  m_cacheBox->insertItem(1, "Always");
  m_cacheBox->insertItem(2, "Never");
  m_cacheBox->setCurrentIndex(0);
  setOldComboField("Cache");
  QHBoxLayout *cacheline = new QHBoxLayout;
  
  cacheline->addWidget(cacheLabel, 0, Qt::AlignRight);
  cacheline->addWidget(m_cacheBox, 0, Qt::AlignLeft);
  QLabel *validLbl = getValidatorMarker("Cache");
  cacheline->addWidget(validLbl, 0, Qt::AlignLeft);
  cacheline->addStretch();  
  
  m_mainLayout->addLayout(cacheline);
}

/// Set old input for combo box
void LoadRawDialog::setOldComboField(const QString & propName)
{
  Mantid::Kernel::Property *prop =getAlgorithmProperty(propName);
  QString selectedValue("");
  if( isForScript() ) selectedValue = QString::fromStdString(prop->value());
  else if( m_oldValues.contains(propName) ) selectedValue = m_oldValues[propName];
  else return;
  
  if( selectedValue.startsWith("I") ) m_cacheBox->setCurrentIndex(0);
  else if( selectedValue.startsWith("A") ) m_cacheBox->setCurrentIndex(1);
  else m_cacheBox->setCurrentIndex(2);
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
  if( m_wsBox->isEnabled() ) m_wsBox->setText(QFileInfo(filepath).baseName());
}
