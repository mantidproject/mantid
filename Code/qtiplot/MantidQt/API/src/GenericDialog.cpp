//----------------------------------
// Includes
//----------------------------------
#include "MantidQtAPI/GenericDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"

#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/FileProperty.h"
#include "MantidAPI/IWorkspaceProperty.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSignalMapper>
#include <QFileInfo>
#include <QDir>

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
    //Put everything in a vertical box
    // Making the dialog a parent of the layout automatically sets mainLay as the top-level layout
    QVBoxLayout *mainLay = new QVBoxLayout(this);

    // Create a grid of properties if there are any available
    
    const std::vector<Mantid::Kernel::Property*> & prop_list = getAlgorithm()->getProperties();
    if ( !prop_list.empty() )
    {
      //Put the property boxes in a grid
      m_inputGrid = new QGridLayout;

      // For the few algorithms (mainly loading) that do not have input workspaces, we do not
      // want to render the 'replace input workspace button'. Do a quick scan to check.
      // Also the ones that don't have a set of allowed values as input workspace
      bool haveInputWS(false);
      std::vector<Mantid::Kernel::Property*>::const_iterator pEnd = prop_list.end();
      for(std::vector<Mantid::Kernel::Property*>::const_iterator pIter = prop_list.begin();
	  pIter != pEnd; ++pIter)
      {
	Mantid::Kernel::Property *prop = *pIter;
	if( prop->direction() == Mantid::Kernel::Direction::Input && 
	    dynamic_cast<Mantid::API::IWorkspaceProperty*>(prop) )
	{
	  if( !prop->allowedValues().empty() )
	  {
	    haveInputWS = true;
	  }
	  break;
	}
      }
      //Each property is on its own row
      int row(-1);
      for(std::vector<Mantid::Kernel::Property*>::const_iterator pIter = prop_list.begin();
	  pIter != pEnd; ++pIter)
      {
	Mantid::Kernel::Property* prop = *pIter;
	QString propName = QString::fromStdString(prop->name());
	bool isWorkspaceProp(dynamic_cast<Mantid::API::IWorkspaceProperty*>(prop));
	// Only accept input for output properties or workspace properties
	if( prop->direction() == Mantid::Kernel::Direction::Output &&
	    !isWorkspaceProp )
	{
	  continue;
	}
	
	++row;
	// The name and valid label
	QLabel *nameLbl = new QLabel(propName);
	QLabel *validLbl = getValidatorMarker(propName);
	
	// Get the value string to enter into the box. The function figures out the
	// appropriate value to return based on previous input, script input or nothing
	
	bool isEnabled = isWidgetEnabled(propName);
	
	//check if there are only certain allowed values for the property
	Mantid::Kernel::FileProperty* fileType = dynamic_cast<Mantid::Kernel::FileProperty*>(prop);;
	if( dynamic_cast<Mantid::Kernel::PropertyWithValue<bool>* >(prop) ) 
	{
	  QCheckBox *checkBox = new QCheckBox(propName);
	  this->setCheckBoxState(propName, checkBox);
	  
	  checkBox->setToolTip(  QString::fromStdString(prop->documentation()) );
	  m_inputGrid->addWidget(new QLabel(""), row, 0, 0);
	  m_inputGrid->addWidget(checkBox, row, 1, 0);
	  m_inputGrid->addWidget(validLbl, row, 2, 0);
	  
	  checkBox->setEnabled(isEnabled);
	  
	}
	else if ( !prop->allowedValues().empty() && !fileType )
	{
	  //It is a choice of certain allowed values and can use a combination box
	  //Check if this is the row that matches the one that we want to link to the
	  //output box and used the saved combo box
	  QComboBox *optionsBox = new QComboBox;
	  fillAndSetComboBox(propName, optionsBox);
	  
	  nameLbl->setBuddy(optionsBox);
	  nameLbl->setToolTip(  QString::fromStdString(prop->documentation()) );
	  optionsBox->setToolTip(  QString::fromStdString(prop->documentation()) );
	  
	  m_inputGrid->addWidget(nameLbl, row, 0, 0);
	  m_inputGrid->addWidget(optionsBox, row, 1, 0);
	  m_inputGrid->addWidget(validLbl, row, 2, 0);
          
	  optionsBox->setEnabled(isEnabled);
	  if( isWorkspaceProp )
	  {
	    flagInputWS(optionsBox);
	  }
	}
	//For everything else render a text box
	else 
	{
	  QLineEdit *textBox = new QLineEdit;
	  fillLineEdit(propName, textBox);
	  
	  nameLbl->setBuddy(textBox);
	  m_editBoxes[textBox] = propName;
	  nameLbl->setToolTip(  QString::fromStdString(prop->documentation()) );
	  textBox->setToolTip(  QString::fromStdString(prop->documentation()) );
	  
	  //Add the widgets to the grid
	  m_inputGrid->addWidget(nameLbl, row, 0, 0);
	  m_inputGrid->addWidget(textBox, row, 1, 0);
	  m_inputGrid->addWidget(validLbl, row, 2, 0);

	  // If this is an output workspace property and there is an input workspace property
	  // add a button that replaces the input workspace
	  if( isWorkspaceProp && haveInputWS )
	  {
	    QPushButton *inputWSReplace = this->createReplaceWSButton(textBox);
	    if( inputWSReplace )
	    {
	      inputWSReplace->setEnabled(isEnabled);
	      m_inputGrid->addWidget(inputWSReplace, row, 3, 0);
	    }
	  }
	  textBox->setEnabled(isEnabled);
	  
	  if( fileType )
	  {
	    QPushButton *browseBtn = new QPushButton(tr("Browse"));
	    connect(browseBtn, SIGNAL(clicked()), m_signalMapper, SLOT(map()));
	    m_signalMapper->setMapping(browseBtn, textBox);
	    m_inputGrid->addWidget(browseBtn, row, 3, 0);
	    browseBtn->setEnabled(isEnabled);
	  }
	}//end combo box/dialog box decision

      }

      //Wire up the signal mapping object
      connect(m_signalMapper, SIGNAL(mapped(QWidget*)), this, SLOT(browseClicked(QWidget*)));

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
    }

    // Add the help, run and cancel buttons
    mainLay->addLayout(createDefaultButtonLayout());
}

