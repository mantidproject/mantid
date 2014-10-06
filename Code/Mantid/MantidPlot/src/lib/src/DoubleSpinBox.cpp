/***************************************************************************
 File                 : DoubleSpinBox.cpp
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
#include "DoubleSpinBox.h"

#include <MyParser.h>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QCloseEvent>
#include <float.h>
#include <math.h>

DoubleSpinBox::DoubleSpinBox(const char format, QWidget * parent) :
  QAbstractSpinBox(parent), d_format(format), d_min_val(-DBL_MAX), d_max_val(DBL_MAX), d_value(0.0),
  d_step(0.1), d_prec(14)
{
  if (format == 'f')
    d_prec = 1;

  setFocusPolicy(Qt::StrongFocus);
  lineEdit()->setText(locale().toString(d_value, d_format, d_prec));
  setWrapping(false);
  connect(this, SIGNAL(editingFinished()), this, SLOT(interpretText()));
}

void DoubleSpinBox::setSingleStep(double val)
{
  if (d_step != val && val < d_max_val)
    d_step = val;
}

void DoubleSpinBox::setMaximum(double max)
{
  if (max == d_max_val || max > DBL_MAX)
    return;

  d_max_val = max;
}

double DoubleSpinBox::getMaximum()
{
  return d_max_val;
}

void DoubleSpinBox::setMinimum(double min)
{
  if (min == d_min_val || min < -DBL_MAX)
    return;

  d_min_val = min;
}

double DoubleSpinBox::getMinimum()
{
  return d_min_val;
}

void DoubleSpinBox::setRange(double min, double max)
{
  setMinimum(min);
  setMaximum(max);
}

/**
 * Interpret the text and update the stored value.
 * @param notify If true then emit signals to indicate if the value has changed (default=true)
 * The default is important so that connected signals ensure the correct updates are pushed
 * through but we need to be able to turn them off as there are cases where this causes
 * a recursive call.
 */
void DoubleSpinBox::interpretText(bool notify)
{
// RJT: Keep our version of this, which contains a bug fix (see [10521]).
// Also, there are lines referring to methods that don't exist in our (older) MyParser class.
  bool ok = false;
  double value = locale().toDouble(text(), &ok);
  if (ok && setValue(value))
  {
    if(notify) emit valueChanged(d_value);
  }
  else
  {
    QString val = text().remove(",");
    value = locale().toDouble(val, &ok);
    if ( ok && setValue(value) )
    {
      if(notify) emit valueChanged(d_value);
    }
    else
    {
      //Check for any registered test strings that map to a given value
      for(auto it = m_specialTextMappings.begin(); it != m_specialTextMappings.end(); ++it)
      {
        if(it->first == text())
        {
          //Found a matching string, try to set the value
          if(setValue(it->second))
          {
            lineEdit()->setText(text());
            if(notify)
              emit valueChanged(d_value);
          }
        }
      }

      lineEdit()->setText(textFromValue(d_value));
    }
  }

//  bool ok = false;
//  QString s = text();
//  double value = locale().toDouble(s, &ok);
//  if (ok && value == d_value)
//    return;
//
//  if (!ok)
//  {
//    MyParser parser;
//    parser.setLocale(QLocale());
//    parser.addGSLConstants();
//    try
//    {
//      parser.SetExpr(s.toAscii().constData());
//      value = parser.Eval();
//    } catch (mu::ParserError &e)
//    {
//      lineEdit()->setText(textFromValue(d_value));
//      return;
//    }
//  }
//
//  if (setValue(value))
//    emit valueChanged( d_value);
//  else
//    lineEdit()->setText(textFromValue(d_value));
}

/**
 * Adds a mapping from string whihc may be entered into the edit box and a double value.
 * The mapping is case sensitive
 *
 * @param text QString with text to map
 * @param value Value to map it to
 */
void DoubleSpinBox::addSpecialTextMapping(QString text, double value)
{
  m_specialTextMappings[text] = value;
}

void DoubleSpinBox::stepBy(int steps)
{
  double val = d_value + steps * d_step;
  if (fabs(fabs(d_value) - d_step) < 1e-14 && d_value * steps < 0)//possible zero
    val = 0.0;

  if (setValue(val))
    emit valueChanged( d_value);
}

QAbstractSpinBox::StepEnabled DoubleSpinBox::stepEnabled() const
{
  QAbstractSpinBox::StepEnabled stepDown = QAbstractSpinBox::StepNone;
  if (d_value > d_min_val)
    stepDown = StepDownEnabled;

  QAbstractSpinBox::StepEnabled stepUp = QAbstractSpinBox::StepNone;
  if (d_value < d_max_val)
    stepUp = StepUpEnabled;

  return stepDown | stepUp;
}

double DoubleSpinBox::value()
{
  const bool notify(false);
  interpretText(notify);
  return d_value;
}

bool DoubleSpinBox::setValue(double val)
{
  if (val >= d_min_val && val <= d_max_val)
  {
    d_value = val;
    lineEdit()->setText(textFromValue(d_value));
    return true;
  }

  lineEdit()->setText(textFromValue(d_value));
  return false;
}

QString DoubleSpinBox::textFromValue(double value) const
{
  if (!specialValueText().isEmpty() && value == d_min_val)
    return specialValueText();

  if (d_prec <= 14)
    return locale().toString(value, d_format, d_prec);

  return locale().toString(value, d_format, 6);
}

QValidator::State DoubleSpinBox::validate(QString &, int &) const
{
  return QValidator::Acceptable;
}

void DoubleSpinBox::focusInEvent(QFocusEvent * e)
{
  emit activated(this);
  return QAbstractSpinBox::focusInEvent(e);
}

/*****************************************************************************
 *
 * Class RangeLimitBox
 *
 *****************************************************************************/

RangeLimitBox::RangeLimitBox(LimitType type, QWidget * parent) :
      QWidget(parent), d_type(type)
{
  d_checkbox = new QCheckBox();
  d_spin_box = new DoubleSpinBox();
  d_spin_box->setSpecialValueText(" ");
  d_spin_box->setValue(-DBL_MAX);
  d_spin_box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  d_spin_box->setEnabled(false);

  QHBoxLayout *l = new QHBoxLayout(this);
  l->setMargin(0);
  l->setSpacing(0);
  l->addWidget(d_checkbox);
  l->addWidget(d_spin_box);

  setFocusPolicy(Qt::StrongFocus);
  setFocusProxy( d_spin_box);
  connect(d_checkbox, SIGNAL(toggled(bool)), d_spin_box, SLOT(setEnabled(bool)));
}

double RangeLimitBox::value()
{
  if (d_checkbox->isChecked())
    return d_spin_box->value();

  double val = -DBL_MAX;
  if (d_type == RightLimit)
    return DBL_MAX;
  return val;
}
