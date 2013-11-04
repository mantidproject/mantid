#include "MantidKernel/ConfigService.h"
#include "MantidQtAPI/ManageUserDirectories.h"
#include "MantidQtCustomInterfaces/IndirectLoadAscii.h"
#include "MantidQtCustomInterfaces/IndirectNeutron.h"
#include "MantidQtCustomInterfaces/IndirectMolDyn.h"

#include <QDesktopServices>
#include <QUrl>

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
  namespace CustomInterfaces
  {
    DECLARE_SUBWINDOW(IndirectLoadAscii);
  }
}

using namespace MantidQt::CustomInterfaces;

IndirectLoadAscii::IndirectLoadAscii(QWidget *parent) : UserSubWindow(parent),
	m_changeObserver(*this, &IndirectLoadAscii::handleDirectoryChange)
{

}

void IndirectLoadAscii::initLayout()
{
	m_uiForm.setupUi(this);
	
  // Connect Poco Notification Observer
  Mantid::Kernel::ConfigService::Instance().addObserver(m_changeObserver);

	//insert each tab into the interface on creation
	m_loadAsciiTabs.insert(std::make_pair(NEUTRON, new IndirectNeutron(m_uiForm.IndirectLoadAsciiTabs->widget(NEUTRON))));
	m_loadAsciiTabs.insert(std::make_pair(MOLDYN, new IndirectMolDyn(m_uiForm.IndirectLoadAsciiTabs->widget(MOLDYN))));

	//Connect each tab to the actions available in this GUI
	std::map<unsigned int, IndirectLoadAsciiTab*>::iterator iter;
	for (iter = m_loadAsciiTabs.begin(); iter != m_loadAsciiTabs.end(); ++iter)
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
   * Handles closing the window.
   *
   * @param :: the detected close event
   */
  void IndirectLoadAscii::closeEvent(QCloseEvent*)
  {
    Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);
  }

  /**
   * Handles a change in directory.
   *
   * @param pNf :: notification
   */
  void IndirectLoadAscii::handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf)
  {
    std::string key = pNf->key();
    if ( key == "defaultsave.directory" )
    {
      loadSettings();
    }
  }

/**
 * Load the setting for each tab on the interface.
 *
 * This includes setting the default browsing directory to be the default save directory.
 */
void IndirectLoadAscii::loadSettings()
{
  QSettings settings;
  QString settingsGroup = "CustomInterfaces/IndirectAnalysis/";
  QString saveDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));

  settings.beginGroup(settingsGroup + "ProcessedFiles");
  settings.setValue("last_directory", saveDir);

	std::map<unsigned int, IndirectLoadAsciiTab*>::iterator iter;
	for (iter = m_loadAsciiTabs.begin(); iter != m_loadAsciiTabs.end(); ++iter)
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
void IndirectLoadAscii::runClicked()
{
	int tabIndex = m_uiForm.IndirectLoadAsciiTabs->currentIndex();

	if(m_loadAsciiTabs[tabIndex]->validate())
	{
		m_loadAsciiTabs[tabIndex]->run();
	}
}

/**
 * Slot to open a new browser window and navigate to the help page
 * on the wiki for the currently selected tab.
 */
void IndirectLoadAscii::helpClicked()
{
	int tabIndex = m_uiForm.IndirectLoadAsciiTabs->currentIndex();
	QString url = m_loadAsciiTabs[tabIndex]->tabHelpURL();
	QDesktopServices::openUrl(QUrl(url));
}

/**
 * Slot to show the manage user dicrectories dialog when the user clicks
 * the button on the interface.
 */
void IndirectLoadAscii::manageUserDirectories()
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
void IndirectLoadAscii::showMessageBox(const QString& message)
{
  showInformationBox(message);
}

IndirectLoadAscii::~IndirectLoadAscii()
{
}
