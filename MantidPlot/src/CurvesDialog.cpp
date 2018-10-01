/***************************************************************************
    File                 : CurvesDialog.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu
 Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Add/remove curves dialog

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
#include "CurvesDialog.h"
#include "ApplicationWindow.h"
#include "Folder.h"
#include "FunctionCurve.h"
#include "Graph.h"
#include "Mantid/MantidMatrixCurve.h"
#include "Matrix.h"
#include "PlotCurve.h"
#include "Table.h"
#include <MantidQtWidgets/Common/pixmaps.h>

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QGroupBox>
#include <QKeySequence>
#include <QLabel>
#include <QLayout>
#include <QListWidget>
#include <QMenu>
#include <QPixmap>
#include <QPushButton>
#include <QShortcut>

#include <QMessageBox>

using namespace MantidQt::API;

CurvesDialog::CurvesDialog(ApplicationWindow *app, Graph *g, Qt::WFlags fl)
    : QDialog(g, fl), d_app(app), d_graph(g) {
  if (!app) {
    throw std::logic_error(
        "Null ApplicationWindow pointer is passed to CurvesDialog.");
  }
  setObjectName("CurvesDialog");
  setWindowTitle(tr("MantidPlot - Add/Remove curves"));
  setSizeGripEnabled(true);
  setFocus();

  QHBoxLayout *hl = new QHBoxLayout();

  hl->addWidget(new QLabel(tr("New curves style")));
  boxStyle = new QComboBox();
  boxStyle->addItem(getQPixmap("lPlot_xpm"), tr(" Line"));
  boxStyle->addItem(getQPixmap("pPlot_xpm"), tr(" Scatter"));
  boxStyle->addItem(getQPixmap("lpPlot_xpm"), tr(" Line + Symbol"));
  boxStyle->addItem(getQPixmap("dropLines_xpm"), tr(" Vertical drop lines"));
  boxStyle->addItem(getQPixmap("spline_xpm"), tr(" Spline"));
  boxStyle->addItem(getQPixmap("hor_steps_xpm"), tr(" Horizontal steps"));
  boxStyle->addItem(getQPixmap("vert_steps_xpm"), tr(" Vertical steps"));
  boxStyle->addItem(getQPixmap("area_xpm"), tr(" Area"));
  boxStyle->addItem(getQPixmap("vertBars_xpm"), tr(" Vertical Bars"));
  boxStyle->addItem(getQPixmap("hBars_xpm"), tr(" Horizontal Bars"));
  hl->addWidget(boxStyle);

  boxMatrixStyle = new QComboBox();
  boxMatrixStyle->addItem(getQPixmap("color_map_xpm"),
                          tr("Contour - Color Fill"));
  boxMatrixStyle->addItem(getQPixmap("contour_map_xpm"), tr("Contour Lines"));
  boxMatrixStyle->addItem(getQPixmap("gray_map_xpm"), tr("Gray Scale Map"));
  boxMatrixStyle->addItem(getQPixmap("histogram_xpm"), tr("Histogram"));
  boxMatrixStyle->hide();
  hl->addWidget(boxMatrixStyle);
  hl->addStretch();

  QGridLayout *gl = new QGridLayout();
  gl->addWidget(new QLabel(tr("Available data")), 0, 0);
  gl->addWidget(new QLabel(tr("Graph contents")), 0, 2);

  available = new QListWidget();
  available->setSelectionMode(QAbstractItemView::ExtendedSelection);
  gl->addWidget(available, 1, 0);

  // add button (move to graph contents)
  QVBoxLayout *vl1 = new QVBoxLayout();
  btnAdd = new QPushButton();
  btnAdd->setIcon(getQPixmap("next_xpm"));
  btnAdd->setFixedWidth(35);
  btnAdd->setFixedHeight(30);
  vl1->addWidget(btnAdd);

  // remove button (move to available data)
  btnRemove = new QPushButton();
  btnRemove->setIcon(getQPixmap("prev_xpm"));
  btnRemove->setFixedWidth(35);
  btnRemove->setFixedHeight(30);
  vl1->addWidget(btnRemove);
  vl1->addStretch();

  gl->addLayout(vl1, 1, 1);
  contents = new QListWidget();
  contents->setSelectionMode(QAbstractItemView::ExtendedSelection);
  gl->addWidget(contents, 1, 2);

  QVBoxLayout *vl2 = new QVBoxLayout();
  btnAssociations = new QPushButton(tr("&Plot Associations..."));
  btnAssociations->setEnabled(false);
  vl2->addWidget(btnAssociations);

  btnRange = new QPushButton(tr("Edit &Range..."));
  btnRange->setEnabled(false);
  vl2->addWidget(btnRange);

  btnEditFunction = new QPushButton(tr("&Edit Function..."));
  btnEditFunction->setEnabled(false);
  vl2->addWidget(btnEditFunction);

  btnOK = new QPushButton(tr("OK"));
  vl2->addWidget(btnOK);

  btnCancel = new QPushButton(tr("Close"));
  vl2->addWidget(btnCancel);

  boxShowRange = new QCheckBox(tr("&Show Range"));
  vl2->addWidget(boxShowRange);

  vl2->addStretch();
  gl->addLayout(vl2, 1, 3);

  QVBoxLayout *vl3 = new QVBoxLayout(this);
  vl3->addLayout(hl);
  vl3->addLayout(gl);

  boxShowCurrentFolder = new QCheckBox(tr("Show current &folder only"));
  vl3->addWidget(boxShowCurrentFolder);

  init();

  connect(boxShowCurrentFolder, SIGNAL(toggled(bool)), this,
          SLOT(showCurrentFolder(bool)));
  connect(boxShowRange, SIGNAL(toggled(bool)), this,
          SLOT(showCurveRange(bool)));
  connect(btnRange, SIGNAL(clicked()), this, SLOT(showCurveRangeDialog()));
  connect(btnAssociations, SIGNAL(clicked()), this,
          SLOT(showPlotAssociations()));
  connect(btnEditFunction, SIGNAL(clicked()), this, SLOT(showFunctionDialog()));
  connect(btnAdd, SIGNAL(clicked()), this, SLOT(addCurves()));
  connect(btnRemove, SIGNAL(clicked()), this, SLOT(removeCurves()));
  connect(btnOK, SIGNAL(clicked()), this, SLOT(close()));
  connect(btnCancel, SIGNAL(clicked()), this, SLOT(close()));
  connect(contents, SIGNAL(itemSelectionChanged()), this, SLOT(enableBtnOK()));
  connect(contents, SIGNAL(currentRowChanged(int)), this,
          SLOT(showCurveBtn(int)));
  connect(contents, SIGNAL(itemSelectionChanged()), this,
          SLOT(enableRemoveBtn()));
  connect(available, SIGNAL(itemSelectionChanged()), this,
          SLOT(enableAddBtn()));

  QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Delete), this);
  connect(shortcut, SIGNAL(activated()), this, SLOT(removeCurves()));
  shortcut = new QShortcut(QKeySequence("-"), this);
  connect(shortcut, SIGNAL(activated()), this, SLOT(removeCurves()));
  shortcut = new QShortcut(QKeySequence("+"), this);
  connect(shortcut, SIGNAL(activated()), this, SLOT(addCurves()));

  setGraph(g);
}

CurvesDialog::~CurvesDialog() {
  // Delete our local copies of the curves
  QMap<QString, PlotCurve *>::iterator it;
  for (it = d_plotCurves.begin(); it != d_plotCurves.end(); ++it) {
    delete it.value();
  }
}

void CurvesDialog::showCurveBtn(int) {
  QwtPlotItem *it = d_graph->plotItem(contents->currentRow());
  if (!it)
    return;

  if (it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram ||
      it->rtti() == QwtPlotItem::Rtti_PlotUserItem) {
    btnEditFunction->setEnabled(false);
    btnAssociations->setEnabled(false);
    btnRange->setEnabled(false);
    return;
  }

  PlotCurve *c = dynamic_cast<PlotCurve *>(it);
  if (c && c->type() == GraphOptions::Function) {
    btnEditFunction->setEnabled(true);
    btnAssociations->setEnabled(false);
    btnRange->setEnabled(false);
    return;
  }

  btnAssociations->setEnabled(true);

  btnRange->setEnabled(true);
  if (c && c->type() == GraphOptions::ErrorBars)
    btnRange->setEnabled(false);
}

void CurvesDialog::showCurveRangeDialog() {
  int curve = contents->currentRow();
  if (curve < 0)
    curve = 0;

  d_app->showCurveRangeDialog(d_graph, curve);
  updateCurveRange();
}

void CurvesDialog::showPlotAssociations() {
  int curve = contents->currentRow();
  if (curve < 0)
    curve = 0;

  close();

  d_app->showPlotAssociations(curve);
}

void CurvesDialog::showFunctionDialog() {
  int currentRow = contents->currentRow();
  close();

  d_app->showFunctionDialog(d_graph, currentRow);
}

QSize CurvesDialog::sizeHint() const { return QSize(700, 400); }

void CurvesDialog::contextMenuEvent(QContextMenuEvent *e) {
  QPoint pos = available->viewport()->mapFromGlobal(QCursor::pos());
  QRect rect = available->visualItemRect(available->currentItem());
  if (rect.contains(pos)) {
    QMenu contextMenu(this);
    QList<QListWidgetItem *> lst = available->selectedItems();
    if (lst.size() > 1)
      contextMenu.addAction(tr("&Plot Selection"), this, SLOT(addCurves()));
    else if (lst.size() == 1)
      contextMenu.addAction(tr("&Plot"), this, SLOT(addCurves()));
    contextMenu.exec(QCursor::pos());
  }

  pos = contents->viewport()->mapFromGlobal(QCursor::pos());
  rect = contents->visualItemRect(contents->currentItem());
  if (rect.contains(pos)) {
    QMenu contextMenu(this);
    QList<QListWidgetItem *> lst = contents->selectedItems();

    if (lst.size() > 1)
      contextMenu.addAction(tr("&Delete Selection"), this,
                            SLOT(removeCurves()));
    else if (lst.size() > 0)
      contextMenu.addAction(tr("&Delete Curve"), this, SLOT(removeCurves()));
    contextMenu.exec(QCursor::pos());
  }
  e->accept();
}

void CurvesDialog::init() {
  bool currentFolderOnly = d_app->d_show_current_folder;
  boxShowCurrentFolder->setChecked(currentFolderOnly);
  showCurrentFolder(currentFolderOnly);

  QStringList matrices = d_app->matrixNames();
  if (!matrices.isEmpty()) {
    boxMatrixStyle->show();
    available->addItems(matrices);
  }

  int style = d_app->defaultCurveStyle;
  if (style == GraphOptions::Line)
    boxStyle->setCurrentIndex(0);
  else if (style == GraphOptions::Scatter)
    boxStyle->setCurrentIndex(1);
  else if (style == GraphOptions::LineSymbols)
    boxStyle->setCurrentIndex(2);
  else if (style == GraphOptions::VerticalDropLines)
    boxStyle->setCurrentIndex(3);
  else if (style == GraphOptions::Spline)
    boxStyle->setCurrentIndex(4);
  else if (style == GraphOptions::VerticalSteps)
    boxStyle->setCurrentIndex(5);
  else if (style == GraphOptions::HorizontalSteps)
    boxStyle->setCurrentIndex(6);
  else if (style == GraphOptions::Area)
    boxStyle->setCurrentIndex(7);
  else if (style == GraphOptions::VerticalBars)
    boxStyle->setCurrentIndex(8);
  else if (style == GraphOptions::HorizontalBars)
    boxStyle->setCurrentIndex(9);

  QList<MdiSubWindow *> wList = d_app->windowsList();
  foreach (MdiSubWindow *w, wList) {
    MultiLayer *ml = dynamic_cast<MultiLayer *>(w);
    if (ml) // layers are numbered starting from 1
    {
      for (int i = 1; i <= ml->layers(); i++) {
        Graph *g = ml->layer(i);
        if (g) {
          for (int j = 0; j < g->curves(); j++) {
            MantidMatrixCurve *c =
                dynamic_cast<MantidMatrixCurve *>(g->curve(j));
            if (c) {
              available->addItem(c->title().text());
              // Store copies of the curves
              // Necessary because a curve is deleted when it's removed from a
              // plot
              d_plotCurves[c->title().text()] = c->clone(g);
              ml->setCloseOnEmpty(false);
            }
          }
        }
      }
    }
  }

  if (!available->count())
    btnAdd->setDisabled(true);
}

void CurvesDialog::setGraph(Graph *graph) {
  QList<QListWidgetItem *> lst = available->selectedItems();
  d_graph = graph;
  contents->addItems(d_graph->plotItemsList());
  enableRemoveBtn();
  enableAddBtn();
}

void CurvesDialog::addCurves() {
  QStringList emptyColumns;
  QList<QListWidgetItem *> lst = available->selectedItems();
  for (int i = 0; i < lst.size(); ++i) {
    QString text = lst.at(i)->text();
    if (contents->findItems(text, Qt::MatchExactly).isEmpty()) {
      if (!addCurve(text))
        emptyColumns << text;
    }
  }
  d_graph->updatePlot();
  Graph::showPlotErrorMessage(this, emptyColumns);

  showCurveRange(boxShowRange->isChecked());
}

bool CurvesDialog::addCurve(const QString &name) {
  QStringList matrices = d_app->matrixNames();
  if (matrices.contains(name)) {
    Matrix *m = d_app->matrix(name);
    if (!m)
      return false;

    switch (boxMatrixStyle->currentIndex()) {
    case 0:
      d_graph->plotSpectrogram(m, GraphOptions::ColorMap);
      break;
    case 1:
      d_graph->plotSpectrogram(m, GraphOptions::Contour);
      break;
    case 2:
      d_graph->plotSpectrogram(m, GraphOptions::GrayScale);
      break;
    case 3:
      d_graph->addHistogram(m);
      break;
    }

    contents->addItem(name);
    return true;
  }

  int style = curveStyle();
  Table *t = d_app->table(name);
  if (t) {
    PlotCurve *c = d_graph->insertCurve(t, name, style);
    CurveLayout cl = Graph::initCurveLayout();
    int color, symbol;
    d_graph->guessUniqueCurveLayout(color, symbol);

    cl.lCol = color;
    cl.symCol = color;
    cl.fillCol = color;
    cl.lWidth = float(d_app->defaultCurveLineWidth);
    cl.sSize = d_app->defaultSymbolSize;
    cl.sType = symbol;

    if (style == GraphOptions::Line)
      cl.sType = 0;
    else if (style == GraphOptions::VerticalBars ||
             style == GraphOptions::HorizontalBars) {
      cl.filledArea = 1;
      cl.lCol = 0;
      cl.aCol = color;
      cl.sType = 0;
    } else if (style == GraphOptions::Area) {
      cl.filledArea = 1;
      cl.aCol = color;
      cl.sType = 0;
    } else if (style == GraphOptions::VerticalDropLines)
      cl.connectType = 2;
    else if (style == GraphOptions::VerticalSteps ||
             style == GraphOptions::HorizontalSteps) {
      cl.connectType = 3;
      cl.sType = 0;
    } else if (style == GraphOptions::Spline)
      cl.connectType = 5;

    d_graph->updateCurveLayout(c, &cl);
    contents->addItem(name);
    return true;
  }

  if (d_plotCurves.find(name) != d_plotCurves.end()) {
    d_graph->insertCurve(d_plotCurves[name]->clone(d_graph));
    return true;
  }
  return false;
}

/**Remove curves function
 *
 */
