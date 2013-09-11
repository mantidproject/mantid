#include "MantidQtCustomInterfaces/IndirectBayes.h"
#include "MantidQtCustomInterfaces/ResNorm.h"

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
}

void IndirectBayes::initLayout()
{
}

IndirectBayes::~IndirectBayes()
{
}
