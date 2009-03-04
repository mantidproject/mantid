//----------------------------------
// Includes
//----------------------------------
#include "MantidQtAPI/GenericDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"

#include "MantidKernel/PropertyWithValue.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QSignalMapper>
#include <QFileDialog>

// Dialog stuff is defined here
using namespace MantidQt::API;

//----------------------------------
// Public member functions
//----------------------------------
/**
 * Default Constructor
 */
GenericDialog::GenericDialog(QWidget* parent) : AlgorithmDialog(parent)
{
  m_signalMapper = new QSignalMapper(this);
}

/**
 * Destructor
 */
GenericDialog::~GenericDialog()
{
}

//----------------------------------
// Protected member functions
//----------------------------------
/**
 * Create the layout for this dialog.
 */
void GenericDialog::initLayout()
{
  //Put the property boxes in a grid
   m_inputGrid = new QGridLayout;

   //See if we have any previous input for this algorithm
   QHash<QString, QString> oldValues;
   AlgorithmInputHistory::Instance().hasPreviousInput(QString::fromStdString(getAlgorithm()->name()), oldValues);

  //Each property is on its own row
  int row(0);
  std::vector<Mantid::Kernel::Property*>::const_iterator pEnd = getAlgorithm()->getProperties().end();
  for ( std::vector<Mantid::Kernel::Property*>::const_iterator pIter = getAlgorithm()->getProperties().begin();
	pIter != pEnd; ++pIter, ++row )
  {
    Mantid::Kernel::Property* prop = *pIter;
    QString propName = QString::fromStdString(prop->name());
    // Only produce allow input for output properties or workspace properties
    if ( prop->direction() == Mantid::Kernel::Direction::Output &&
	 !dynamic_cast<Mantid::API::IWorkspaceProperty*>(prop)) continue;

    // The name and valid label
    QLabel *nameLbl = new QLabel(propName);
    QLabel *validLbl = getValidatorMarker(propName);

    
    bool fileType = (prop->getValidatorType() == "file");

    if ( !prop->allowedValues().empty() && !fileType )
    {

      //If the property has allowed values then use a combo box.			
      QComboBox *optionsBox = new QComboBox;
      nameLbl->setBuddy(optionsBox);
      
      if ( dynamic_cast<Mantid::Kernel::PropertyWithValue<bool>* >(prop) )
      {
	optionsBox->addItem("No");
	optionsBox->setItemData(0, QString("0"));
	optionsBox->addItem("Yes");
	optionsBox->setItemData(1, QString("1"));
// 	// Set to show default value. This works because *p returns a boolean and that is then cast to an integer 0/1 and that
// 	// maps to the correct index
// 	
	QString selectedValue("");
	if( oldValues.contains(propName) ) selectedValue = oldValues[propName];
	else selectedValue = QString::fromStdString(prop->value());

	if( selectedValue == "0" ) optionsBox->setCurrentIndex(0);
	else optionsBox->setCurrentIndex(1);
	
      }
      else
      {
	std::vector<std::string> items = prop->allowedValues();
	std::vector<std::string>::const_iterator vend = items.end();
	
	QString selectedValue("");
  if( isForScript() )
  {
    selectedValue = QString::fromStdString(prop->value());
  }
  else
  {
    if( oldValues.contains(propName) ) selectedValue = oldValues[propName];
  }
  
	int index(0);
	for(std::vector<std::string>::const_iterator vitr = items.begin(); vitr != vend; 
	    ++vitr, ++index)
	{
	  optionsBox->addItem(QString::fromStdString(*vitr));
	  optionsBox->setItemData(index, QString::fromStdString(*vitr));
	  if( QString::fromStdString(*vitr) == selectedValue ) optionsBox->setCurrentIndex(index);
	}
	
	if( isForScript() && prop->isValid() && !prop->isDefault() ) optionsBox->setEnabled(false);
      }

      m_inputGrid->addWidget(nameLbl, row, 0, 0);
      m_inputGrid->addWidget(optionsBox, row, 1, 0);
      m_inputGrid->addWidget(validLbl, row, 2, 0);
    }
    else 
    {
      QLineEdit *textBox = new QLineEdit;
      nameLbl->setBuddy(textBox);
      m_editBoxes[textBox] = row;

      if( isForScript() && prop->isValid() && !prop->isDefault() )
      {
	textBox->setText(QString::fromStdString(prop->value()));
	textBox->setEnabled(false);
      }
      else
      {
	if( oldValues.contains(propName) ) textBox->setText(oldValues[propName]);
      }

      //Add the widgets to the grid
      m_inputGrid->addWidget(nameLbl, row, 0, 0);
      m_inputGrid->addWidget(textBox, row, 1, 0);
      m_inputGrid->addWidget(validLbl, row, 2, 0);

      if( fileType )
      {
	QPushButton *browseBtn = new QPushButton(tr("Browse"));
	connect(browseBtn, SIGNAL(clicked()), m_signalMapper, SLOT(map()));
	m_signalMapper->setMapping(browseBtn, textBox);
	
	m_inputGrid->addWidget(browseBtn, row, 3, 0);
	
	if( isForScript() && prop->isValid() && !prop->isDefault() ) browseBtn->setEnabled(false);
      }
      
    }
  }

  //Wire up the signal mapping object
  connect(m_signalMapper, SIGNAL(mapped(QWidget*)), this, SLOT(browseClicked(QWidget*)));
 	
  m_okButton = new QPushButton(tr("OK"));
  connect(m_okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
	
  m_exitButton = new QPushButton(tr("Cancel"));
  connect(m_exitButton, SIGNAL(clicked()), this, SLOT(close()));
	

  //Put everything in a vertical box
  // Making the dialog a parent of the layout automatically sets mainLay as the top-level layout
  QVBoxLayout *mainLay = new QVBoxLayout(this);
 
  if( isMessageAvailable() )
  {
    QLabel *inputMessage = new QLabel(this);
    inputMessage->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    inputMessage->setText(getOptionalMessage());
    QHBoxLayout *msgArea = new QHBoxLayout;
    msgArea->addWidget(inputMessage);
    mainLay->addLayout(msgArea);
  }

  //The property boxes
  mainLay->addLayout(m_inputGrid);
	
  QHBoxLayout *buttonRowLayout = new QHBoxLayout;
  buttonRowLayout->addStretch();
  buttonRowLayout->addWidget(m_exitButton);
  buttonRowLayout->addWidget(m_okButton);
  mainLay->addLayout(buttonRowLayout);
  
}

/**
 * Parses the input from the box and adds the propery (name, value) pairs to 
 * the map stored in the base class
 */
void GenericDialog::parseInput()
{
  int nRows = m_inputGrid->rowCount();
  for( int row = 0; row < nRows; ++row )
  {
    QLabel *propName = static_cast<QLabel*>(m_inputGrid->itemAtPosition(row, 0)->widget());
    QWidget *buddy = propName->buddy();

    if( qobject_cast<QLineEdit*>(buddy) )
    {
      addPropertyValueToMap(propName->text(), qobject_cast<QLineEdit*>(buddy)->text());
    }
    else
    {
      QComboBox *box = qobject_cast<QComboBox*>(buddy);
      addPropertyValueToMap(propName->text(), box->itemData(box->currentIndex()).toString());
    }
  }
}

/**
 * This slot is called when a browse button is clicked
 * @param widget The widget that is associated with the button that was clicked. In this case they are always QLineEdit widgets
 */
void GenericDialog::browseClicked(QWidget* widget)
{
  //I mapped this to a QLineEdit, so cast it
  QLineEdit *pathBox = qobject_cast<QLineEdit*>(widget);
  
  int index = m_editBoxes[pathBox];
  Mantid::Kernel::Property* prop = getAlgorithm()->getProperties()[index];

  //The allowed values in this context are file extensions
  std::vector<std::string> exts = prop->allowedValues();

  QString filter;
  if( !exts.empty() )
  {
    filter = "Files (";
		
    std::vector<std::string>::const_iterator iend = exts.end();
    for( std::vector<std::string>::const_iterator itr = exts.begin(); itr != iend; ++itr)
    {
  	  filter.append("*." + QString::fromStdString(*itr) + " ");
    }
		
    filter.trimmed();
    filter.append(QString::fromStdString(")"));
  }
  else
    {
      filter = "All Files (*.*)";
    }

  QString prevdir("");
  if( !pathBox->text().isEmpty() )
  {
    prevdir = QFileInfo(pathBox->text()).absoluteDir().path();
    AlgorithmInputHistory::Instance().setPreviousDirectory(prevdir);
  }  
   
  QString filepath = QFileDialog::getOpenFileName(this, tr("Select File"), AlgorithmInputHistory::Instance().getPreviousDirectory(), filter);
  if( filepath.isEmpty() ) return;
  
  pathBox->setText(filepath);
  prevdir = QFileInfo(pathBox->text()).absoluteDir().path();
  AlgorithmInputHistory::Instance().setPreviousDirectory(prevdir);
}
