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
#include <QCheckBox>

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
LoadRawDialog::LoadRawDialog(QWidget *parent) : AlgorithmDialog(parent)
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
  QVBoxLayout *main_layout = new QVBoxLayout(this);  

  // Add the helpful summary message
  if( isMessageAvailable() )
    this->addOptionalMessage(main_layout);

  
  //------------- Filename property ---------------------
  QHBoxLayout *prop_line = new QHBoxLayout;
  prop_line->addWidget(new QLabel("Select a file to load:"));  

  m_pathBox = new QLineEdit;
  m_pathBox->setMinimumWidth(m_pathBox->fontMetrics().maxWidth()*13);
  prop_line->addWidget(m_pathBox);
  tie(m_pathBox, "Filename", prop_line);
  
  QPushButton *browseBtn = new QPushButton("Browse");
  connect(browseBtn, SIGNAL(clicked()), this, SLOT(browseClicked()));
  browseBtn->setEnabled(isWidgetEnabled("Filename"));
  prop_line->addWidget(browseBtn);
  
  main_layout->addLayout(prop_line); 

  //------------- OutputWorkspace property ---------------------
  m_wsBox = new QLineEdit;

  prop_line = new QHBoxLayout;
  prop_line->addWidget(new QLabel("Enter name for workspace:"));  
  prop_line->addWidget(m_wsBox);
  int charWidth = m_wsBox->fontMetrics().maxWidth();
  m_wsBox->setMaximumWidth(charWidth*8);
  tie(m_wsBox, "OutputWorkspace", prop_line);
  prop_line->addStretch();
  main_layout->addLayout(prop_line); 

  //------------- Spectra properties ---------------------
  QGroupBox *groupbox = new QGroupBox("Spectra Options");
  prop_line = new QHBoxLayout;
  
  QLineEdit *text_field = new QLineEdit;
  text_field->setMaximumWidth(charWidth*3);
  prop_line->addWidget(new QLabel("Start:"));
  prop_line->addWidget(text_field);
  tie(text_field, "SpectrumMin", prop_line);
  text_field = new QLineEdit;
  text_field->setMaximumWidth(charWidth*3);
  prop_line->addWidget(new QLabel("End:"));
  prop_line->addWidget(text_field);
  tie(text_field, "SpectrumMax", prop_line);
  text_field = new QLineEdit;
  text_field->setMinimumWidth(charWidth*6);
  prop_line->addWidget(new QLabel("List:"));
  prop_line->addWidget(text_field);
   
  tie(text_field, "SpectrumList", prop_line);
  prop_line->addStretch();
  groupbox->setLayout(prop_line);
  main_layout->addWidget(groupbox);
  
  //------------- Cache option , log files options and Monitors Options ---------------------
   
  Mantid::Kernel::Property* cacheProp= getAlgorithmProperty("Cache");
  if(cacheProp)
  {
    QComboBox *cacheBox = new QComboBox;
    std::set<std::string> items =cacheProp->allowedValues();
    std::set<std::string>::const_iterator vend = items.end();
    for(std::set<std::string>::const_iterator vitr = items.begin(); vitr != vend; 
	      ++vitr)
    {
	    cacheBox->addItem(QString::fromStdString(*vitr));
    }
    prop_line = new QHBoxLayout;
    prop_line->addWidget(new QLabel("Cache file locally:"), 0, Qt::AlignRight);
    prop_line->addWidget(cacheBox, 0, Qt::AlignLeft);
    tie(cacheBox, "Cache", prop_line);
  }

  prop_line->addStretch();
  //If the algorithm version supports the LoadLog property add a check box for it
  Mantid::Kernel::Property *loadlogs = getAlgorithmProperty("LoadLogFiles");
  if( loadlogs )
  {   
    QCheckBox *checkbox = new QCheckBox ("Load Log Files",this);
    prop_line->addWidget(checkbox);
    tie(checkbox, "LoadLogFiles", prop_line);
  }
  prop_line->addStretch();  
  //If the algorithm version supports the LoadMonitors property add a check box for it
  Mantid::Kernel::Property* loadMonitors=getAlgorithmProperty("LoadMonitors");
  if(loadMonitors)
  {  	  
	  QComboBox *monitorsBox =new QComboBox;
	  std::set<std::string> monitoritems =loadMonitors->allowedValues();
	  std::set<std::string>::const_iterator mend = monitoritems.end();
	  for(std::set<std::string>::const_iterator mitr = monitoritems.begin(); mitr != mend; 
		    ++mitr)
	  {
		  monitorsBox->addItem(QString::fromStdString(*mitr));
	  }
	  prop_line->addWidget(new QLabel("Monitors:"), 0, Qt::AlignRight);
	  prop_line->addWidget(monitorsBox);
	  tie(monitorsBox, "LoadMonitors", prop_line);
  }
  
  
  if(prop_line)main_layout->addLayout(prop_line);
  
  //Buttons 
  main_layout->addLayout(createDefaultButtonLayout("?", "Load", "Cancel"));
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

  QString filepath = this->openFileDialog("Filename");
  if( !filepath.isEmpty() ) 
  {
    m_pathBox->clear();
    m_pathBox->setText(filepath.trimmed());
  }

  //Add a suggestion for workspace name
  if( m_wsBox->isEnabled() && !filepath.isEmpty() ) m_wsBox->setText(QFileInfo(filepath).baseName());
}