void CurvesDialog::removeCurves() {
  int count = contents->count();
  QList<QListWidgetItem *> lst = contents->selectedItems();

  // disables user from deleting last graph from the graph
  if (count == 1 || count == lst.size()) {
    QMessageBox::warning(this, tr("Cannot Delete"),
                         tr("There should be at least one graph plotted in the "
                            "graph contents "));
    return;
  }

  for (int i = 0; i < lst.size(); ++i) {
    QListWidgetItem *it = lst.at(i);
    QString s = it->text();
    if (boxShowRange->isChecked()) {
      QStringList lst = s.split("[");
      s = lst[0];
    }
    d_graph->removeCurve(s);
  }

  showCurveRange(boxShowRange->isChecked());
  d_graph->updatePlot();
}

/** Enable Disable buttons function
 *
 */
void CurvesDialog::enableAddBtn() {
  btnAdd->setEnabled(available->count() > 0 &&
                     !available->selectedItems().isEmpty());
}

/** Enables or disables the button when appropriate number of graphs are in
 *graph contents
 *
 */
void CurvesDialog::enableRemoveBtn() {
  btnRemove->setEnabled(contents->count() > 1 &&
                        !contents->selectedItems().isEmpty());
}

/** Enables btnOK when there is even one graph plotted in graph contents area
 *
 */
