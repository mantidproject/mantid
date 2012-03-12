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
  m_propsWidget = new AlgorithmPropertiesWidget(this);
  dialog_layout->addWidget(m_propsWidget, 1);
  m_propsWidget->setAlgorithm(this->getAlgorithm());

  // Create and add the OK/Cancel/Help. buttons
  dialog_layout->addLayout(this->createDefaultButtonLayout(), 0);

  QCoreApplication::processEvents();

  // At this point, all the widgets have been added and are visible.
  // This makes sure the viewport does not get scaled smaller, even if some controls are hidden.
  QWidget * viewport = m_propsWidget->m_viewport;
  viewport->layout()->update();

  const int screenHeight = QApplication::desktop()->height();
  const int dialogHeight = viewport->sizeHint().height();

  // If the thing won't end up too big compared to the screen height,
  // resize the scroll area so we don't get a scroll bar
  if ( (dialogHeight+100) < 0.8*screenHeight )
    m_propsWidget->m_scroll->setFixedHeight(dialogHeight+10);

  dialog_layout->setSizeConstraint(QLayout::SetMinimumSize);

  // Set all previous values (from history, etc.)
  for( auto it = m_propsWidget->m_propWidgets.begin(); it != m_propsWidget->m_propWidgets.end(); it++)
  {
    this->setPreviousValue(it.value(), it.key());
  }

  // Mark the properties that will be forced
  QStringList enabled = m_enabled;
  QStringList disabled = m_disabled;
  // Disabled the python arguments
  disabled += m_python_arguments;
  m_propsWidget->addEnabledAndDisableLists(enabled, disabled);

  // Using the default values, hide or disable the dynamically shown properties
  m_propsWidget->hideOrDisableProperties();

}


//-----------------------------------------------------------------------------
/** Parse out information from the dialog
 */
void GenericDialog::parseInput()
{
  auto itr = m_propsWidget->m_propWidgets.begin();
  for(; itr != m_propsWidget->m_propWidgets.end(); itr++ )
  {
    // Get the value from each widget and store it
    storePropertyValue(itr.key(), itr.value()->getValue());
  }
}




