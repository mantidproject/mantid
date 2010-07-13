//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/ConvertToEnergy.h"
#include "MantidQtCustomInterfaces/Homer.h" // user interface for Direct instruments
#include "MantidQTCustomInterfaces/Indirect.h" // user interface for Indirect instruments

#include "MantidKernel/ConfigService.h"

#include <QMessageBox>
#include <QDir>

#include <QDesktopServices>
#include <QUrl>

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

/**
* Default constructor for class. Initialises interface pointers to NULL values.
* @param parent This is a pointer to the "parent" object in Qt, most likely the main MantidPlot window.
*/
ConvertToEnergy::ConvertToEnergy(QWidget *parent) :
UserSubWindow(parent), m_directInstruments(NULL), m_indirectInstruments(NULL)
{
}

/**
* Empty virtual function to create interface for subclasses.
*/
void ConvertToEnergy::setIDFValues(const QString & prefix) 
{
	//
}

/**
* On user clicking the "help" button on the interface, directs their request to the relevant
* interface's helpClicked() function.
*/
void ConvertToEnergy::helpClicked()
{
	switch ( m_curEmodeType )
	{
	case Direct:
		m_directInstruments->helpClicked();
		break;
	case InDirect:
		m_indirectInstruments->helpClicked();
		break;
	default:
		QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
			"ConvertToEnergy"));
	}
}

/**
* This is the function called when the "Run" button is clicked. It will call the relevent function
* in the subclass.
*/
void ConvertToEnergy::runClicked()
{
	switch ( m_curEmodeType )
	{
	case Direct:
		m_directInstruments->runClicked();
		break;
	case InDirect:
		m_indirectInstruments->runClicked();
		break;
	case Undefined:
	default:
		showInformationBox("This interface is not configured to use the instrument you have selected.\nPlease check your instrument selection.");
	}
}

/**
* Sets up Qt UI file and connects signals, slots, etc. Calls setupCbInst(), etc.
*/
void ConvertToEnergy::initLayout()
{
	m_uiForm.setupUi(this);
	m_curInterfaceSetup = "";
	m_curEmodeType = Undefined;

	// set up the Instrument combo boxes
	setupCbInst();

	// Signal / Slot Connections Set Up Here

	// signal/slot connections to respond to changes in instrument selection combo boxes
	connect(m_uiForm.loadRun_cbInst, SIGNAL(activated(const QString&)), this, SLOT(userSelectInstrument(const QString&)));
	connect(m_uiForm.cbInst, SIGNAL(activated(const QString&)), this, SLOT(userSelectInstrument(const QString&)));

	// and to ensure that the two combo boxes display the same thing
	connect(m_uiForm.loadRun_cbInst, SIGNAL(activated(int)), m_uiForm.cbInst, SLOT(setCurrentIndex(int)));
	connect(m_uiForm.cbInst, SIGNAL(activated(int)), m_uiForm.loadRun_cbInst, SLOT(setCurrentIndex(int)));

	// connect "?" (Help) Button
	connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(helpClicked()));
	// connect the "Run" button
	connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));


}

/**
* This function is ran after initLayout(), and runPythonCode is unavailable before this function
* has run (because of the setup of the base class). For this reason, "setup" functions that require
* Python scripts are located here.
*/
void ConvertToEnergy::initLocalPython()
{
	// select starting instrument
	getStartingInstrument();
}



