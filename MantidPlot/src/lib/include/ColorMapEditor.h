/***************************************************************************
        File                 : ColorMapEditor.h
        Project              : QtiPlot
--------------------------------------------------------------------
        Copyright            : (C) 2006 by Ion Vasilief
        Email (use @ for *)  : ion_vasilief*yahoo.fr
        Description          : A QwtLinearColorMap Editor Widget
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
#ifndef COLORMAPEDITOR_H
#define COLORMAPEDITOR_H

#include <QLocale>
#include <QWidget>
#include <qwt_color_map.h>

class QPushButton;
class QTableWidget;
class QCheckBox;
class DoubleSpinBox;

//! A complex widget allowing to customize a QwtLinearColorMap.
/**
 * It uses a QTableWidget to display the values in one column and their
 corresponding colors in a second column.
 * A click on a table color pops-up a QColorDialog allowing to customize it.

        \image html images/color_map_editor.png
 */
class ColorMapEditor : public QWidget {
  Q_OBJECT

public:
  //! Constructor.
  /**
   * \param parent parent widget (only affects placement of the widget)
   */
  ColorMapEditor(const QLocale &locale = QLocale::system(), int precision = 6,
                 QWidget *parent = nullptr);
  //! Returns the customized color map.
  QwtLinearColorMap colorMap() { return color_map; };
  //! Use this function to initialize the color map to be edited.
  void setColorMap(const QwtLinearColorMap &map);
  //! Use this function to initialize the values range.
  void setRange(double min, double max);
  //! Exports the map to a pseudo-XML string
  static QString saveToXmlString(const QwtLinearColorMap &color_map);

signals:
  void scalingChanged();

protected slots:
  void updateColorMap();
  void enableButtons(int row);
  void showColorDialog(int row, int col);
  void insertLevel();
  void deleteLevel();
  void setScaledColors(bool scale = true);
  void spinBoxActivated(DoubleSpinBox *);

  bool eventFilter(QObject *object, QEvent *e) override;

private:
  //! Table displaying the values ranges in the first column and their
  // corresponding colors in the second column
  QTableWidget *table;
  QPushButton *insertBtn, *deleteBtn;
  QCheckBox *scaleColorsBox;

  //! Color map object
  QwtLinearColorMap color_map;
  //! Levels range
  double min_val, max_val;
  //! Locale settings used to display level values
  QLocale d_locale;
  //! Precision used to display level values
  int d_precision;
};

#endif
