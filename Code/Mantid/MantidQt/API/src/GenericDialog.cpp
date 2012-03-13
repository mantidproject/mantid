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

  // Mark the properties that will be forced enabled or disabled
  QStringList enabled = m_enabled;
  QStringList disabled = m_disabled;
  // Disabled the python arguments
  disabled += m_python_arguments;
  m_propsWidget->addEnabledAndDisableLists(enabled, disabled);


  // At this point, all the widgets have been added and are visible.
  // This makes sure the viewport does not get scaled smaller, even if some controls are hidden.
  QWidget * viewport = m_propsWidget->m_viewport;
  //QScrollArea * scroll = m_propsWidget->m_scroll;
  viewport->layout()->update();
  // This makes the layout minimum size = that of the widgets inside
  viewport->layout()->setSizeConstraint(QLayout::SetMinimumSize);

  QCoreApplication::processEvents();

  int screenHeight = QApplication::desktop()->height();
  int dialogHeight = viewport->sizeHint().height();

//  std::cout << m_propsWidget->sizeHint().height() << " m_propsWidget->sizeHint().height()\n";
//  std::cout << viewport->sizeHint().height() << " viewport->sizeHint().height()\n";
//  std::cout << viewport->layout()->sizeHint().height() << " viewport->layout()->sizeHint().height\n";
//  std::cout << m_propsWidget->size().height() << " m_propsWidget->size().height()\n";
//  std::cout << viewport->size().height() << " viewport->size().height()\n";
//  std::cout << this->size().height() << " this->size().height()\n";

  // If the thing won't end up too big compared to the screen height,
  // resize the scroll area so we don't get a scroll bar
  if ( (dialogHeight+100) < 0.8*screenHeight )
  {
    m_propsWidget->m_scroll->setMinimumHeight(dialogHeight+10);
    // Find the size that the dialog WANTS to be.
    dialogHeight = this->sizeHint().height();
//    int dialogWidth = this->sizeHint().height() + 25;
    // But allow the scroll area to resize smaller again
    m_propsWidget->m_scroll->setMinimumHeight(60);
    // But resize the dialog again to its preferred size.
    this->resize(this->width(), dialogHeight);
  }

  // Set all previous values (from history, etc.)
  for( auto it = m_propsWidget->m_propWidgets.begin(); it != m_propsWidget->m_propWidgets.end(); it++)
  {
    this->setPreviousValue(it.value(), it.key());
  }

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

//-----------------------------------------------------------------------------
/**
 * A slot that can be used to connect a button that accepts the dialog if
 * all of the properties are valid
 */
void GenericDialog::accept()
{
  // Get property values
  parse();

  //Try and set and validate the properties and
  if( setPropertyValues() )
  {
    //Store input for next time
    saveInput();
    QDialog::accept();
  }
  else
  {
    // Highlight the validators that are in error (combined from them + whole algorithm)
    auto itr = m_propsWidget->m_propWidgets.begin();
    for(; itr != m_propsWidget->m_propWidgets.end(); itr++ )
    {
      if (m_errors.contains(itr.key()))
        itr.value()->setError( m_errors[itr.key()] );
    }

    QMessageBox::critical(this, "",
              "One or more properties are invalid. The invalid properties are\n"
        "marked with a *, hold your mouse over the * for more information." );
  }
}



