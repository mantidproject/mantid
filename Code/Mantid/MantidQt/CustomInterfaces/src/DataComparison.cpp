//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/DataComparison.h"

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomInterfaces
{
  DECLARE_SUBWINDOW(DataComparison);
}
}

using namespace MantidQt::CustomInterfaces;

//----------------------
// Public member functions
//----------------------
///Constructor
DataComparison::DataComparison(QWidget *parent) :
  UserSubWindow(parent)
{
}

/// Set up the dialog layout
void DataComparison::initLayout()
{
  m_uiForm.setupUi(this);
}

