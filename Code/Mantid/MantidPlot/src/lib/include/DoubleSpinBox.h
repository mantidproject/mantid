/***************************************************************************
    File                 : DoubleSpinBox.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2007-2008 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : A Double Spin Box

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef DoubleSpinBox_H
#define DoubleSpinBox_H

#include <map>

#include <QAbstractSpinBox>
#include <QCheckBox>

//! A QDoubleSpinBox allowing to customize numbers display with respect to locale settings. 
/**
 * It allows the user to specify a custom display format.
 */
class DoubleSpinBox : public QAbstractSpinBox
{
  Q_OBJECT

public:
  //! Constructor.
  /**
   * \param format format used to display numbers: has the same meaning as in QLocale::toString ( double i, char f = 'g', int prec = 6 )
   * \param parent parent widget (only affects placement of the dialog)
   */
  DoubleSpinBox(const char format = 'g', QWidget * parent = 0);

  void setSingleStep(double val);
  void setMaximum(double max);
  void setMinimum(double min);
  void setRange(double min, double max);

  double getMaximum();
  double getMinimum();

  int decimals(){return d_prec;};
  void setDecimals(int prec){if (prec >= 0) d_prec = prec;};

  double value();
  bool setValue(double val);

  void setFormat(const char format, int prec = 1){d_format = format; setDecimals(prec);};

  void addSpecialTextMapping(QString text, double value);

  QString textFromValue ( double value ) const;
  virtual QValidator::State validate ( QString & input, int & pos ) const;

  signals:
  void valueChanged ( double d );
  //! Signal emitted when the spin box gains focus
  void activated(DoubleSpinBox *);

private slots:
  void interpretText(bool notify=true);

protected:
  void stepBy ( int steps );
  StepEnabled stepEnabled () const;
  void focusInEvent(QFocusEvent *);

private:
  char d_format;
  double d_min_val;
  double d_max_val;
  double d_value;
  double d_step;
  int d_prec;

  //A set of mappins from strings which the user can enter in the box to double values
  std::map<QString, double> m_specialTextMappings;
};

//! A checkable DoubleSpinBox that can be used to select the limits of a double interval. 
class RangeLimitBox : public QWidget
{
public:
  enum LimitType{LeftLimit, RightLimit};

  RangeLimitBox(LimitType type, QWidget * parent = 0);
  void setDecimals(int prec){d_spin_box->setDecimals(prec);};
  double value();
  bool isChecked(){return d_checkbox->isChecked();};

private:
  DoubleSpinBox *d_spin_box;
  QCheckBox *d_checkbox;
  LimitType d_type;
};
#endif // FITDIALOG_H
