//----------------------------------
// Includes
//----------------------------------
#include "MantidQtAPI/GenericDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"

#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidKernel/MaskedProperty.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QScrollArea>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPalette>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSignalMapper>
#include <QFileInfo>
#include <QDir>
#include "MantidAPI/MultipleFileProperty.h"

// Dialog stuff is defined here
using namespace MantidQt::API;
using namespace Mantid::Kernel;
using namespace Mantid::API;

//----------------------------------
// Public member functions
//----------------------------------
/**
* Default Constructor
*/
GenericDialog::GenericDialog(QWidget* parent) : AlgorithmDialog(parent)
{
}

/**
* Destructor
*/
GenericDialog::~GenericDialog()
{
  // Delete all the mappers
  QHash<QString, QSignalMapper *>::iterator it;
  for (it = m_mappers.begin(); it != m_mappers.end(); it++)
    delete it.value();
}



//---------------------------------------------------------------------------------------------------------------
/** Layout a checkbox for a bool property
 * @param prop :: Property to create controls for*/
void GenericDialog::layoutBoolProperty(PropertyWithValue<bool>* prop, int row)
{
  QString propName = QString::fromStdString(prop->name());
  QCheckBox *checkBox = new QCheckBox(propName);
  m_inputGrid->addWidget(new QLabel(""), row, 0, 0);
  m_inputGrid->addWidget(checkBox, row, 1, 0);
  tie(checkBox, propName, m_inputGrid);

  // Map the check box state changing to the property name
  QSignalMapper * mapper = new QSignalMapper(this);
  connect(checkBox, SIGNAL( stateChanged(int)), mapper, SLOT(map()));
  mapper->setMapping(checkBox, propName);
  connect(mapper, SIGNAL(mapped(const QString &)), this, SLOT(propertyChanged(const QString &)));
  m_mappers.insert(propName, mapper);

}


//---------------------------------------------------------------------------------------------------------------
/** Layout a combobox for a property with options
 * @param prop :: Property to create controls for */
void GenericDialog::layoutOptionsProperty(Property* prop, int row)
{
  QString propName = QString::fromStdString(prop->name());
  // The name and valid label
  QLabel *nameLbl = new QLabel(propName);
  nameLbl->setToolTip(  QString::fromStdString(prop->documentation()) );
  m_inputGrid->addWidget(nameLbl, row, 0, 0);

  //It is a choice of certain allowed values and can use a combination box
  //Check if this is the row that matches the one that we want to link to the
  //output box and used the saved combo box
  QComboBox *optionsBox = new QComboBox;
  std::set<std::string> items = prop->allowedValues();
  std::set<std::string>::const_iterator vend = items.end();
  for(std::set<std::string>::const_iterator vitr = items.begin(); vitr != vend; ++vitr)
    optionsBox->addItem(QString::fromStdString(*vitr));

  m_inputGrid->addWidget(optionsBox, row, 1, 0);
  tie(optionsBox, propName, m_inputGrid, true, nameLbl);
  bool isWorkspaceProp(dynamic_cast<IWorkspaceProperty*>(prop));
  if( isWorkspaceProp )
    flagInputWS(optionsBox);

  // Map the options box to the property name
  QSignalMapper * mapper = new QSignalMapper(this);
  connect(optionsBox, SIGNAL( currentIndexChanged(int)), mapper, SLOT(map()));
  mapper->setMapping(optionsBox, propName);
  connect(mapper, SIGNAL(mapped(const QString &)), this, SLOT(propertyChanged(const QString &)));
  m_mappers.insert(propName, mapper);
}



//---------------------------------------------------------------------------------------------------------------
bool haveInputWS(const std::vector<Property*> & prop_list)
{
  // For the few algorithms (mainly loading) that do not have input workspaces, we do not
  // want to render the 'replace input workspace button'. Do a quick scan to check.
  // Also the ones that don't have a set of allowed values as input workspace
  std::vector<Property*>::const_iterator pEnd = prop_list.end();
  for(std::vector<Property*>::const_iterator pIter = prop_list.begin();
    pIter != pEnd; ++pIter)
  {
    Property *prop = *pIter;
    if( prop->direction() == Direction::Input && dynamic_cast<IWorkspaceProperty*>(prop) )
    {
      return true;
    }
  }
  return false;
}


