#include "MantidQtCustomInterfaces/C2ETab.h"

namespace MantidQt
{
namespace CustomInterfaces
{

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  C2ETab::C2ETab(Ui::ConvertToEnergy& uiForm, QWidget * parent) : QWidget(parent),
      m_uiForm(uiForm)
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  C2ETab::~C2ETab()
  {
  }
  
  void C2ETab::runTab()
  {
    run();
  }

  void C2ETab::setupTab()
  {
    setup();
  }

  void C2ETab::validateTab()
  {
    validate();
  }


} // namespace CustomInterfaces
} // namespace Mantid
