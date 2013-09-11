
#include <iostream>
#include <QLineEdit>

#include "MantidQtSpectrumViewer/EModeHandler.h"
#include "MantidQtSpectrumViewer/QtUtils.h"
#include "MantidQtSpectrumViewer/SVUtils.h"
#include "MantidQtSpectrumViewer/ErrorHandler.h"

namespace MantidQt
{
namespace SpectrumView
{

/**
 *  Construct an EModeHandler object to manage the E Mode and E Fixed controls 
 *  in the specified UI
 */
EModeHandler::EModeHandler( Ui_SpectrumViewer* iv_ui )
{
  this->iv_ui = iv_ui;
}


/**
 * Get the EMode value (0,1,2) from the GUI.
 */
int EModeHandler::GetEMode()
{
  return iv_ui->emode_combo_box->currentIndex();
}


/**
 * Set the EMode to display in the GUI.
 *
 * @param mode   Integer code for the emode type, 
 *               0 = Diffractometer
 *               1 = Direct Geometry Spectrometer
 *               2 = Indirect Geometry Spectrometer
 *               NOTE: Any other value will be interpreted as 0
 *               and the gui will not be changed.
 */
void EModeHandler::SetEMode( const int mode )
{
  if ( mode >= 0 && mode <= 2 )
  {
     iv_ui->emode_combo_box->setCurrentIndex( mode );
  }
  else
  {
    ErrorHandler::Error("Mode number invalid: " + mode );
  }
}


/**
 *  Return the user specified EFixed value, OR 0, if no valid 
 *  EFixed value was set.
 */
double EModeHandler::GetEFixed()
{
  double efixed;
  std::string text = iv_ui->efixed_control->text().toStdString();
  if ( !SVUtils::StringToDouble( text, efixed ) )
  {
    ErrorHandler::Error("E Fixed is not a NUMBER! Value reset to default.");
    efixed = 0;
  }
  else if ( efixed < 0 )
  {
    ErrorHandler::Error("E Fixed is negative, Value reset to default.");
    efixed = 0;
  }

  SetEFixed( efixed );
  return efixed;
}


/**
 *  Set the EFixed value that is displayed in the UI.
 *
 *  @param efixed  The new efixed value to display in the
 *                 UI.  This must be positive, or the
 *                 displayed value will be set to zero.
 */
void EModeHandler::SetEFixed( const double efixed )
{
  double new_value = efixed;
  if ( efixed < 0 )
  {
    ErrorHandler::Error("E Fixed is negative, reset to default.");
    new_value = 0;
  }

  QtUtils::SetText( 10, 4, new_value, iv_ui->efixed_control );
}


} // namespace SpectrumView
} // namespace MantidQt 
