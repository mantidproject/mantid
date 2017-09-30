/***************************************************************************
    File                 : LineDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Line options dialog

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
#ifndef LINEDIALOG_H
#define LINEDIALOG_H

#include <qdialog.h>

class QCheckBox;
class QComboBox;
class QPushButton;
class QTabWidget;
class QWidget;
class QSpinBox;
class DoubleSpinBox;
class ColorButton;
class ArrowMarker;
class DoubleSpinBox;

//! Line options dialog
class LineDialog : public QDialog {
  Q_OBJECT

public:
  LineDialog(ArrowMarker *line, QWidget *parent = nullptr,
             Qt::WFlags fl = nullptr);

  enum Unit { ScaleCoordinates, Pixels };

  void initGeometryTab();
  void enableHeadTab();
  void setCoordinates(int unit);

public slots:
  void enableButtonDefault(QWidget *w);
  void setDefaultValues();
  void displayCoordinates(int unit);
  void setLineStyle(Qt::PenStyle style);
  void accept() override;
  void apply();

private:
  ArrowMarker *lm;

  ColorButton *colorBox;
  QComboBox *styleBox;
  DoubleSpinBox *widthBox;
  QComboBox *unitBox;
  QPushButton *btnOk;
  QPushButton *btnApply;
  QPushButton *buttonDefault;
  QCheckBox *endBox;
  QCheckBox *startBox, *filledBox;
  QTabWidget *tw;
  QWidget *options, *geometry, *head;
  DoubleSpinBox *xStartBox, *yStartBox, *xEndBox, *yEndBox;
  QSpinBox *xStartPixelBox, *yStartPixelBox, *xEndPixelBox, *yEndPixelBox;
  QSpinBox *boxHeadAngle, *boxHeadLength;
};

#endif // LINEDIALOG_H
