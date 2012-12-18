#include "MantidQtRefDetectorViewer/RefLimitsHandler.h"

namespace MantidQt
{
namespace RefDetectorViewer
{

RefLimitsHandler::RefLimitsHandler( Ui_RefImageViewer* iv_ui ) : m_ui(iv_ui)
{
}

/**
 * return the value of the peak left
 **/
int RefLimitsHandler::getPeakLeft() const
{
  return m_ui->lineEdit_peakLeft->text().toInt();
}

/**
 * return the value of the peak right
 **/
int RefLimitsHandler::getPeakRight() const
{
  return m_ui->lineEdit_peakRight->text().toInt();
}

/**
 * return the value of the back left
 **/
int RefLimitsHandler::getBackLeft() const
{
  return m_ui->lineEdit_backLeft->text().toInt();
}

/**
 * return the value of the back right
 **/
int RefLimitsHandler::getBackRight() const
{
  return m_ui->lineEdit_backRight->text().toInt();
}

/**
 * return the value of the TOF min
 **/
int RefLimitsHandler::getTOFmin() const
{
  return m_ui->lineEdit_TOFmin->text().toInt();
}

/**
 * return the value of the TOF max
 **/
int RefLimitsHandler::getTOFmax() const
{
  return m_ui->lineEdit_TOFmax->text().toInt();
}

/**
 * set the peak left
 */
void RefLimitsHandler::setPeakLeft(const int value)
{
  m_ui->lineEdit_peakLeft->setText(QString::number(value));
}

/**
 * set the peak right
 */
void RefLimitsHandler::setPeakRight(const int value)
{
  m_ui->lineEdit_peakRight->setText(QString::number(value));
}

/**
 * set the back left
 */
void RefLimitsHandler::setBackLeft(const int value)
{
  m_ui->lineEdit_backLeft->setText(QString::number(value));
}

/**
 * set the back right
 */
void RefLimitsHandler::setBackRight(const int value)
{
  m_ui->lineEdit_backRight->setText(QString::number(value));
}

/**
 * set the TOF min
 */
void RefLimitsHandler::setTOFmin(const int value)
{
  m_ui->lineEdit_TOFmin->setText(QString::number(value));
}

/**
 * set the TOF max
 */
void RefLimitsHandler::setTOFmax(const int value)
{
  m_ui->lineEdit_TOFmax->setText(QString::number(value));
}

/** Update the currently active line edit with the coordinate passed in
 *  @param x An x coordinate - pertains to TOF lines
 *  @param y A y coordinate - pertains to peak & background lines
 */
void RefLimitsHandler::setActiveValue(const double x, const double y)
{
  if (m_ui->radioButton_peakLeft->isChecked()) { //peak left selected
    setPeakLeft(y);
  }
  else if (m_ui->radioButton_peakRight->isChecked()) { //peak right selected
    setPeakRight(y);
  }
  else if (m_ui->radioButton_backLeft->isChecked()) { //back left selected
    setBackLeft(y);
  }
  else if (m_ui->radioButton_backRight->isChecked()) { //back right selected
    setBackRight(y);
  }
  else if (m_ui->radioButton_TOFmin->isChecked()) { //tof min selected
    setTOFmin(x);
  }
  else if (m_ui->radioButton_TOFmax->isChecked()) { // tof max selected
    setTOFmax(x);
  }

}

} // namespace RefDetectorViewer
} // namespace MantidQt
