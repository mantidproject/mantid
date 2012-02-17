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
#include <QGroupBox>
#include <climits>
#include "MantidQtAPI/FilePropertyWidget.h"
#include "MantidQtAPI/PropertyWidgetFactory.h"

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



//
////---------------------------------------------------------------------------------------------------------------
///** Layout a checkbox for a bool property
// * @param prop :: Property to create controls for
// * @param row :: insertion location for checkbox
// */
//void GenericDialog::layoutBoolProperty(PropertyWithValue<bool>* prop, int row)
//{
//  QString propName = QString::fromStdString(prop->name());
//  QCheckBox *checkBox = new QCheckBox(propName);
//  m_currentGrid->addWidget(new QLabel(""), row, 0, 0);
//  m_currentGrid->addWidget(checkBox, row, 1, 0);
//  tie(checkBox, propName, m_currentGrid);
//
//  // Map the check box state changing to the property name
//  QSignalMapper * mapper = new QSignalMapper(this);
//  connect(checkBox, SIGNAL( stateChanged(int)), mapper, SLOT(map()));
//  mapper->setMapping(checkBox, propName);
//  connect(mapper, SIGNAL(mapped(const QString &)), this, SLOT(propertyChanged(const QString &)));
//  m_mappers.insert(propName, mapper);
//
//}
//
//
////---------------------------------------------------------------------------------------------------------------
///** Layout a combobox for a property with options
// * @param prop :: Property to create controls for
// * @param row :: insertion location for checkbox
// */
//void GenericDialog::layoutOptionsProperty(Property* prop, int row)
//{
//  QString propName = QString::fromStdString(prop->name());
//  // The name and valid label
//  QLabel *nameLbl = new QLabel(propName);
//  nameLbl->setToolTip(  QString::fromStdString(prop->documentation()) );
//  m_currentGrid->addWidget(nameLbl, row, 0, 0);
//
//  //It is a choice of certain allowed values and can use a combination box
//  //Check if this is the row that matches the one that we want to link to the
//  //output box and used the saved combo box
//  QComboBox *optionsBox = new QComboBox;
//  bool isWorkspaceProp(dynamic_cast<IWorkspaceProperty*>(prop));
//  std::set<std::string> items = prop->allowedValues();
//  std::set<std::string>::const_iterator vend = items.end();
//  for(std::set<std::string>::const_iterator vitr = items.begin(); vitr != vend; ++vitr)
//  {
//    QString propValue = QString::fromStdString(*vitr);
//    if ( isWorkspaceProp && ( ! m_showHidden ) && propValue.startsWith("__") ) continue;
//    optionsBox->addItem(propValue);
//  }
//
//  m_currentGrid->addWidget(optionsBox, row, 1, 0);
//  tie(optionsBox, propName, m_currentGrid, true, nameLbl);
//  if( isWorkspaceProp )
//    flagInputWS(optionsBox);
//
//  // Map the options box to the property name
//  QSignalMapper * mapper = new QSignalMapper(this);
//  connect(optionsBox, SIGNAL( currentIndexChanged(int)), mapper, SLOT(map()));
//  mapper->setMapping(optionsBox, propName);
//  connect(mapper, SIGNAL(mapped(const QString &)), this, SLOT(propertyChanged(const QString &)));
//  m_mappers.insert(propName, mapper);
//}
//
//
//
////---------------------------------------------------------------------------------------------------------------
///** Layout a textbox for a property with options
// * @param prop :: Property to create controls for
// * @param row :: insertion location for checkbox
// */
//void GenericDialog::layoutTextProperty(Property* prop, int row)
//{
//  QString propName = QString::fromStdString(prop->name());
//  // The name and valid label
//  QLabel *nameLbl = new QLabel(propName);
//  nameLbl->setToolTip(  QString::fromStdString(prop->documentation()) );
//  m_currentGrid->addWidget(nameLbl, row, 0, 0);
//
//  // The textbox
//  QLineEdit *textBox = new QLineEdit;
//  nameLbl->setBuddy(textBox);
//
//  //check this is a masked property
//  Mantid::Kernel::MaskedProperty<std::string> * maskedProp = dynamic_cast<Mantid::Kernel::MaskedProperty<std::string> *>(prop);
//  if(maskedProp)
//    textBox->setEchoMode(QLineEdit::Password);
//
//  // If this is an output workspace property and there is an input workspace property
//  // add a button that replaces the input workspace
//  QPushButton *inputWSReplace = NULL;
//  bool isWorkspaceProp(dynamic_cast<IWorkspaceProperty*>(prop));
//  if( isWorkspaceProp )
//  {
//    if( prop->direction() == Direction::Output )
//    {
//      if( haveInputWS(getAlgorithm()->getProperties()) )
//      {
//        inputWSReplace = this->createReplaceWSButton(textBox);
//        if (inputWSReplace)
//          m_currentGrid->addWidget(inputWSReplace, row, 2, 0);
//      }
//    }
//    else if( prop->direction() == Direction::Input )
//    {
//      flagInputWS(textBox);
//    }
//    else {}
//  }
//
//  // For file properties
//  QPushButton *browseBtn = NULL;
//
//  //Is this a FileProperty?
//  Mantid::API::FileProperty* fileType = dynamic_cast<Mantid::API::FileProperty*>(prop);
//  Mantid::API::MultipleFileProperty* multipleFileType = dynamic_cast<Mantid::API::MultipleFileProperty*>(prop);
//  if( fileType || multipleFileType )
//  {
//    //Make a browser button
//    browseBtn = new QPushButton(tr("Browse"));
//    m_currentGrid->addWidget(browseBtn, row, 2, 0);
//
//    // Map click on the button to link to the browseMultipleClicked(PropName) method
//    QSignalMapper * mapper = new QSignalMapper(this);
//    connect(browseBtn, SIGNAL(clicked()), mapper, SLOT(map()));
//    mapper->setMapping(browseBtn, propName);
//    if (fileType)
//      connect(mapper, SIGNAL(mapped(const QString &)), this, SLOT(browseClicked(const QString &)));
//    else if (multipleFileType)
//      connect(mapper, SIGNAL(mapped(const QString &)), this, SLOT(browseMultipleClicked(const QString &)));
//    m_mappers.insert(propName, mapper);
//  }
//  else
//  {
//    // NON-file edit box
//    // Map the edit box to the property name
//    QSignalMapper * mapper = new QSignalMapper(this);
//    connect(textBox, SIGNAL(editingFinished()), mapper, SLOT(map()));
//    mapper->setMapping(textBox, propName);
//    connect(mapper, SIGNAL(mapped(const QString &)), this, SLOT(propertyChanged(const QString &)));
//    m_mappers.insert(propName, mapper);
// }
//
//  //Add the widgets to the grid
//  m_currentGrid->addWidget(textBox, row, 1, 0);
//  tie(textBox, propName, m_currentGrid, true, nameLbl, browseBtn, inputWSReplace);
//}



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
    m_currentGrid = m_inputGrid;

    std::string group = "";

    //Each property is on its own row
    int row = 0;

    for(std::vector<Property*>::const_iterator pIter = prop_list.begin();
      pIter != prop_list.end(); ++pIter)
    {
      Property* prop = *pIter;
      QString propName = QString::fromStdString(prop->name());

      // Are we entering a new group?
      if (prop->getGroup() != group)
      {
        group = prop->getGroup();

        if (group == "")
        {
          // Return to the original grid
          m_currentGrid = m_inputGrid;
        }
        else
        {
          // Make a groupbox with a border and a light background
          QGroupBox * grpBox = new QGroupBox(QString::fromStdString(group) );
          grpBox->setAutoFillBackground(true);
          grpBox->setStyleSheet(
              "QGroupBox { border: 1px solid gray;  border-radius: 4px; font-weight: bold; margin-top: 4px; margin-bottom: 4px; padding-top: 16px; }"
              "QGroupBox::title { background-color: transparent;  subcontrol-position: top center;  padding-top:4px; padding-bottom:4px; } ");
          QPalette pal = grpBox->palette();
          pal.setColor(grpBox->backgroundRole(), pal.alternateBase().color());
          grpBox->setPalette(pal);

          // Put the frame in the main grid
          m_inputGrid->addWidget(grpBox, row, 0, 1, 4);

          // Make a layout in the grp box
          m_currentGrid = new QGridLayout;
          grpBox->setLayout(m_currentGrid);
          row++;
        }
      }

      // Only accept input for output properties or workspace properties
      bool isWorkspaceProp(dynamic_cast<IWorkspaceProperty*>(prop));
      if( prop->direction() == Direction::Output && !isWorkspaceProp )
        continue;

      // Create the appropriate widget at this row in the grid.
      PropertyWidgetFactory::createWidget(prop, this, m_currentGrid, row);


//      // the function analyses the property type and creates specific widget for it
//      // in the vertical position, specified by row;
//      this->createSpecificPropertyWidget(prop, row);

      ++row;
    } //(end for each property)

    // Add the helpful summary message
    if( isMessageAvailable() )
      this->addOptionalMessage(dialog_layout);

    //The property boxes
    mainLay->addLayout(m_inputGrid);

  }
  // Add a stretchy item to allow the properties grid to be top-aligned
  mainLay->addStretch(1);

  dialog_layout->addWidget(scroll); // add scroll to the QDialog's layout
  // Add the help, run and cancel buttons
  dialog_layout->addLayout(createDefaultButtonLayout());

  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  scroll->setWidget(viewport);
  scroll->setWidgetResizable(true);
  scroll->setAlignment(Qt::AlignLeft & Qt::AlignTop);

  // At this point, all the widgets have been added and are visible.
  // This makes sure the viewport does not get scaled smaller, even if some controls are hidden.
  viewport->setMinimumHeight( viewport->height() + 10 );

  const int screenHeight = QApplication::desktop()->height();
  const int dialogHeight = viewport->height();
  // If the thing won't end up too big compared to the screen height,
  // resize the scroll area so we don't get a scroll bar
  if ( (dialogHeight+100) < 0.8*screenHeight )
  {
    scroll->setFixedHeight(viewport->height()+10);
  }

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
* @param propName :: The widget that is associated with the button that was clicked. In this case they are always QLineEdit widgets
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
  QStringList files = FilePropertyWidget::openMultipleFileDialog( getAlgorithmProperty(propName) );

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


