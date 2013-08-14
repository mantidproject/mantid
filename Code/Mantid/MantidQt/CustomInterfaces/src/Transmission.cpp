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
  }

  QString Transmission::validate()
  {
    return NULL;
  }

} // namespace CustomInterfaces
} // namespace Mantid
