#include "MantidKernel/ConfigService.h"
#include "MantidQtAPI/ManageUserDirectories.h"
#include "MantidQtCustomInterfaces/IndirectForeign.h"
#include "MantidQtCustomInterfaces/ForCE.h"
#include "MantidQtCustomInterfaces/MolDyn.h"

#include <QDesktopServices>
#include <QUrl>

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
  namespace CustomInterfaces
  {
    DECLARE_SUBWINDOW(IndirectForeign);
  }
}

using namespace MantidQt::CustomInterfaces;

IndirectForeign::IndirectForeign(QWidget *parent) : UserSubWindow(parent)
{

}

void IndirectForeign::initLayout()
{
	m_uiForm.setupUi(this);

	//insert each tab into the interface on creation
	m_foreignTabs.insert(std::make_pair(FORCE, new ForCE(m_uiForm.IndirectForeignTabs->widget(FORCE))));
	m_foreignTabs.insert(std::make_pair(MOLDYN, new MolDyn(m_uiForm.IndirectForeignTabs->widget(MOLDYN))));

	//Connect each tab to the actions available in this GUI
	std::map<unsigned int, IndirectForeignTab*>::iterator iter;
	for (iter = m_foreignTabs.begin(); iter != m_foreignTabs.end(); ++iter)
	{
		connect(iter->second, SIGNAL(executePythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
		connect(iter->second, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));
		
	}

	loadSettings();

	//Connect statements for the buttons shared between all tabs on the Indirect Bayes interface
	connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
	connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(helpClicked()));
	connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this, SLOT(manageUserDirectories()));
}

/**
 * Load the setting for each tab on the interface.
 *
 * This includes setting the default browsing directory to be the default save directory.
 */
void IndirectForeign::loadSettings()
{
  QSettings settings;
  QString settingsGroup = "CustomInterfaces/IndirectAnalysis/";
  QString saveDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));

  settings.beginGroup(settingsGroup + "ProcessedFiles");
  settings.setValue("last_directory", saveDir);

	std::map<unsigned int, IndirectForeignTab*>::iterator iter;
	for (iter = m_foreignTabs.begin(); iter != m_foreignTabs.end(); ++iter)
	{
  	iter->second->loadSettings(settings);
  }

  settings.endGroup();
}


/**
 * Slot to run the underlying algorithm code based on the currently selected
 * tab.
 * 
 * This method checks the tabs validate method is passing before calling
 * the run method.
 */
void IndirectForeign::runClicked()
{
	int tabIndex = m_uiForm.IndirectForeignTabs->currentIndex();

	if(m_foreignTabs[tabIndex]->validate())
	{
		m_foreignTabs[tabIndex]->run();
	}
}

/**
 * Slot to open a new browser window and navigate to the help page
 * on the wiki for the currently selected tab.
 */
void IndirectForeign::helpClicked()
{
	int tabIndex = m_uiForm.IndirectForeignTabs->currentIndex();
	QString url = m_foreignTabs[tabIndex]->tabHelpURL();
	QDesktopServices::openUrl(QUrl(url));
}

/**
 * Slot to show the manage user dicrectories dialog when the user clicks
 * the button on the interface.
 */
void IndirectForeign::manageUserDirectories()
{
  MantidQt::API::ManageUserDirectories *ad = new MantidQt::API::ManageUserDirectories(this);
  ad->show();
  ad->setFocus();
}

/**
 * Slot to wrap the protected showInformationBox method defined
 * in UserSubWindow and provide access to composed tabs.
 * 
 * @param message :: The message to display in the message box
 */
void IndirectForeign::showMessageBox(const QString& message)
{
  showInformationBox(message);
}

IndirectForeign::~IndirectForeign()
{
}
