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
#include <QLineEdit>
#include <QHBoxLayout>
#include <QCloseEvent>
#include <float.h>

DoubleSpinBox::DoubleSpinBox(const char format, QWidget * parent)
:QAbstractSpinBox(parent),
d_format(format),
d_min_val(-DBL_MAX),
d_max_val(DBL_MAX),
d_value(0.0),
d_step(0.1),
d_prec(1)
{
    lineEdit()->setText(locale().toString(d_value, d_format, d_prec));
	setKeyboardTracking(false);
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

void DoubleSpinBox::setMinimum(double min)
{
	if (min == d_min_val || min < -DBL_MAX)
		return;

	d_min_val = min;
}

void DoubleSpinBox::setRange(double min, double max)
{
	setMinimum(min);
	setMaximum(max);
}

void DoubleSpinBox::interpretText()
{
	bool ok = false;
	double value = locale().toDouble(text(), &ok);
	if (ok && setValue(value))
        emit valueChanged(d_value);
    else
        lineEdit()->setText(textFromValue(d_value));
}

void DoubleSpinBox::stepBy ( int steps )
{
	if (setValue(d_value + steps * d_step))
        emit valueChanged(d_value);
}

QAbstractSpinBox::StepEnabled DoubleSpinBox::stepEnabled () const
{
	QAbstractSpinBox::StepEnabled stepDown = QAbstractSpinBox::StepNone;
	if (d_value > d_min_val)
		stepDown = StepDownEnabled;

	QAbstractSpinBox::StepEnabled stepUp = QAbstractSpinBox::StepNone;
	if (d_value < d_max_val)
		stepUp = StepUpEnabled;

	return stepDown | stepUp;
}

bool DoubleSpinBox::setValue(double val)
{
	if (val < d_min_val || val > d_max_val){
        lineEdit()->setText(textFromValue(d_value));
        return false;
	}

    d_value = val;
	lineEdit()->setText(textFromValue(d_value));
	return true;
}

QString DoubleSpinBox::textFromValue ( double value) const
{
	if (d_prec < 14)
		return locale().toString(value, d_format, d_prec);

	return locale().toString(value, d_format, 6);
}

QValidator::State DoubleSpinBox::validate(QString & , int & ) const
{
	return QValidator::Acceptable;
}

/*****************************************************************************
 *
 * Class RangeLimitBox
 *
 *****************************************************************************/

RangeLimitBox::RangeLimitBox(LimitType type, QWidget * parent)
:QWidget(parent),
d_type(type)
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
    setFocusProxy(d_spin_box);
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
