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
    QString sampleNo = m_uiForm.transInputFile->getFirstFilename();
    QString canNo = m_uiForm.transCanFile->getFirstFilename();

    QFileInfo finfo(sampleNo);
    QString inst = finfo.baseName();

    //flags for various algorithm options
    QString verbose("False");
    QString save("False");
    QString plot("False");

    if(m_uiForm.trans_ckVerbose->isChecked())
    {
      verbose  = "True";
    }

    if(m_uiForm.trans_ckSave->isChecked())
    {
      save = "True";
    }

    if(m_uiForm.trans_ckPlot->isChecked())
    {
      plot = "True";
    }

    QString pyInput =
        "from IndirectEnergyConversion import IndirectTrans \n"
        "IndirectTrans('"+inst+"','"+sampleNo+"','"+canNo+"',"
        "Verbose="+verbose+","
        "Plot="+plot+","
        "Save="+save+""
        ")\n";

    emit runAsPythonScript(pyInput, true);
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
