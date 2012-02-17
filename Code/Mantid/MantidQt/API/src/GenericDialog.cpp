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
  bool hasInputWS = haveInputWS(prop_list);

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
      PropertyWidget * widget = PropertyWidgetFactory::createWidget(prop, this, m_currentGrid, row);

      // Record in the list of tied widgets (used in the base AlgorithmDialog)
      tie(widget, propName, m_currentGrid);

      // Whenever the value changes in the widget, this fires propertyChanged()
      connect(widget, SIGNAL( valueChanged(const QString &)), this, SLOT(propertyChanged(const QString &)));

      // For clicking the "Replace Workspace" button (if any)
      connect(widget, SIGNAL( replaceWorkspaceName(const QString &)), this, SLOT(replaceWSClicked(const QString &)));

      // Only show the "Replace Workspace" button if the algorithm has an input workspace.
      widget->showReplaceWSButton(hasInputWS);

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


//-------------------------------------------------------------------------------------------------
/** Go through all the properties, and check their validators to determine
 * whether they should be made disabled/invisible.
 * It also shows/hids the validators.
 * All properties' values should be set already, otherwise the validators
 * will be running on old data.
 */
void GenericDialog::hideOrDisableProperties()
{
  //std::cout << "hideOrDisableProperties===========================================================\n";
  QStringList::const_iterator pend = m_algProperties.end();
  for( QStringList::const_iterator pitr = m_algProperties.begin(); pitr != pend; ++pitr )
  {
    const QString propName = *pitr;
    Mantid::Kernel::Property *prop = getAlgorithmProperty(propName);

    // Find the widget for this property.
    if (m_tied_properties.contains(propName))
    {
      QWidget* widget = m_tied_properties[propName];
      PropertyWidget* propWidget = qobject_cast<PropertyWidget*>(widget);
      if (propWidget)
      {

        // Set the enabled and visible flags based on what the validators say. Default is always true.
        bool enabled = isWidgetEnabled(propName);
        bool visible = prop->isVisible();

        // Dynamic PropertySettings objects allow a property to change validators.
        // This removes the old widget and creates a new one instead.
        if (prop->isConditionChanged())
        {
          prop->getSettings()->applyChanges(prop);

          // Delete the old widget
          int row = propWidget->getGridRow();
          QGridLayout * layout = propWidget->getGridLayout();
          propWidget->deleteLater();

          // Create the appropriate widget at this row in the grid.
          propWidget = PropertyWidgetFactory::createWidget(prop, this, layout, row);
          widget = propWidget;

          // Record in the list of tied widgets (used in the base AlgorithmDialog)
          tie(widget, propName, layout);

          // Whenever the value changes in the widget, this fires propertyChanged()
          connect(widget, SIGNAL( valueChanged(const QString &)), this, SLOT(propertyChanged(const QString &)));
        }

        // Show/hide the validator label (that red star)
        QString error = "";
        if (m_errors.contains(propName)) error = m_errors[propName];
        // Always show controls that are in error
        if (error.length() != 0)
          visible = true;

        //std::cout << prop->name() << " enabled " << enabled << " visible " << visible << std::endl;

        // Hide/disable the widget
        propWidget->setEnabled( enabled );
        propWidget->setVisible( visible );

        QLabel *validator = getValidatorMarker(propName);
        // If there's no validator then assume it's handling its own validation notification
        if( validator )
        {
          validator->setToolTip( error );
          validator->setVisible( error.length() != 0);
        }
      } // is a PropertyWidget
    } // widget is tied
  } // for each property

  this->repaint(true);
}

//-------------------------------------------------------------------------------------------------
/** A slot to handle the replace workspace button click
 * @param outputEdit :: The line edit that is associated, via the signalmapper, with this click
 */
void GenericDialog::replaceWSClicked(const QString & propName)
{
  if (m_tied_properties.contains(propName))
  {
    QWidget * widget = m_tied_properties[propName];
    PropertyWidget* propWidget = qobject_cast<PropertyWidget*>(widget);
    if (propWidget)
    {
      // Find the name to put in the spot
      QString wsName("");
      QHash<QString, QWidget*>::iterator it;
      for (it = m_tied_properties.begin(); it != m_tied_properties.end(); it++)
      {
        // Only look at workspace properties
        Property * prop = this->getAlgorithmProperty(it.key());
        IWorkspaceProperty * wsProp = dynamic_cast<IWorkspaceProperty*>(prop);
        PropertyWidget* otherWidget = qobject_cast<PropertyWidget*>(it.value());
        if (otherWidget && wsProp)
        {
          if (prop->direction() == Direction::Input)
          {
            // Input workspace property. Get the text typed in.
            wsName = otherWidget->getValue();
            break;
          }
        }
      }

      if (!wsName.isEmpty())
        propWidget->setValue(wsName);
    }
  }

//  QPushButton *btn = qobject_cast<QPushButton*>(m_signal_mapper->mapping(outputEdit));
//  if( !btn ) return;
//  int input =  m_wsbtn_tracker.value(btn);
//
//  QWidget *wsInputWidget = m_inputws_opts.value(input-1);
//  QString wsname("");
//  if( QComboBox *options = qobject_cast<QComboBox*>(wsInputWidget) )
//  {
//    wsname = options->currentText();
//  }
//  else if( QLineEdit *editField = qobject_cast<QLineEdit*>(wsInputWidget) )
//  {
//    wsname = editField->text();
//  }
//  else return;
//
//  //Adjust tracker
//  input = (input % m_inputws_opts.size() ) + 1;
//  m_wsbtn_tracker[btn] = input;
//
//  // Check if any of the other line edits have this name
//  QVector<QLineEdit*>::const_iterator iend = m_outputws_fields.constEnd();
//  for( QVector<QLineEdit*>::const_iterator itr = m_outputws_fields.constBegin();
//       itr != iend; ++itr )
//  {
//    //Check that we are not the field we are actually comparing against
//    if( (*itr) == outputEdit ) continue;
//    if( (*itr)->text() == wsname )
//    {
//      wsname += "-1";
//      break;
//    }
//  }
//  QLineEdit *edit = qobject_cast<QLineEdit*>(outputEdit);
//  if( edit )
//  {
//    edit->setText(wsname);
//  }
}

