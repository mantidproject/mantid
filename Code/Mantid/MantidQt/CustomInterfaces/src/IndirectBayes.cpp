#include "MantidQtAPI/ManageUserDirectories.h"
#include "MantidQtCustomInterfaces/IndirectBayes.h"
#include "MantidQtCustomInterfaces/JumpFit.h"
#include "MantidQtCustomInterfaces/Quasi.h"
#include "MantidQtCustomInterfaces/ResNorm.h"
#include "MantidQtCustomInterfaces/Stretch.h"

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
  namespace CustomInterfaces
  {
    DECLARE_SUBWINDOW(IndirectBayes);
  }
}

using namespace MantidQt::CustomInterfaces;

IndirectBayes::IndirectBayes(QWidget *parent) : UserSubWindow(parent)
{
	m_uiForm.setupUi(this);

	//insert each tab into the interface on creation
	m_bayesTabs.insert(std::make_pair(RES_NORM, new ResNorm(m_uiForm.indirectBayesTabs->widget(RES_NORM))));
	m_bayesTabs.insert(std::make_pair(QUASI, new Quasi(m_uiForm.indirectBayesTabs->widget(QUASI))));
	m_bayesTabs.insert(std::make_pair(STRETCH, new Stretch(m_uiForm.indirectBayesTabs->widget(STRETCH))));
	m_bayesTabs.insert(std::make_pair(JUMP_FIT, new JumpFit(m_uiForm.indirectBayesTabs->widget(JUMP_FIT))));

	//Connect each tab to the python execution method
	std::map<unsigned int, IndirectBayesTab*>::iterator iter;
	for (iter = m_bayesTabs.begin(); iter != m_bayesTabs.end(); ++iter)
	{
		connect(iter->second, SIGNAL(executePythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
	}

	connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
	connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(helpClicked()));
	connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this, SLOT(manageUserDirectories()));
}

void IndirectBayes::initLayout()
{
}

void IndirectBayes::runClicked()
{
	int tabIndex = m_uiForm.indirectBayesTabs->currentIndex();

	if(m_bayesTabs[tabIndex]->validate())
	{
		m_bayesTabs[tabIndex]->run();
	}
}

void IndirectBayes::helpClicked()
{
	int tabIndex = m_uiForm.indirectBayesTabs->currentIndex();
	QString url = m_bayesTabs[tabIndex]->tabHelpURL();
	QDesktopServices::openUrl(QUrl(url));
}

void IndirectBayes::manageUserDirectories()
{
  MantidQt::API::ManageUserDirectories *ad = new MantidQt::API::ManageUserDirectories(this);
  ad->show();
  ad->setFocus();
}

IndirectBayes::~IndirectBayes()
{
}
