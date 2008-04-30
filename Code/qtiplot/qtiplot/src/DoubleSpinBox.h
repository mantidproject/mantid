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

#include <QAbstractSpinBox>
#include <QCheckBox>

class DoubleSpinBox : public QAbstractSpinBox
{
	Q_OBJECT

public:
    DoubleSpinBox(const char format = 'g', QWidget * parent = 0);

	void setSingleStep(double val);
	void setMaximum(double max);
	void setMinimum(double min);
	void setRange(double min, double max);

	int decimals(){return d_prec;};
	void setDecimals(int prec){if (prec >= 0) d_prec = prec;};

	double value(){return d_value;};
	bool setValue(double val);

	QString textFromValue ( double value ) const;
	virtual QValidator::State validate ( QString & input, int & pos ) const;

signals:
	void valueChanged ( double d );

private slots:
	void interpretText();

protected:
	void stepBy ( int steps );
	StepEnabled stepEnabled () const;

private:
    const char d_format;
	double d_min_val;
	double d_max_val;
	double d_value;
	double d_step;
	int d_prec;
};

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
