#include <iostream>
#include <QLineEdit>

#include "MantidQtSpectrumViewer/EModeHandler.h"
#include "MantidQtSpectrumViewer/QtUtils.h"
#include "MantidKernel/Logger.h"


namespace
{
  Mantid::Kernel::Logger g_log("SpectrumView");
}


namespace MantidQt
{
namespace SpectrumView
{

/**
 *  Construct an EModeHandler object to manage the E Mode and E Fixed controls
 *  in the specified UI
 */
EModeHandler::EModeHandler( Ui_SpectrumViewer* svUI ):
  m_svUI(svUI)
{
}


/**
 * Get the EMode value (0,1,2) from the GUI.
 */
int EModeHandler::getEMode()
{
  return m_svUI->emode_combo_box->currentIndex();
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
void EModeHandler::setEMode( const int mode )
{
  if(mode >= 0 && mode <= 2)
    m_svUI->emode_combo_box->setCurrentIndex( mode );
  else
    g_log.error() << "Mode number invalid: " << QString::number(mode).toStdString() << std::endl;
}

/**
 *  Return the user specified EFixed value, OR 0, if no valid
 *  EFixed value was set.
 */
double EModeHandler::getEFixed()
{
  QString text = m_svUI->efixed_control->text();
  bool isNumber = false;
  double eFixed = text.toDouble(&isNumber);
  if(!isNumber)
  {
    g_log.information("E Fixed is not a NUMBER! Value reset to default.");
    eFixed = 0;
  }
  else if ( eFixed < 0 )
  {
    g_log.information("E Fixed is negative, Value reset to default.");
    eFixed = 0;
  }

  setEFixed( eFixed );
  return eFixed;
}


/**
 *  Set the EFixed value that is displayed in the UI.
 *
 *  @param eFixed  The new efixed value to display in the
 *                 UI.  This must be positive, or the
 *                 displayed value will be set to zero.
 */
void EModeHandler::setEFixed( const double eFixed )
{
  double newValue = eFixed;
  if ( eFixed < 0 )
  {
    g_log.information("E Fixed is negative, reset to default.");
    newValue = 0;
  }

  QtUtils::SetText( 10, 4, newValue, m_svUI->efixed_control );
}


} // namespace SpectrumView
} // namespace MantidQt