void CurvesDialog::enableBtnOK() {
  btnOK->setEnabled(contents->count() > 0 &&
                    !contents->selectedItems().isEmpty());
}

int CurvesDialog::curveStyle() {
  int style = 0;
  switch (boxStyle->currentIndex()) {
  case 0:
    style = GraphOptions::Line;
    break;
  case 1:
    style = GraphOptions::Scatter;
    break;
  case 2:
    style = GraphOptions::LineSymbols;
    break;
  case 3:
    style = GraphOptions::VerticalDropLines;
    break;
  case 4:
    style = GraphOptions::Spline;
    break;
  case 5:
    style = GraphOptions::VerticalSteps;
    break;
  case 6:
    style = GraphOptions::HorizontalSteps;
    break;
  case 7:
    style = GraphOptions::Area;
    break;
  case 8:
    style = GraphOptions::VerticalBars;
    break;
  case 9:
    style = GraphOptions::HorizontalBars;
    break;
  }
  return style;
}

void CurvesDialog::showCurveRange(bool on) {
  int row = contents->currentRow();
  contents->clear();
  if (on) {
    QStringList lst;
    for (int i = 0; i < d_graph->curves(); i++) {
      QwtPlotItem *it = d_graph->plotItem(i);
      if (!it)
        continue;

      auto plotCurve = dynamic_cast<PlotCurve *>(it);
      if (plotCurve && plotCurve->type() != GraphOptions::Function) {
        if (DataCurve *c = dynamic_cast<DataCurve *>(it)) {
          lst << c->title().text() + "[" + QString::number(c->startRow() + 1) +
                     ":" + QString::number(c->endRow() + 1) + "]";
        }
      } else
        lst << it->title().text();
    }
    contents->addItems(lst);
  } else
    contents->addItems(d_graph->plotItemsList());

  contents->setCurrentRow(row);
  enableRemoveBtn();
}