/**
* Parses the input from the box and adds the propery (name, value) pairs to 
* the map stored in the base class
*/
void GenericDialog::parseInput()
{
  if (!m_inputGrid) return; // algorithm dont have properties
  int nRows = m_inputGrid->rowCount();
  for( int row = 0; row < nRows; ++row )
  {
    QWidget *control = m_inputGrid->itemAtPosition(row, 0)->widget();
    if( !control ) continue;
    QLabel *propName = static_cast<QLabel*>(control);
    if( !propName->text().isEmpty() )
    {
      QWidget *buddy = propName->buddy();
      if( QComboBox* select_box = qobject_cast<QComboBox*>(buddy) )
      {
	storePropertyValue(propName->text(), select_box->currentText());
      }
      else
      {
	storePropertyValue(propName->text(), qobject_cast<QLineEdit*>(buddy)->text());
      }
    }
    else
    {
      QCheckBox *checker = qobject_cast<QCheckBox*>(m_inputGrid->itemAtPosition(row, 1)->widget());
      if( !checker ) continue;
      
      if( checker->checkState() == Qt::Checked )
      {
	storePropertyValue(checker->text(), "1");
      }
      else
      {
	storePropertyValue(checker->text(), "0");
      }
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
  
  QString propName("");
  if( m_editBoxes.contains(pathBox) ) propName = m_editBoxes[pathBox];
  else return;

  if( !pathBox->text().isEmpty() )
  {
    AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(pathBox->text()).absoluteDir().path());
  }  
  QString filepath = this->openLoadFileDialog(propName);
  if( !filepath.isEmpty() ) 
  {
    pathBox->clear();
    pathBox->setText(filepath.trimmed());
  }
}
