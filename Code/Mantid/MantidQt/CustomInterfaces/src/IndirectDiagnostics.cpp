#include "MantidQtCustomInterfaces/IndirectDiagnostics.h"

namespace MantidQt
{
namespace CustomInterfaces
{
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  IndirectDiagnostics::IndirectDiagnostics(Ui::IndirectDataReduction& uiForm, QWidget * parent) :
      IndirectDataReductionTab(uiForm, parent)
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  IndirectDiagnostics::~IndirectDiagnostics()
  {
  }
  
  void IndirectDiagnostics::setup()
  {
  }

  void IndirectDiagnostics::run()
  {
  }

  bool IndirectDiagnostics::validate()
  {
  }

} // namespace CustomInterfaces
} // namespace Mantid
