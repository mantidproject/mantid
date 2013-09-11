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
}

void IndirectBayes::initLayout()
{
}

IndirectBayes::~IndirectBayes()
{
}
