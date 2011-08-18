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
  m_signalMapper_browse = new QSignalMapper(this);
  m_signalMapper_browseMultiple = new QSignalMapper(this);
}

/**
* Destructor
*/
GenericDialog::~GenericDialog()
{
  delete m_signalMapper_browse;
  delete m_signalMapper_browseMultiple;
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
  QVBoxLayout *mainLay = new QVBoxLayout(scroll);
  viewport->setLayout(mainLay);

  // Add a layout for QDialog
  QVBoxLayout *dialog_layout = new QVBoxLayout(this);
  setLayout(dialog_layout);

  // Create a grid of properties if there are any available

  const std::vector<Property*> & prop_list = getAlgorithm()->getProperties();
  if ( !prop_list.empty() )
  {
    //Put the property boxes in a grid
    m_inputGrid = new QGridLayout;

    // For the few algorithms (mainly loading) that do not have input workspaces, we do not
    // want to render the 'replace input workspace button'. Do a quick scan to check.
    // Also the ones that don't have a set of allowed values as input workspace
    bool haveInputWS(false);
    std::vector<Property*>::const_iterator pEnd = prop_list.end();
    for(std::vector<Property*>::const_iterator pIter = prop_list.begin();
      pIter != pEnd; ++pIter)
    {
      Property *prop = *pIter;
      if( prop->direction() == Direction::Input && dynamic_cast<IWorkspaceProperty*>(prop) )
      {
        haveInputWS = true;
        break;
      }
    }
    //Each property is on its own row
    int row(-1);
    for(std::vector<Property*>::const_iterator pIter = prop_list.begin();
      pIter != pEnd; ++pIter)
    {
      Property* prop = *pIter;
      QString propName = QString::fromStdString(prop->name());
      bool isWorkspaceProp(dynamic_cast<IWorkspaceProperty*>(prop));
      // Only accept input for output properties or workspace properties
      if( prop->direction() == Direction::Output &&
        !isWorkspaceProp )
      {
        continue;
      }
      ++row;
      // The name and valid label
      QLabel *nameLbl = new QLabel(propName);
      nameLbl->setToolTip(  QString::fromStdString(prop->documentation()) );

      // Get the value string to enter into the box. The function figures out the
      // appropriate value to return based on previous input, script input or nothing

      bool isEnabled = isWidgetEnabled(propName);

      Mantid::API::FileProperty* fileType = dynamic_cast<Mantid::API::FileProperty*>(prop);
      Mantid::API::MultipleFileProperty* multipleFileType = dynamic_cast<Mantid::API::MultipleFileProperty*>(prop);

      // CheckBox shown for BOOL properties
      if( dynamic_cast<PropertyWithValue<bool>* >(prop) ) 
      {
        QCheckBox *checkBox = new QCheckBox(propName);
        m_inputGrid->addWidget(new QLabel(""), row, 0, 0);
        m_inputGrid->addWidget(checkBox, row, 1, 0);
        tie(checkBox, propName, m_inputGrid);
      }
      //Check if there are only certain allowed values for the property
      else if ( !prop->allowedValues().empty() && !fileType )
      {
        //It is a choice of certain allowed values and can use a combination box
        //Check if this is the row that matches the one that we want to link to the
        //output box and used the saved combo box
        QComboBox *optionsBox = new QComboBox;
        std::set<std::string> items = prop->allowedValues();
        std::set<std::string>::const_iterator vend = items.end();
        for(std::set<std::string>::const_iterator vitr = items.begin(); vitr != vend; 
          ++vitr)
        {
          optionsBox->addItem(QString::fromStdString(*vitr));
        }

        nameLbl->setBuddy(optionsBox);

        m_inputGrid->addWidget(nameLbl, row, 0, 0);
        m_inputGrid->addWidget(optionsBox, row, 1, 0);
        tie(optionsBox, propName, m_inputGrid);
        if( isWorkspaceProp )
        {
          flagInputWS(optionsBox);
        }
      }
      //For everything else render a text box
      else 
      {
        QLineEdit *textBox = new QLineEdit;
        nameLbl->setBuddy(textBox);
        //check this is a masked property
        Mantid::Kernel::MaskedProperty<std::string> * maskedProp = dynamic_cast<Mantid::Kernel::MaskedProperty<std::string> *>(prop);
        if(maskedProp)
        {
          textBox->setEchoMode(QLineEdit::Password);
        }
        m_editBoxes[textBox] = propName;

        //Add the widgets to the grid
        m_inputGrid->addWidget(nameLbl, row, 0, 0);
        m_inputGrid->addWidget(textBox, row, 1, 0);
        QWidget* validator = tie(textBox, propName, m_inputGrid);	  

        // If this is an output workspace property and there is an input workspace property
        // add a button that replaces the input workspace
        int row_pos(0), col_pos(0), span(0);
        m_inputGrid->getItemPosition(m_inputGrid->indexOf(validator), &row_pos, &col_pos, &span, &span);
        if( isWorkspaceProp )
        {
          if( prop->direction() == Direction::Output )
          {
            QPushButton *inputWSReplace = this->createReplaceWSButton(textBox);
            if( haveInputWS && inputWSReplace )
            {
              inputWSReplace->setEnabled(isEnabled);
              m_inputGrid->addWidget(inputWSReplace, row, col_pos + 1, 0);
            }
          }
          else if( prop->direction() == Direction::Input )
          {
            flagInputWS(textBox);
          }
          else {}
        }

        //Is this a FileProperty?
        if( fileType )
        {
          //Make a browser button
          QPushButton *browseBtn = new QPushButton(tr("Browse"));
          connect(browseBtn, SIGNAL(clicked()), m_signalMapper_browse, SLOT(map()));
          m_signalMapper_browse->setMapping(browseBtn, textBox);
          m_inputGrid->addWidget(browseBtn, row, col_pos + 1, 0);
          browseBtn->setEnabled(isEnabled);
          //Wire up the signal mapping object
          connect(m_signalMapper_browse, SIGNAL(mapped(QWidget*)), this, SLOT(browseClicked(QWidget*)));
        }

        //Is this a MultipleFileProperty?
        if( multipleFileType )
        {
          //Make a browser button
          QPushButton *browseBtn = new QPushButton(tr("Browse"));
          connect(browseBtn, SIGNAL(clicked()), m_signalMapper_browseMultiple, SLOT(map()));
          m_signalMapper_browseMultiple->setMapping(browseBtn, textBox);
          m_inputGrid->addWidget(browseBtn, row, col_pos + 1, 0);
          browseBtn->setEnabled(isEnabled);
          //Wire up the signal mapping object
          connect(m_signalMapper_browseMultiple, SIGNAL(mapped(QWidget*)), this, SLOT(browseMultipleClicked(QWidget*)));
        }


      }//end combo box/dialog box decision

    }


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


/**
* This slot is called when a browse button is clicked
* @param widget :: The widget that is associated with the button that was clicked. In this case they are always QLineEdit widgets
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
  QString filepath = this->openFileDialog(propName);
  if( !filepath.isEmpty() ) 
  {
    pathBox->clear();
    pathBox->setText(filepath.trimmed());
  }
}


/** This slot is called when a browse button for multiple files is clicked.
 *
* @param widget :: The widget that is associated with the button that was clicked. In this case they are always QLineEdit widgets
*/
void GenericDialog::browseMultipleClicked(QWidget* widget)
{
  std::cout << "GenericDialog::browseMultipleClicked()" << std::endl;
  //I mapped this to a QLineEdit, so cast it
  QLineEdit *pathBox = qobject_cast<QLineEdit*>(widget);

  // Get property name
  QString propName("");
  if( m_editBoxes.contains(pathBox) ) propName = m_editBoxes[pathBox];
  else return;

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


