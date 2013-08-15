#include "MantidQtCustomInterfaces/Transmission.h"

namespace MantidQt
{
namespace CustomInterfaces
{

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  Transmission::Transmission(Ui::ConvertToEnergy& uiForm, QWidget * parent) :
      C2ETab(uiForm, parent)
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  Transmission::~Transmission()
  {

  }
  
  void Transmission::setup()
  {
  }

  void Transmission::run()
  {
    QString inst = m_uiForm.cbInst->currentText().lower();
    QString sampleNo = m_uiForm.transInputFile->getFirstFilename();
    QString canNo = m_uiForm.transCanFile->getFirstFilename();

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
        "IDAtransmission("
        "Instrument='"+inst+"',"
        "SamNumber='"+sampleNo+"',"
        "CanNumber='"+canNo+"',"
        "Verbose="+verbose+","
        "Plot="+plot+","
        "Save="+save+""
        ")\n";

    emit runAsPythonScript(pyInput, false);
  }

  bool Transmission::validate()
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

    return true;
  }

} // namespace CustomInterfaces
} // namespace Mantid
