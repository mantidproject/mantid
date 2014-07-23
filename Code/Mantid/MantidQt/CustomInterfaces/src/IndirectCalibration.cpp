#include "MantidQtCustomInterfaces/IndirectCalibration.h"

namespace MantidQt
{
namespace CustomInterfaces
{
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  IndirectCalibration::IndirectCalibration(Ui::IndirectDataReduction& uiForm, QWidget * parent) :
      IndirectDataReductionTab(uiForm, parent)
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  IndirectCalibration::~IndirectCalibration()
  {
  }
  
  void IndirectCalibration::setup()
  {
  }

  void IndirectCalibration::run()
  {
  }

  bool IndirectCalibration::validate()
  {
  }

} // namespace CustomInterfaces
} // namespace Mantid