/**
* Populates the "Instrument" combo boxes for the direct and indirect interfaces.
* Uses the Mantid.properties file to get the name (displayed as ComboBox's Text), and
* the instrument prefix (used as the accompanying 'data' field).
*/
void ConvertToEnergy::setupCbInst()
{
	// Populate Instrument Selection Boxes
	Mantid::Kernel::ConfigServiceImpl & mtd_config = Mantid::Kernel::ConfigService::Instance();

	// get instrument names from config file
	std::string key_name = std::string("instrument.names.") + mtd_config.getString("default.facility");
	QString names = QString::fromStdString(mtd_config.getString(key_name));
	QStringList name_list = names.split(";", QString::SkipEmptyParts);
	// get instrument prefixes from config file
	std::string key_pref = std::string("instrument.prefixes.") + mtd_config.getString("default.facility");
	QString prefixes = QString::fromStdString(mtd_config.getString(key_pref));
	QStringList pref_list = prefixes.split(";", QString::SkipEmptyParts);

	// clear any previous contents of the comboBoxes
	m_uiForm.loadRun_cbInst->clear();
	m_uiForm.cbInst->clear();

	// iterate through the two QStringLists adding text (name) and data (prefix) to the ComboBox
	for ( int i = 0; i < pref_list.size() ; i++ )
	{
		m_uiForm.loadRun_cbInst->addItem(name_list[i], QVariant(pref_list[i]));
		m_uiForm.cbInst->addItem(name_list[i], QVariant(pref_list[i]));
	}

}

/**
* Sets up the initial instrument for the interface. This value is taken from the users'
* settings in the menu View -> Preferences -> Mantid -> Instrument
*/
void ConvertToEnergy::getStartingInstrument()
{
	Mantid::Kernel::ConfigServiceImpl & mtd_config = Mantid::Kernel::ConfigService::Instance();

	QString curInstru = QString::fromStdString(mtd_config.getString("default.instrument"));
	int index = m_uiForm.loadRun_cbInst->findData(QVariant(curInstru));
	QString prefix = m_uiForm.loadRun_cbInst->itemText(index);

	m_uiForm.loadRun_cbInst->setCurrentIndex(index);
	m_uiForm.cbInst->setCurrentIndex(index);

	instrumentSelectChanged(prefix);

}


/**
* This function: 1. loads the instrument and gets the value of deltaE-mode parameter
*				 2. Based on this value, makes the necessary changes to the form setup (direct or indirect).
* @param prefix name of the instrument from the QComboBox
*/
void ConvertToEnergy::instrumentSelectChanged(const QString& prefix)
{
	QString defFile = getIDFPath(prefix);

	if ( defFile == "" )
	{
		m_curEmodeType = Undefined;
		return;
	}

	DeltaEMode desired = instrumentDeltaEMode(defFile);

	if ( desired == Undefined )
	{
		m_curEmodeType = Undefined;
		QMessageBox::warning(this, "MantidPlot", "Selected instrument (" + prefix + ") does not have a parameter to signify it's deltaE-mode");
		return;
	}

	DeltaEMode current;

	if ( m_curInterfaceSetup == "" )
	{
		current = Undefined;
	}
	else
	{
		current = DeltaEMode(m_uiForm.swInstrument->currentIndex());
	}

	if ( desired != current || m_curInterfaceSetup != prefix )
	{
		changeInterface(desired);
	}

	m_curInterfaceSetup = prefix;
	m_curEmodeType = desired;

}

/**
* Gets the path to the selected instrument's Instrument Definition File (IDF), if the instrument has a parameter file.
* @param prefix the instrument's name from the QComboBox
* @return A string containing the path to the IDF, or an empty string if no parameter file exists.
*/
QString ConvertToEnergy::getIDFPath(const QString& prefix)
{
	QString paramfile_dir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("parameterDefinition.directory"));
	QDir paramdir(paramfile_dir);
	paramdir.setFilter(QDir::Files);
	QStringList filters;
	filters << prefix + "*_Parameters.xml";
	paramdir.setNameFilters(filters);

	QStringList entries = paramdir.entryList();
	QString defFilePrefix;

	if( entries.isEmpty() )
	{
		QMessageBox::warning(this, "MantidPlot", "Selected instrument (" + prefix + ") does not have a parameter file.\nCannot run analysis");
		m_uiForm.pbRun->setEnabled(false);
		return "";
	}
	else
	{
		m_uiForm.pbRun->setEnabled(true);
		defFilePrefix = entries[0];
		defFilePrefix.chop(15); // cut "_Parameters.xml" off the string
	}

	QString defFile = paramdir.filePath(defFilePrefix + "_Definition.xml");
	return defFile;
}

