//----------------------------------
// Includes
//----------------------------------
#include "MantidQtAPI/UserSubWindow.h"

#include <QIcon>

using namespace MantidQt::API;

//------------------------------------------------------
// Public member functions
//------------------------------------------------------
/**
 * Default Constructor
 */
UserSubWindow::UserSubWindow(QWidget* parent) :  QMdiSubWindow(parent), m_bIsInitialized(false), m_ifacename("")
{
}

/**
 * Destructor
 */
UserSubWindow::~UserSubWindow()
{
}

/**
 * Create the layout for this dialog.
 */
void UserSubWindow::initializeLayout()
{
  if( isInitialized() ) return;

  //Calls the derived class function
  this->initLayout();

  //Se the object name to the interface name
  setObjectName(m_ifacename);
  setOption(QMdiSubWindow::RubberBandResize);

  //Set the icon
  setWindowIcon(QIcon(":/MantidPlot_Icon_32offset.png"));

  
  m_bIsInitialized = true;
}

/**
 * Has this window been initialized yet
 *  @returns Whether initialzedLayout has been called yet
 */
bool UserSubWindow::isInitialized() const
{ 
  return m_bIsInitialized; 
}

//--------------------------------------
// Private member functions
//-------------------------------------
/**
 * Set the interface name
 * @param iface_name The name of the interface
 */
void UserSubWindow::setInterfaceName(const QString & iface_name)
{
  m_ifacename = iface_name;
}
