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
#include "MantidQtAPI/AlgorithmPropertiesWidget.h"
#include "MantidQtAPI/PropertyWidget.h"

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

//----------------------------------
// Protected member functions
//----------------------------------
/**
* Create the layout for this dialog.
*/
void GenericDialog::initLayout()
{

  // Add a layout for QDialog
  QVBoxLayout *dialog_layout = new QVBoxLayout();
  setLayout(dialog_layout);
  // Add the helpful summary message
  if( isMessageAvailable() )
    this->addOptionalMessage(dialog_layout);

  // Make the widget with all the properties
  AlgorithmPropertiesWidget * propsWidget = new AlgorithmPropertiesWidget(this);
  dialog_layout->addWidget(propsWidget);
  propsWidget->setAlgorithm(this->getAlgorithm());

  // Create and add the OK/Cancel/Help. buttons
  dialog_layout->addLayout(this->createDefaultButtonLayout());

  for( auto it = propsWidget->m_propWidgets.begin(); it != propsWidget->m_propWidgets.end(); it++)
  {
    // This is the created PropertyWidget
    PropertyWidget * widget = *it;

    // Record in the list of tied widgets (used in the base AlgorithmDialog)
    tie(widget, QString::fromStdString(widget->getProperty()->name()), widget->getGridLayout());

    // Whenever the value changes in the widget, this fires propertyChanged()
    connect(widget, SIGNAL( valueChanged(const QString &)), this, SLOT(propertyChanged(const QString &)));

    // For clicking the "Replace Workspace" button (if any)
    connect(widget, SIGNAL( replaceWorkspaceName(const QString &)), this, SLOT(replaceWSClicked(const QString &)));
  } //(end for each PropertyWidget)

  QCoreApplication::processEvents();

  // At this point, all the widgets have been added and are visible.
  // This makes sure the viewport does not get scaled smaller, even if some controls are hidden.
  QWidget * viewport = propsWidget->m_viewport;
  viewport->layout()->update();

  const int screenHeight = QApplication::desktop()->height();
  const int dialogHeight = viewport->sizeHint().height();

  // If the thing won't end up too big compared to the screen height,
  // resize the scroll area so we don't get a scroll bar
  if ( (dialogHeight+100) < 0.8*screenHeight )
    propsWidget->m_scroll->setFixedHeight(dialogHeight+10);

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

