#include "MantidQtCustomInterfaces/IndirectConvertToEnergy.h"

namespace MantidQt
{
namespace CustomInterfaces
{
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  IndirectConvertToEnergy::IndirectConvertToEnergy(Ui::IndirectDataReduction& uiForm, QWidget * parent) :
      IndirectDataReductionTab(uiForm, parent)
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  IndirectConvertToEnergy::~IndirectConvertToEnergy()
  {
  }
  
  void IndirectConvertToEnergy::setup()
  {
  }

  void IndirectConvertToEnergy::run()
  {
  }

  bool IndirectConvertToEnergy::validate()
  {
  }

} // namespace CustomInterfaces
} // namespace Mantid
