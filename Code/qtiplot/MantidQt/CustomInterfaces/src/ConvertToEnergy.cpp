//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/ConvertToEnergy.h"
#include "MantidQtCustomInterfaces/Homer.h"

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomInterfaces

{
  DECLARE_SUBWINDOW(ConvertToEnergy);
}
}

using namespace MantidQt::CustomInterfaces;

//----------------------
// Public member functions
//----------------------
///Constructor
ConvertToEnergy::ConvertToEnergy(QWidget *parent) :
  UserSubWindow(parent)
{
}

/// Set up the dialog layout
void ConvertToEnergy::initLayout()
{
  m_uiForm.setupUi(this);
  
  m_directInstruments = new Homer(qobject_cast<QWidget*>(this->parent()), m_uiForm);
  m_directInstruments->initLayout();
}

/**
 * Initialize Python
 */
void ConvertToEnergy::initLocalPython()
{
  connect(m_directInstruments, SIGNAL(runAsPythonScript(const QString&)),
	  this, SIGNAL(runAsPythonScript(const QString&)));
  m_directInstruments->initLocalPython();
}
