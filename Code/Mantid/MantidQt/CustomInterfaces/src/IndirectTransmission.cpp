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

    QString sampleNo = m_uiForm.transInputFile->getFirstFilename();
    QString canNo = m_uiForm.transCanFile->getFirstFilename();

    IAlgorithm_sptr transAlg = AlgorithmManager::Instance().create("IndirectTransmissionReduction", -1);
    transAlg->initialize();
    transAlg->setProperty("SampleFile", sampleNo.toStdString());
    transAlg->setProperty("CanFile", canNo.toStdString());
    transAlg->setProperty("Verbose", m_uiForm.trans_ckVerbose->isChecked());
    transAlg->setProperty("Plot", m_uiForm.trans_ckPlot->isChecked());
    transAlg->setProperty("Save", m_uiForm.trans_ckSave->isChecked());

    runAlgorithm(transAlg);
  }

  bool IndirectTransmission::validate()
  {
    QString currentInst = m_uiForm.cbInst->currentText();

    //Check if we have an appropriate instrument
    if(currentInst != "IRIS" && currentInst != "OSIRIS")
    {
      return false;
    }

    //Check that the user has entered some file names
    if(m_uiForm.transInputFile->isEmpty()
        || m_uiForm.transCanFile->isEmpty())
    {
      return false;
    }

    //Check if we have a file problem
    QString errorInputFile = m_uiForm.transInputFile->getFileProblem();
    QString errorCanFile = m_uiForm.transCanFile->getFileProblem();

    if(!errorInputFile.isEmpty() || !errorCanFile.isEmpty())
    {
      return false;
    }

    return true;
  }

} // namespace CustomInterfaces
} // namespace Mantid