void CurvesDialog::updateCurveRange() {
  showCurveRange(boxShowRange->isChecked());
}

void CurvesDialog::showCurrentFolder(bool currentFolder) {
  d_app->d_show_current_folder = currentFolder;
  available->clear();
  if (currentFolder) {
    Folder *f = d_app->currentFolder();
    if (f) {
      QStringList columns;
      foreach (QWidget *w, f->windowsList()) {
        if (!w->inherits("Table"))
          continue;

        if (Table *t = dynamic_cast<Table *>(w)) {
          for (int i = 0; i < t->numCols(); i++) {
            if (t->colPlotDesignation(i) == Table::Y)
              columns << QString(t->objectName()) + "_" + t->colLabel(i);
          }
        }
      }
      available->addItems(columns);
    }
  } else
    available->addItems(d_app->columnsList(Table::Y));
}

void CurvesDialog::closeEvent(QCloseEvent *e) {
  d_app->d_add_curves_dialog_size = this->size();
  // Need to reenable close-on-empty behaviour so
  // that deleting workspaces causes the empty graphs to
  // disappear
  QList<MdiSubWindow *> wList = d_app->windowsList();
  foreach (MdiSubWindow *w, wList) {
    MultiLayer *ml = dynamic_cast<MultiLayer *>(w);
    if (ml)
      ml->setCloseOnEmpty(true);
  }
  e->accept();
}
