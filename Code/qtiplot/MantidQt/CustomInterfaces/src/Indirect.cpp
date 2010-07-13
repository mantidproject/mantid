#include "MantidQtCustomInterfaces/Indirect.h"

#include <QUrl>
#include <QDesktopServices>

using namespace MantidQt::CustomInterfaces;

/**
* This is the constructor for the Indirect Instruments Interface.
* It is used primarily to ensure sane values for member variables.
*/
Indirect::Indirect(QWidget *parent, Ui::ConvertToEnergy & uiForm) : 
  ConvertToEnergy(parent) , m_mantidplot(parent) /*, m_runFilesWid(NULL),
  m_diagPage(NULL),m_saveChanged(false), m_isPyInitialized(false) */
{
  m_uiForm = uiForm;
}

/**
* This function performs any one-time actions needed when the Inelastic interface
* is first selected.
*/
void Indirect::initLayout()
{
	//
}

/**
* This function will hold any Python-dependent setup actions for the interface.
*/
void Indirect::initLocalPython()
{
	//
}

/**
* This function opens a web browser window to the Mantid Project wiki page for this
* interface ("Inelastic" subsection of ConvertToEnergy).
*/
void Indirect::helpClicked()
{
	QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
		"ConvertToEnergy#Inelastic"));
}

/**
* This function will control the actions needed for the Indirect interface when the
* "Run" button is clicked by the user.
*/
void Indirect::runClicked()
{
	//
	showInformationBox("Indirect Interface \"Run\" event.");
}

/**
* This function holds any steps that must be performed on the selection of an instrument,
* for example loading values from the Instrument Definition File (IDF).
* @param prefix The selected instruments prefix in Mantid.
*/
void Indirect::setIDFValues(const QString & prefix)
{
	//
}