/**
* Runs a Python script to discover whether the selected instrument is direct or indirect.
* @param defFile path to instrument definition file.
* @return UNDEFINED deltaE-mode not found, otherwise the relevant value (DIRECT or INDIRECT)
*/
ConvertToEnergy::DeltaEMode ConvertToEnergy::instrumentDeltaEMode(const QString& defFile)
{
	QString pyInput =
		"from mantidsimple import *\n"
		"import sys\n"
		"LoadEmptyInstrument(\"%1\", \"instrument\")\n"
		"instrument = mtd['instrument'].getInstrument()\n"
		"try:\n"
		"    print instrument.getStringParameter('deltaE-mode')[0]\n"
		"except IndexError, message:\n" // the above line will raise an IndexError in Python
		"    print \"\"\n"				// if the instrument doesn't have this parameter.
		"mtd.deleteWorkspace('instrument')";

	pyInput = pyInput.arg(defFile);

	QString pyOutput = runPythonCode(pyInput).trimmed();

	if ( pyOutput == "direct" )
	{
		return Direct;
	}
	else if ( pyOutput == "indirect" )
	{
		return InDirect;
	}
	else
	{
		return Undefined;
	}
}

/**
* Makes the changes necessary for switching between Direct and Indirect interfaces.
* @param desired The interface format that is to be changed to.
*/
void ConvertToEnergy::changeInterface(DeltaEMode desired)
{
	QString curInstPrefix = m_uiForm.cbInst->itemData(m_uiForm.cbInst->currentIndex()).toString();;
	switch ( desired )
	{
	case Direct:
		m_uiForm.tabWidget->removeTab(m_uiForm.tabWidget->indexOf(m_uiForm.tabCalibration));
		m_uiForm.tabWidget->addTab(m_uiForm.tabDiagnoseDetectors, "Diagnose Detectors");
		m_uiForm.tabWidget->addTab(m_uiForm.tabAbsoluteUnits, "Absolute Units");

		if ( m_directInstruments == NULL )
		{
			m_directInstruments = new Homer(qobject_cast<QWidget*>(this->parent()), m_uiForm);
			m_directInstruments->initLayout();
			connect(m_directInstruments, SIGNAL(runAsPythonScript(const QString&)),
				this, SIGNAL(runAsPythonScript(const QString&)));
			m_directInstruments->initLocalPython();
		}
		m_directInstruments->setIDFValues(curInstPrefix);
		break;
	case InDirect:
		m_uiForm.tabWidget->removeTab(m_uiForm.tabWidget->indexOf(m_uiForm.tabDiagnoseDetectors));
		m_uiForm.tabWidget->removeTab(m_uiForm.tabWidget->indexOf(m_uiForm.tabAbsoluteUnits));
		m_uiForm.tabWidget->addTab(m_uiForm.tabCalibration, "Calibration");

		if ( m_indirectInstruments == NULL )
		{
			m_indirectInstruments = new Indirect(qobject_cast<QWidget*>(this->parent()), m_uiForm);
			m_indirectInstruments->initLayout();
			connect(m_directInstruments, SIGNAL(runAsPythonScript(const QString&)),
				this, SIGNAL(runAsPythonScript(const QString&)));
			m_indirectInstruments->initLocalPython();
		}
		m_indirectInstruments->setIDFValues(curInstPrefix);
		break;
	}

	m_uiForm.swInstrument->setCurrentIndex(desired);
	m_uiForm.swInputFiles->setCurrentIndex(desired);
	m_uiForm.swAnalysis->setCurrentIndex(desired);
	m_uiForm.swConvertToEnergy->setCurrentIndex(desired);
	m_uiForm.swRebin->setCurrentIndex(desired);
}


/**
* If the instrument selection has changed, calls instrumentSelectChanged
* @param prefix instrument name from QComboBox object
*/
void ConvertToEnergy::userSelectInstrument(const QString& prefix) {
	if ( prefix != m_curInterfaceSetup )
	{
		instrumentSelectChanged(prefix);
	}
}