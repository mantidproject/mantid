#include "MantidQtCustomInterfaces/IndirectTransmission.h"

#include <QFileInfo>

namespace MantidQt
{
namespace CustomInterfaces
{

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  IndirectTransmission::IndirectTransmission(Ui::IndirectDataReduction& uiForm, QWidget * parent) :
      IndirectDataReductionTab(uiForm, parent)
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  IndirectTransmission::~IndirectTransmission()
  {

  }
  
  void IndirectTransmission::setup()
  {
  }

  void IndirectTransmission::run()
  {
    using namespace Mantid::API;

    QString sampleWsName = m_uiForm.trans_dsSampleInput->getCurrentDataName();
    QString canWsName = m_uiForm.trans_dsCanInput->getCurrentDataName();

    IAlgorithm_sptr transAlg = AlgorithmManager::Instance().create("IndirectTransmissionMonitor", -1);
    transAlg->initialize();

    transAlg->setProperty("SampleWorkspace", sampleWsName.toStdString());
    transAlg->setProperty("CanWorkspace", canWsName.toStdString());

    transAlg->setProperty("Verbose", m_uiForm.trans_ckVerbose->isChecked());
    transAlg->setProperty("Plot", m_uiForm.trans_ckPlot->isChecked());
    transAlg->setProperty("Save", m_uiForm.trans_ckSave->isChecked());

    runAlgorithm(transAlg);
  }

  bool IndirectTransmission::validate()
  {
    // Check if we have an appropriate instrument
    QString currentInst = m_uiForm.cbInst->currentText();
    if(currentInst != "IRIS" && currentInst != "OSIRIS")
      return false;

    // Check for an invalid sample input
    if(!m_uiForm.trans_dsSampleInput->isValid())
      return false;

    // Check for an invalid can input
    if(!m_uiForm.trans_dsCanInput->isValid())
      return false;

    return true;
  }

} // namespace CustomInterfaces
} // namespace Mantid
