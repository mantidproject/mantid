#include "MantidKernel/ConfigService.h"
#include "MantidQtAPI/HelpWindow.h"
#include "MantidQtAPI/ManageUserDirectories.h"
#include "MantidQtCustomInterfaces/Indirect/DensityOfStates.h"
#include "MantidQtCustomInterfaces/Indirect/IndirectSimulation.h"
#include "MantidQtCustomInterfaces/Indirect/IndirectMolDyn.h"
#include "MantidQtCustomInterfaces/Indirect/IndirectSassena.h"

#include <QDesktopServices>
#include <QUrl>

// Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
  namespace CustomInterfaces
  {
    DECLARE_SUBWINDOW(IndirectSimulation);
  }
}

using namespace MantidQt::CustomInterfaces;

IndirectSimulation::IndirectSimulation(QWidget *parent) : UserSubWindow(parent),
	m_changeObserver(*this, &IndirectSimulation::handleDirectoryChange)
{
}

IndirectSimulation::~IndirectSimulation()
{
}

void IndirectSimulation::initLayout()
{
	m_uiForm.setupUi(this);

  // Connect Poco Notification Observer
  Mantid::Kernel::ConfigService::Instance().addObserver(m_changeObserver);

	// Insert each tab into the interface on creation
	m_simulationTabs.insert(std::make_pair(MOLDYN, new IndirectMolDyn(m_uiForm.IndirectSimulationTabs->widget(MOLDYN))));
	m_simulationTabs.insert(std::make_pair(SASSENA, new IndirectSassena(m_uiForm.IndirectSimulationTabs->widget(SASSENA))));
	m_simulationTabs.insert(std::make_pair(DOS, new DensityOfStates(m_uiForm.IndirectSimulationTabs->widget(DOS))));

	// Connect each tab to the actions available in this GUI
	std::map<unsigned int, IndirectSimulationTab*>::iterator iter;
	for (iter = m_simulationTabs.begin(); iter != m_simulationTabs.end(); ++iter)
	{
		connect(iter->second, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
		connect(iter->second, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));
	}

	loadSettings();

	// Connect statements for the buttons shared between all tabs on the Indirect Bayes interface
	connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
	connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(helpClicked()));
	connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this, SLOT(manageUserDirectories()));
}

  /**
   * Handles closing the window.
   *
   * @param :: the detected close event
   */
  void IndirectSimulation::closeEvent(QCloseEvent*)
  {
    Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);
  }

  /**
   * Handles a change in directory.
   *
   * @param pNf :: notification
   */
  void IndirectSimulation::handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf)
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
void IndirectSimulation::loadSettings()
{
  QSettings settings;
  QString settingsGroup = "CustomInterfaces/IndirectAnalysis/";
  QString saveDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));

  settings.beginGroup(settingsGroup + "ProcessedFiles");
  settings.setValue("last_directory", saveDir);

	std::map<unsigned int, IndirectSimulationTab*>::iterator iter;
	for (iter = m_simulationTabs.begin(); iter != m_simulationTabs.end(); ++iter)
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
void IndirectSimulation::runClicked()
{
	int tabIndex = m_uiForm.IndirectSimulationTabs->currentIndex();
  m_simulationTabs[tabIndex]->runTab();
}

/**
 * Slot to open a new browser window and navigate to the help page
 * on the wiki for the currently selected tab.
 */
void IndirectSimulation::helpClicked()
{
  MantidQt::API::HelpWindow::showCustomInterface(NULL, QString("Indirect_Simulation"));
}

/**
 * Slot to show the manage user dicrectories dialog when the user clicks
 * the button on the interface.
 */
void IndirectSimulation::manageUserDirectories()
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
void IndirectSimulation::showMessageBox(const QString& message)
{
  showInformationBox(message);
}