//---------------------------------------------------------------------------------------------------------------
/** Layout a textbox for a property with options
 * @param prop :: Property to create controls for */
void GenericDialog::layoutTextProperty(Property* prop, int row)
{
  QString propName = QString::fromStdString(prop->name());
  // The name and valid label
  QLabel *nameLbl = new QLabel(propName);
  nameLbl->setToolTip(  QString::fromStdString(prop->documentation()) );
  m_inputGrid->addWidget(nameLbl, row, 0, 0);

  // The textbox
  QLineEdit *textBox = new QLineEdit;
  nameLbl->setBuddy(textBox);

  //check this is a masked property
  Mantid::Kernel::MaskedProperty<std::string> * maskedProp = dynamic_cast<Mantid::Kernel::MaskedProperty<std::string> *>(prop);
  if(maskedProp)
    textBox->setEchoMode(QLineEdit::Password);

  // If this is an output workspace property and there is an input workspace property
  // add a button that replaces the input workspace
  QPushButton *inputWSReplace = NULL;
  bool isWorkspaceProp(dynamic_cast<IWorkspaceProperty*>(prop));
  if( isWorkspaceProp )
  {
    if( prop->direction() == Direction::Output )
    {
      if( haveInputWS(getAlgorithm()->getProperties()) )
      {
        inputWSReplace = this->createReplaceWSButton(textBox);
        if (inputWSReplace)
          m_inputGrid->addWidget(inputWSReplace, row, 2, 0);
      }
    }
    else if( prop->direction() == Direction::Input )
    {
      flagInputWS(textBox);
    }
    else {}
  }

  // For file properties
  QPushButton *browseBtn = NULL;

  //Is this a FileProperty?
  Mantid::API::FileProperty* fileType = dynamic_cast<Mantid::API::FileProperty*>(prop);
  Mantid::API::MultipleFileProperty* multipleFileType = dynamic_cast<Mantid::API::MultipleFileProperty*>(prop);
  if( fileType || multipleFileType )
  {
    //Make a browser button
    browseBtn = new QPushButton(tr("Browse"));
    m_inputGrid->addWidget(browseBtn, row, 2, 0);

    // Map click on the button to link to the browseMultipleClicked(PropName) method
    QSignalMapper * mapper = new QSignalMapper(this);
    connect(browseBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(browseBtn, propName);
    if (fileType)
      connect(mapper, SIGNAL(mapped(const QString &)), this, SLOT(browseClicked(const QString &)));
    else if (multipleFileType)
      connect(mapper, SIGNAL(mapped(const QString &)), this, SLOT(browseMultipleClicked(const QString &)));
    m_mappers.insert(propName, mapper);
  }
  else
  {
    // NON-file edit box
    // Map the edit box to the property name
    QSignalMapper * mapper = new QSignalMapper(this);
    connect(textBox, SIGNAL(editingFinished()), mapper, SLOT(map()));
    mapper->setMapping(textBox, propName);
    connect(mapper, SIGNAL(mapped(const QString &)), this, SLOT(propertyChanged(const QString &)));
    m_mappers.insert(propName, mapper);
 }

  //Add the widgets to the grid
  m_inputGrid->addWidget(textBox, row, 1, 0);
  tie(textBox, propName, m_inputGrid, true, nameLbl, browseBtn, inputWSReplace);
}





//----------------------------------
// Protected member functions
//----------------------------------
/**
* Create the layout for this dialog.
*/
void GenericDialog::initLayout()
{
  // Create a scroll area for the (rare) occasion when an algorithm has
  // so many properties it won't fit on the screen
  QScrollArea *scroll = new QScrollArea(this);
  QWidget *viewport = new QWidget(this);
  // Put everything in a vertical box and put it inside the scroll area
  QVBoxLayout *mainLay = new QVBoxLayout();
  viewport->setLayout(mainLay);

  // Add a layout for QDialog
  QVBoxLayout *dialog_layout = new QVBoxLayout();
  setLayout(dialog_layout);

  // Create a grid of properties if there are any available

  const std::vector<Property*> & prop_list = getAlgorithm()->getProperties();
  if ( !prop_list.empty() )
  {
    //Put the property boxes in a grid
    m_inputGrid = new QGridLayout;

    //Each property is on its own row
    int row(-1);
    for(std::vector<Property*>::const_iterator pIter = prop_list.begin();
      pIter != prop_list.end(); ++pIter)
    {
      Property* prop = *pIter;
      QString propName = QString::fromStdString(prop->name());

      // Only accept input for output properties or workspace properties
      bool isWorkspaceProp(dynamic_cast<IWorkspaceProperty*>(prop));
      if( prop->direction() == Direction::Output && !isWorkspaceProp )
        continue;
      ++row;

      // Look for specific property types
      Mantid::API::FileProperty* fileType = dynamic_cast<Mantid::API::FileProperty*>(prop);
      Mantid::API::MultipleFileProperty* multipleFileType = dynamic_cast<Mantid::API::MultipleFileProperty*>(prop);
      PropertyWithValue<bool>* boolProp = dynamic_cast<PropertyWithValue<bool>* >(prop);

      if (boolProp)
      { // CheckBox shown for BOOL properties
        layoutBoolProperty(boolProp, row);
      }
      else if ( !prop->allowedValues().empty() && !fileType && !multipleFileType )
      { //Check if there are only certain allowed values for the property
        layoutOptionsProperty(prop, row);
      }
      else 
      { //For everything else render a text box
        layoutTextProperty(prop, row);
      }
    } //(end for each property)

    // Add the helpful summary message
    if( isMessageAvailable() )
      this->addOptionalMessage(dialog_layout);

    //The property boxes
    mainLay->addLayout(m_inputGrid);
  }

  dialog_layout->addWidget(scroll); // add scroll to the QDialog's layout
  // Add the help, run and cancel buttons
  dialog_layout->addLayout(createDefaultButtonLayout());

  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  scroll->setWidget(viewport);
  scroll->setWidgetResizable(true);

  const int screenHeight = QApplication::desktop()->height();
  const int dialogHeight = viewport->height();
  // If the thing won't end up too big compared to the screen height,
  // resize the scroll area so we don't get a scroll bar
  if ( dialogHeight < 0.8*screenHeight ) scroll->setFixedHeight(viewport->height()+10);

  dialog_layout->setSizeConstraint(QLayout::SetMinimumSize);
}





//--------------------------------------------------------------------------------------
/** SLOT to be called whenever a property's value has just been changed
 * and the widget has lost focus/value has been changed.
 * @param pName :: name of the property that was changed
 */
void GenericDialog::propertyChanged(const QString & pName)
{
  this->storePropertyValue(pName, getValue( m_tied_properties[pName]) );
  this->setPropertyValue(pName, true);
}


//--------------------------------------------------------------------------------------
/**
* This slot is called when a browse button is clicked
* @param propName :: Property being edited
*/
void GenericDialog::browseClicked(const QString & propName)
{
  //I mapped this to a QLineEdit, so cast it
  QLineEdit *pathBox = qobject_cast<QLineEdit*>(m_tied_properties[propName]);

  if( !pathBox->text().isEmpty() )
  {
    AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(pathBox->text()).absoluteDir().path());
  }  
  QString filepath = this->openFileDialog(propName);
  if( !filepath.isEmpty() ) 
  {
    pathBox->clear();
    pathBox->setText(filepath.trimmed());
  }
}


//--------------------------------------------------------------------------------------
/** This slot is called when a browse button for multiple files is clicked.
 *
* @param widget :: The widget that is associated with the button that was clicked. In this case they are always QLineEdit widgets
*/
void GenericDialog::browseMultipleClicked(const QString & propName)
{
  //I mapped this to a QLineEdit, so cast it
  QLineEdit *pathBox = qobject_cast<QLineEdit*>(m_tied_properties[propName]);

  // Adjust the starting driectory
  if( !pathBox->text().isEmpty() )
  {
    QStringList files =  pathBox->text().split(",");
    if (files.size() > 0)
    {
      QString firstFile = files[0];
      AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(firstFile).absoluteDir().path());
    }
  }
  // Open multiple files in the dialog
  QStringList files = this->openMultipleFileDialog(propName);

  // Make into comma-sep string
  QString output;
  QStringList list = files;
  QStringList::Iterator it = list.begin();
  while(it != list.end())
  {
    if (it != list.begin()) output += ",";
    output += *it;
    it++;
  }
  if( !output.isEmpty() )
  {
    pathBox->clear();
    pathBox->setText(output);
  }
}


