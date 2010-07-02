//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/ConvertToEnergy.h"

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
}

