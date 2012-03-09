//----------------------
// Includes
//----------------------
#include "MantidQtCustomDialogs/StartLiveDataDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomDialogs
{
  DECLARE_DIALOG(StartLiveDataDialog);
}
}

using namespace MantidQt::CustomDialogs;

//----------------------
// Public member functions
//----------------------
///Constructor
StartLiveDataDialog::StartLiveDataDialog(QWidget *parent) :
  AlgorithmDialog(parent)
{
}

/// Set up the dialog layout
void StartLiveDataDialog::initLayout()
{
  m_uiForm.setupUi(this);
}

/// Parse input when the dialog is accepted
void StartLiveDataDialog::parseInput()
{
}
