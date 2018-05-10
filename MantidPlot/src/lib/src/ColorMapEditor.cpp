/***************************************************************************
        File                 : ColorMapEditor.cpp
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
#include "ColorMapEditor.h"
#include "MantidQtWidgets/Common/DoubleSpinBox.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLayout>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QTableWidget>

ColorMapEditor::ColorMapEditor(const QLocale &locale, int precision,
                               QWidget *parent)
    : QWidget(parent), color_map(QwtLinearColorMap()), min_val(0), max_val(1),
      d_locale(locale), d_precision(precision) {
  table = new QTableWidget();
  table->setColumnCount(2);
  table->setSelectionMode(QAbstractItemView::SingleSelection);
  table->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  table->verticalHeader()->hide();
  table->horizontalHeader()->setClickable(false);
  table->horizontalHeader()->setResizeMode(0, QHeaderView::Interactive);
  table->horizontalHeader()->setResizeMode(1, QHeaderView::ResizeToContents);
  table->horizontalHeader()->setStretchLastSection(true);
  table->viewport()->setMouseTracking(true);
  table->viewport()->installEventFilter(this);
  table->setHorizontalHeaderLabels({tr("Level"), tr("Color")});
  table->setMinimumHeight(6 * table->horizontalHeader()->height() + 2);
  table->installEventFilter(this);

  connect(table, SIGNAL(cellClicked(int, int)), this,
          SLOT(showColorDialog(int, int)));

  insertBtn = new QPushButton(tr("&Insert"));
  insertBtn->setEnabled(false);
  connect(insertBtn, SIGNAL(clicked()), this, SLOT(insertLevel()));

  deleteBtn = new QPushButton(tr("&Delete"));
  deleteBtn->setEnabled(false);
  connect(deleteBtn, SIGNAL(clicked()), this, SLOT(deleteLevel()));

  QHBoxLayout *hb = new QHBoxLayout();
  hb->addWidget(insertBtn);
  hb->addWidget(deleteBtn);

  scaleColorsBox = new QCheckBox(tr("&Scale Colors"));
  scaleColorsBox->setChecked(true);
  connect(scaleColorsBox, SIGNAL(toggled(bool)), this,
          SLOT(setScaledColors(bool)));

  QVBoxLayout *vl = new QVBoxLayout(this);
  vl->setSpacing(0);
  vl->addWidget(table);
  vl->addLayout(hb);
  vl->addWidget(scaleColorsBox);

  setFocusProxy(table);
  setMaximumWidth(200);
}

void ColorMapEditor::updateColorMap() {
  int rows = table->rowCount();
  QColor c_min = QColor(table->item(0, 1)->text());
  QColor c_max = QColor(table->item(rows - 1, 1)->text());
  QwtLinearColorMap map(c_min, c_max);
  for (int i = 1; i < rows - 1; i++) {
    QwtDoubleInterval range = QwtDoubleInterval(min_val, max_val);
    double val =
        (((DoubleSpinBox *)table->cellWidget(i, 0))->value() - min_val) /
        range.width();
    map.addColorStop(val, QColor(table->item(i, 1)->text()));
  }

  color_map = map;
  setScaledColors(scaleColorsBox->isChecked());
}

void ColorMapEditor::setColorMap(const QwtLinearColorMap &map) {
  scaleColorsBox->setChecked(map.mode() == QwtLinearColorMap::ScaledColors);

  QwtArray<double> colors = map.colorStops();
  int rows = (int)colors.size();
  table->setRowCount(rows);
  table->blockSignals(true);

  QwtDoubleInterval range = QwtDoubleInterval(min_val, max_val);
  for (int i = 0; i < rows; i++) {
    DoubleSpinBox *sb = new DoubleSpinBox();
    sb->setLocale(d_locale);
    sb->setDecimals(d_precision);
    sb->setValue(min_val + colors[i] * range.width());

    if (i == 0)
      sb->setRange(min_val, min_val);
    else if (i == rows - 1)
      sb->setRange(max_val, max_val);
    else
      sb->setRange(min_val, max_val);

    connect(sb, SIGNAL(valueChanged(double)), this, SLOT(updateColorMap()));
    connect(sb, SIGNAL(activated(DoubleSpinBox *)), this,
            SLOT(spinBoxActivated(DoubleSpinBox *)));
    table->setCellWidget(i, 0, sb);

    QColor c = QColor(map.rgb(QwtDoubleInterval(0, 1), colors[i]));
    QTableWidgetItem *it = new QTableWidgetItem(c.name());
    // Avoid compiler warning
    //#ifdef Q_CC_MSVC
    it->setFlags(it->flags() & (~Qt::ItemIsEditable));
    //#else
    //    it->setFlags(!Qt::ItemIsEditable);
    //#endif
    it->setBackground(QBrush(c));
    it->setForeground(QBrush(c));
    table->setItem(i, 1, it);
  }
  table->blockSignals(false);

  color_map = map;
}

void ColorMapEditor::setRange(double min, double max) {
  min_val = qMin(min, max);
  max_val = qMax(min, max);
}

void ColorMapEditor::insertLevel() {
  int row = table->currentRow();
  DoubleSpinBox *sb = (DoubleSpinBox *)table->cellWidget(row, 0);
  if (!sb)
    return;

  double current_value = sb->value();
  double previous_value = min_val;
  sb = (DoubleSpinBox *)table->cellWidget(row - 1, 0);
  if (sb)
    previous_value = sb->value();

  double val = 0.5 * (current_value + previous_value);
  QwtDoubleInterval range = QwtDoubleInterval(min_val, max_val);
  double mapped_val = (val - min_val) / range.width();

  QColor c = QColor(color_map.rgb(QwtDoubleInterval(0, 1), mapped_val));

  table->blockSignals(true);
  table->insertRow(row);

  sb = new DoubleSpinBox();
  sb->setLocale(d_locale);
  sb->setDecimals(d_precision);
  sb->setValue(val);
  sb->setRange(min_val, max_val);
  connect(sb, SIGNAL(valueChanged(double)), this, SLOT(updateColorMap()));
  connect(sb, SIGNAL(activated(DoubleSpinBox *)), this,
          SLOT(spinBoxActivated(DoubleSpinBox *)));
  table->setCellWidget(row, 0, sb);

  QTableWidgetItem *it = new QTableWidgetItem(c.name());
  // Avoid compiler warning
  //#ifdef Q_CC_MSVC
  it->setFlags(it->flags() & (~Qt::ItemIsEditable));
  //#else
  //  it->setFlags(!Qt::ItemIsEditable);
  //#endif
  it->setBackground(QBrush(c));
  it->setForeground(QBrush(c));
  table->setItem(row, 1, it);
  table->blockSignals(false);

  enableButtons(table->currentRow());
  updateColorMap();
}

void ColorMapEditor::deleteLevel() {
  table->removeRow(table->currentRow());
  enableButtons(table->currentRow());
  updateColorMap();
}

void ColorMapEditor::showColorDialog(int row, int col) {
  if (col != 1)
    return;

  enableButtons(row);

  QColor c = QColor(table->item(row, 1)->text());
  QColor color = QColorDialog::getColor(c, this);
  if (!color.isValid() || color == c)
    return;

  table->item(row, 1)->setText(color.name());
  table->item(row, 1)->setForeground(QBrush(color));
  table->item(row, 1)->setBackground(QBrush(color));

  updateColorMap();
}

bool ColorMapEditor::eventFilter(QObject *object, QEvent *e) {
  if (e->type() == QEvent::MouseMove && object == table->viewport()) {
    const QMouseEvent *me = (const QMouseEvent *)e;
    QPoint pos = table->viewport()->mapToParent(me->pos());
    int row = table->rowAt(pos.y() - table->horizontalHeader()->height());
    if (table->columnAt(pos.x()) == 1 && row >= 0 && row < table->rowCount())
      setCursor(QCursor(Qt::PointingHandCursor));
    else
      setCursor(QCursor(Qt::ArrowCursor));
    return true;
  } else if (e->type() == QEvent::Leave && object == table->viewport()) {
    setCursor(QCursor(Qt::ArrowCursor));
    return true;
  } else if (e->type() == QEvent::KeyPress && object == table) {
    QKeyEvent *ke = (QKeyEvent *)e;
    if (ke->key() == Qt::Key_Return && table->currentColumn() == 1) {
      showColorDialog(table->currentRow(), 1);
      return true;
    }
    return false;
  }
  return QObject::eventFilter(object, e);
}

void ColorMapEditor::enableButtons(int row) {
  if (row < 0)
    return;

  if (row == 0 || row == table->rowCount() - 1)
    deleteBtn->setEnabled(false);
  else
    deleteBtn->setEnabled(true);

  if (!row)
    insertBtn->setEnabled(false);
  else
    insertBtn->setEnabled(true);
}

void ColorMapEditor::setScaledColors(bool scale) {
  if (scale)
    color_map.setMode(QwtLinearColorMap::ScaledColors);
  else
    color_map.setMode(QwtLinearColorMap::FixedColors);

  scalingChanged();
}

QString ColorMapEditor::saveToXmlString(const QwtLinearColorMap &color_map) {
  QString s = "<ColorMap>\n";
  s += "\t<Mode>" + QString::number(color_map.mode()) + "</Mode>\n";
  s += "\t<MinColor>" + color_map.color1().name() + "</MinColor>\n";
  s += "\t<MaxColor>" + color_map.color2().name() + "</MaxColor>\n";
  QwtArray<double> colors = color_map.colorStops();
  int stops = (int)colors.size();
  s += "\t<ColorStops>" + QString::number(stops - 2) + "</ColorStops>\n";
  for (int i = 1; i < stops - 1; i++) {
    s += "\t<Stop>" + QString::number(colors[i]) + "\t";
    s += QColor(color_map.rgb(QwtDoubleInterval(0, 1), colors[i])).name();
    s += "</Stop>\n";
  }
  return s += "</ColorMap>\n";
}

void ColorMapEditor::spinBoxActivated(DoubleSpinBox *sb) {
  if (!sb)
    return;

  int rows = table->rowCount();
  for (int i = 0; i < rows; i++) {
    DoubleSpinBox *box = (DoubleSpinBox *)table->cellWidget(i, 0);
    if (box && box == sb) {
      table->setCurrentCell(i, 0);
      enableButtons(i);
      return;
    }
  }
}
