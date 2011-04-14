//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidQtCustomInterfaces/MuonAnalysisHelper.h"
#include "MantidKernel/ConfigService.h"

#include "MantidQtAPI/UserSubWindow.h"

#include <boost/shared_ptr.hpp>
#include <fstream>  
//-----------------------------------------------------------------------------


namespace MantidQt
{
namespace CustomInterfaces
{
namespace Muon
{
  using namespace MantidQt::API;
  using namespace Mantid::Kernel;


/**
 * Add Greek letter to label from code 
 *
 */
void createMicroSecondsLabels(Ui::MuonAnalysis& m_uiForm)
{
  //the unicode code for the mu symbol is 956, doing the below keeps this file ASCII compatible
  static const QChar MU_SYM(956);
  m_uiForm.Time_Zero_label->setText(m_uiForm.Time_Zero_label->text() + QString(" (%1s)").arg(MU_SYM));
  m_uiForm.First_Good_Data_label->setText(m_uiForm.First_Good_Data_label->text() + QString(" (%1s)").arg(MU_SYM));
  m_uiForm.timeAxisStartAtLabel->setText(m_uiForm.timeAxisStartAtLabel->text() + QString(" (%1s)").arg(MU_SYM));
  m_uiForm.timeAxisFinishAtLabel->setText(m_uiForm.timeAxisFinishAtLabel->text() + QString(" (%1s)").arg(MU_SYM));
}

}
}
}
