#include "MantidQtCustomInterfaces/IndirectBayesTab.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  IndirectBayesTab::IndirectBayesTab(Ui::ConvertToEnergy& uiForm, QWidget * parent) : QWidget(parent),
      m_uiForm(uiForm)
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  IndirectBayesTab::~IndirectBayesTab(QWidget* parent)
  {
  }

} // namespace MantidQt
