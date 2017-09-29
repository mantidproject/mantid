/***************************************************************************
    File                 : LayerDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2004-2007 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Arrange layers dialog

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
#ifndef LAYERDIALOG_H
#define LAYERDIALOG_H

#include "MultiLayer.h"

class QGroupBox;
class QPushButton;
class QSpinBox;
class QCheckBox;
class QComboBox;

//! Arrange layers dialog
class LayerDialog : public QDialog {
  Q_OBJECT

public:
  LayerDialog(QWidget *parent = nullptr, Qt::WFlags fl = nullptr);
  void setMultiLayer(MultiLayer *g);

protected slots:
  void accept() override;
  void update();
  void enableLayoutOptions(bool ok);
  void swapLayers();

private:
  MultiLayer *multi_layer;

  QPushButton *buttonOk;
  QPushButton *buttonCancel;
  QPushButton *buttonApply;
  QPushButton *buttonSwapLayers;
  QGroupBox *GroupCanvasSize, *GroupGrid;
  QSpinBox *boxX, *boxY, *boxColsGap, *boxRowsGap;
  QSpinBox *boxRightSpace, *boxLeftSpace, *boxTopSpace, *boxBottomSpace;
  QSpinBox *boxCanvasWidth, *boxCanvasHeight, *layersBox;
  QSpinBox *boxLayerDest, *boxLayerSrc;
  QCheckBox *fitBox;
  QComboBox *alignHorBox, *alignVertBox;
};

#endif
