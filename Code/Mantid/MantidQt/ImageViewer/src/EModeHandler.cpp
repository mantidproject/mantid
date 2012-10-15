
#include <iostream>
#include <QLineEdit>

#include "MantidQtImageViewer/EModeHandler.h"
#include "MantidQtImageViewer/QtUtils.h"
#include "MantidQtImageViewer/IVUtils.h"
#include "MantidQtImageViewer/ErrorHandler.h"

namespace MantidQt
{
namespace ImageView
{

/**
 *  Construct an EModeHandler object to manage the E Mode and E Fixed controls 
 *  in the specified UI
 */
EModeHandler::EModeHandler( Ui_ImageViewer* iv_ui )
{
  this->iv_ui = iv_ui;
}


/**
 */
int EModeHandler::GetEMode()
{
  return iv_ui->emode_combo_box->currentIndex();
}

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


double EModeHandler::GetEFixed()
{
  double efixed;
  std::string text = iv_ui->efixed_control->text().toStdString();
  if ( !IVUtils::StringToDouble( text, efixed ) )
  {
    ErrorHandler::Error("E Fixed is not a NUMBER! Value reset to default.");
    efixed = 0;
  }

  SetEFixed( efixed );
  return efixed;
}


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


} // namespace MantidQt 
} // namespace ImageView
