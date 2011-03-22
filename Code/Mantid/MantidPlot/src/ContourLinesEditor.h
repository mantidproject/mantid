/***************************************************************************
	File                 : ContourLinesEditor.h
	Project              : QtiPlot
--------------------------------------------------------------------
	Copyright            : (C) 2009 by Ion Vasilief
	Email (use @ for *)  : ion_vasilief*yahoo.fr
	Description          : A Contour Lines Editor Widget
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
#ifndef CONTOURLINESEDITOR_H
#define CONTOURLINESEDITOR_H

#include <QWidget>
#include <QLocale>

class QPushButton;
class QTableWidget;
class QCheckBox;
class DoubleSpinBox;
class Spectrogram;
class ColorButton;
class PenStyleBox;

//! A complex widget allowing to customize contour line plots.
/**
 * It uses a QTableWidget to display the values in one column and their corresponding pen in a second column.
 * A click on a table color pops-up a dialog allowing to customize the pen.

	\image html images/color_map_editor.png
 */
class ContourLinesEditor: public QWidget
{
    Q_OBJECT

public:
	//! Constructor.
	/**
	* @param parent :: parent widget (only affects placement of the widget)
	*/
	ContourLinesEditor(const QLocale& locale = QLocale::system(), int precision = 6, QWidget* parent = 0);
	~ContourLinesEditor();
	//! Use this function to initialize the values.
	void setSpectrogram(Spectrogram *sp);
	//! Updates levels in the target spectrogram and replots it.
	void updateContourLevels();
	void updateContourPens();
	//! Updates cell items when spectrogram contour lines changed.
	void updateContents();
	void showPenColumn(bool on = true);

protected slots:
	void enableButtons(int row);
	void showPenDialog(int row, int col);
	void insertLevel();
	void deleteLevel();
	void spinBoxActivated(DoubleSpinBox *);
	void updatePen();

	bool eventFilter(QObject *object, QEvent *e);

private:
	void updatePenColumn();

	//! Table displaying the values ranges in the first column and their corresponding pens in the second column
	QTableWidget *table;
	QPushButton *insertBtn, *deleteBtn;
	Spectrogram *d_spectrogram;

	//! Locale settings used to display level values
	QLocale d_locale;
	//! Precision used to display level values
	int d_precision;

	QDialog *penDialog;
	ColorButton *penColorBox;
	PenStyleBox *penStyleBox;
	DoubleSpinBox *penWidthBox;
	QCheckBox *applyAllColorBox, *applyAllWidthBox, *applyAllStyleBox;

	int d_pen_index;
	QList<QPen> d_pen_list;
};

#endif
