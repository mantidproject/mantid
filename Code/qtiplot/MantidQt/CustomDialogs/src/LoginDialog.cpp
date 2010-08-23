//----------------------
// Includes
//----------------------
#include "MantidQtCustomDialogs/LoginDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"

#include <QDesktopServices>
#include <QUrl>


//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomDialogs
{
  DECLARE_DIALOG(LoginDialog);
}
}

using namespace MantidQt::CustomDialogs;

//----------------------
// Public member functions
//----------------------
///Constructor
LoginDialog::LoginDialog(QWidget *parent) :
  AlgorithmDialog(parent)
{
}

/// Set up the dialog layout
void LoginDialog::initLayout()
{
   m_uiForm.setupUi(this);
   fillLineEdit("Username", m_uiForm.fedIdEdit);
   fillLineEdit("Password",m_uiForm.passwordEdit);
   connect(m_uiForm.helpButton,SIGNAL(clicked()),this,SLOT(helpButtonClicked()));
}

/// Parse input when the dialog is accepted
void LoginDialog::parseInput()
{
	storePropertyValue("Username", m_uiForm.fedIdEdit->text());
	storePropertyValue("Password",m_uiForm.passwordEdit->text() );
}

/// handler for help button click
void LoginDialog::helpButtonClicked()
{
	QDesktopServices::openUrl(QUrl("http://www.mantidproject.org/MantidPlot:_The_ICat_Menu#ICat-.3E_Login"));
}