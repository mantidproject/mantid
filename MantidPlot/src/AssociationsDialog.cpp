/***************************************************************************
    File                 : AssociationsDialog.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Plot associations dialog

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
#include "AssociationsDialog.h"
#include "Table.h"
#include "FunctionCurve.h"
#include "PlotCurve.h"
#include "BoxCurve.h"
#include "QwtErrorPlotCurve.h"
#include "QwtPieCurve.h"
#include "VectorCurve.h"

#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QEvent>
#include <QLayout>
#include <QApplication>
#include <QMessageBox>

AssociationsDialog::AssociationsDialog( Graph* g, Qt::WFlags fl )
    : QDialog( g, fl )
{
  setName( "AssociationsDialog" );
  setWindowTitle( tr( "MantidPlot - Plot Associations" ) );
  setModal(true);
  setSizeGripEnabled(true);
  setFocus();

  QVBoxLayout *vl = new QVBoxLayout();

  QHBoxLayout *hbox1 = new QHBoxLayout ();
  hbox1->addWidget(new QLabel(tr( "Spreadsheet: " )));

  tableCaptionLabel = new QLabel();
  hbox1->addWidget(tableCaptionLabel);
  vl->addLayout(hbox1);

  table = new QTableWidget(3, 5);
  table->horizontalHeader()->setClickable( false );
  table->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
  table->verticalHeader()->hide();
  table->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  table->setMaximumHeight(8*table->rowHeight(0));
  table->setHorizontalHeaderLabels(QStringList() << tr("Column") << tr("X") << tr("Y") << tr("xErr") << tr("yErr"));
  vl->addWidget(table);

  connect(table, SIGNAL(itemClicked(QTableWidgetItem*)), this, SLOT(processStateChange(QTableWidgetItem*)));
  
  associations = new QListWidget();
  associations->setSelectionMode ( QListWidget::SingleSelection );
  vl->addWidget(associations);

  btnApply = new QPushButton(tr( "&Update curves" ));
  btnOK = new QPushButton( tr( "&OK" ) );
  btnOK->setDefault( true );
  btnCancel = new QPushButton( tr( "&Cancel" ) );

  QHBoxLayout *hbox2 = new QHBoxLayout ();
  hbox2->addStretch();
  hbox2->addWidget(btnApply);
  hbox2->addWidget(btnOK);
  hbox2->addWidget(btnCancel);
  vl->addStretch();
  vl->addLayout(hbox2);
  setLayout(vl);

  active_table = 0;

  connect(associations, SIGNAL(currentRowChanged(int)), this, SLOT(updateTable(int)));
  connect(btnOK, SIGNAL(clicked()),this, SLOT(accept()));
  connect(btnCancel, SIGNAL(clicked()),this, SLOT(close()));
  connect(btnApply, SIGNAL(clicked()),this, SLOT(updateCurves()));

  setGraph(g);

}

void AssociationsDialog::accept()
{
updateCurves();
close();
}

void AssociationsDialog::updateCurves()
{
  if (!graph)
    return;

  QApplication::setOverrideCursor(Qt::waitCursor);

  for (int i = 0; i < associations->count(); i++)
    changePlotAssociation(i, plotAssociation(associations->item(i)->text()));
  graph->updatePlot();

  QApplication::restoreOverrideCursor();
}

void AssociationsDialog::changePlotAssociation(int curve, const QString& text)
{
  DataCurve *c = dynamic_cast<DataCurve *>(graph->curve(curve)); //c_keys[curve]);
  if (!c)
        return;

    if (c->plotAssociation() == text)
        return;

  QStringList lst = text.split(",", QString::SkipEmptyParts);
  if (lst.count() == 1){
    c->setTitle(lst[0]);
    if (auto b = dynamic_cast<BoxCurve*>(c))
      b->loadData();
    else if (auto p = dynamic_cast<QwtPieCurve*>(c))
      p->loadData();
  } else if (lst.count() == 2){
    c->setXColumnName(lst[0].remove("(X)"));
    c->setTitle(lst[1].remove("(Y)"));
    c->loadData();
  } else if (lst.count() == 3){//curve with error bars
    if (QwtErrorPlotCurve *er = dynamic_cast<QwtErrorPlotCurve *>(c)) {
      QString xColName = lst[0].remove("(X)");
      QString yColName = lst[1].remove("(Y)");
      QString erColName = lst[2].remove("(xErr)").remove("(yErr)");
      DataCurve *master_curve = graph->masterCurve(xColName, yColName);
      if (!master_curve)
        return;

      int type = QwtErrorPlotCurve::Vertical;
      if (text.contains("(xErr)"))
        type = QwtErrorPlotCurve::Horizontal;
      er->setDirection(type);
      er->setTitle(erColName);
      if (master_curve != er->masterCurve())
        er->setMasterCurve(master_curve);
      else
        er->loadData();
    }
  } else if (lst.count() == 4) {
    if (VectorCurve *v = dynamic_cast<VectorCurve *>(c)) {
      v->setXColumnName(lst[0].remove("(X)"));
      v->setTitle(lst[1].remove("(Y)"));

      QString xEndCol = lst[2].remove("(X)").remove("(A)");
      QString yEndCol = lst[3].remove("(Y)").remove("(M)");
      if (v->vectorEndXAColName() != xEndCol || v->vectorEndYMColName() != yEndCol)
        v->setVectorEnd(xEndCol, yEndCol);
      else
        v->loadData();
    }
  }
  graph->notifyChanges();
}

QString AssociationsDialog::plotAssociation(const QString& text)
{
QString s = text;
QStringList lst = s.split(": ", QString::SkipEmptyParts);
QStringList cols = lst[1].split(",", QString::SkipEmptyParts);

QString tableName = lst[0];
s = tableName + "_" + cols[0];
for (int i=1; i < (int)cols.count(); i++ )
  s+="," + tableName + "_" + cols[i];
return s;
}

void AssociationsDialog::initTablesList(QList<MdiSubWindow *> lst, int curve)
{
tables = lst;
active_table = 0;

if (curve < 0 || curve >= static_cast<int>(associations->count()))
  curve = 0;

associations->setCurrentRow (curve);
}

Table * AssociationsDialog::findTable(int index)
{
  QString text = associations->item(index)->text();
  QStringList lst= text.split(":", QString::SkipEmptyParts);
  foreach(MdiSubWindow *w, tables){
    if (w->objectName() == lst[0])
      return dynamic_cast<Table *>(w);
  }
  return 0;
}

void AssociationsDialog::updateTable(int index)
{
  Table *t = findTable(index);
  if (!t)
    return;

  if (active_table != t)
  {
    active_table = t;
    tableCaptionLabel->setText(t->objectName());
    table->clearContents();
    table->setRowCount(t->numCols());

    QStringList colNames = t->colNames();
    // this vector will tell which rows should be disabled (cause there's no data in them)
    std::vector<bool> disableRow;
    disableRow.resize(table->rowCount(), false);
    for (int i=0; i<table->rowCount(); i++ )
    {
      QTableWidgetItem *cell = new QTableWidgetItem(colNames[i]);
      cell->setBackground (QBrush(Qt::lightGray));
      cell->setFlags (Qt::ItemIsEnabled);
      table->setItem(i, 0, cell);

      // do we need to disable this row cause the corresponding curve it's empty?
      // (empty curves could cause crashes in many other places)
      bool allEmpty = true;
      // Note possible confusion, here 'table' is the table that you see in the AssociationsDialog,
      // whereas t is the underlying data table (spreadsheet).
      for (int dataRow = 0; dataRow < t->numRows() && allEmpty; dataRow++)
      {
        // use i (row in the associations table) as column index
        allEmpty = allEmpty & t->text(dataRow, i).isEmpty();
      }
      if (allEmpty)
        disableRow[i] = true;
    }

    for (int j=1; j < table->columnCount(); j++)
    {
      for (int i=0; i < table->rowCount(); i++ )
      {
          QTableWidgetItem *cell = new QTableWidgetItem();
          cell->setBackground(QBrush(Qt::lightGray));
          cell->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
          table->setItem(i, j, cell);

          // disable (but keep the checkbox, as set above)
          if (disableRow[i])
            cell->setFlags(Qt::NoItemFlags);
      }
    }
  }
  updateColumnTypes();
}

void AssociationsDialog::updateColumnTypes()
{
QString text = associations->currentItem()->text();
QStringList lst = text.split(": ", QString::SkipEmptyParts);
QStringList cols = lst[1].split(",", QString::SkipEmptyParts);

QString xColName, yColName;

int n = (int)cols.count();
if (n == 2){
  xColName = cols[0].remove("(X)");
  yColName = cols[1].remove("(Y)");

    table->showColumn(1);
  table->hideColumn(3);
  table->hideColumn(4);
  }
else if (n == 1){//box plots
  yColName = cols[0];
  table->hideColumn(1);
  table->hideColumn(3);
  table->hideColumn(4);
  }

QTableWidgetItem* it;
for (int i=0; i < table->rowCount(); i++ )
{
  it = table->item(i, 1);

  if (table->item(i, 0)->text() == xColName)
    it->setCheckState(Qt::Checked);
  else
    it->setCheckState(Qt::Unchecked);

  it = table->item(i, 2);

  if (table->item(i,0)->text() == yColName)
    it->setCheckState(Qt::Checked);
  else
    it->setCheckState(Qt::Unchecked);
}

bool xerr = false, yerr = false, vectors = false;
QString errColName, xEndColName, yEndColName;
if (n > 2){
  table->showColumn(3);
  table->showColumn(4);

  if (cols[2].contains("(xErr)") || cols[2].contains("(yErr)")){//if error bars
    table->horizontalHeaderItem(3)->setText(tr("xErr"));
    table->horizontalHeaderItem(4)->setText(tr("yErr"));
    }

  if (cols[2].contains("(xErr)")){
    xerr = true;
    errColName = cols[2].remove("(xErr)");
    } else if (cols[2].contains("(yErr)")){
    yerr = true;
    errColName = cols[2].remove("(yErr)");
    } else if (cols.count() > 3 && cols[2].contains("(X)") && cols[3].contains("(Y)")){
    vectors = true;
    xEndColName = cols[2].remove("(X)");
    yEndColName = cols[3].remove("(Y)");
    table->horizontalHeaderItem(3)->setText(tr("xEnd"));
    table->horizontalHeaderItem(4)->setText(tr("yEnd"));
    } else if (cols.count() > 3 && cols[2].contains("(A)") && cols[3].contains("(M)")){
    vectors = true;
    xEndColName = cols[2].remove("(A)");
    yEndColName = cols[3].remove("(M)");
    table->horizontalHeaderItem(3)->setText(tr("Angle"));
    table->horizontalHeaderItem(4)->setText(tr("Magn.","Magnitude, vector length"));
    }
}


for (int i=0; i < table->rowCount(); i++){
  it = table->item(i, 3);
  if (xerr || vectors){
    if (table->item(i,0)->text() == errColName || table->item(i,0)->text() == xEndColName)
      it->setCheckState(Qt::Checked);
    else
      it->setCheckState(Qt::Unchecked);
    } else
    it->setCheckState(Qt::Unchecked);

  it = table->item(i, 4);
  if (yerr || vectors){
    if (table->item(i,0)->text() == errColName || table->item(i,0)->text() == yEndColName)
      it->setCheckState(Qt::Checked);
    else
      it->setCheckState(Qt::Unchecked);
    } else
    it->setCheckState(Qt::Unchecked);
  }
}

void AssociationsDialog::uncheckCol(int col)
{
  // Can't uncheck header column
  if(col == 0)
    return;

  for (int i = 0; i < table->rowCount(); i++ )
    table->item(i, col)->setCheckState(Qt::Unchecked);
}

void AssociationsDialog::setGraph(Graph *g)
{
    graph = g;

    for (int i=0; i<graph->curves(); i++){
        const QwtPlotItem *it = (QwtPlotItem *)graph->plotItem(i);
        if (!it)
            continue;
        if (it->rtti() != QwtPlotItem::Rtti_PlotCurve)
            continue;

        auto dataCurve = dynamic_cast<const DataCurve *>(it);
        if (dataCurve && dataCurve->type() != Graph::Function){
            QString s = dataCurve->plotAssociation();
            if (auto table = dataCurve->table()){
                QString tableName = table->objectName();
                plotAssociationsList << tableName + ": " + s.remove(tableName + "_");
            }
        }
  }
    associations->addItems(plotAssociationsList);
    associations->setMaximumHeight((plotAssociationsList.count()+1)*associations->visualItemRect(associations->item(0)).height());
}

void AssociationsDialog::updatePlotAssociation(int row, int col)
{
int index = associations->currentRow();
QString text = associations->currentItem()->text();
QStringList lst = text.split(": ", QString::SkipEmptyParts);
QStringList cols = lst[1].split(",", QString::SkipEmptyParts);

if (col == 1){
  cols[0] = table->item(row, 0)->text() + "(X)";
  text = lst[0] + ": " + cols.join(",");
  }
else if (col == 2){
  if (cols.count() >= 2){
    cols[1] = table->item(row, 0)->text() + "(Y)";
    text = lst[0] + ": " + cols.join(",");
  } else // box or pie plots
    text = lst[0] + ": " + table->item(row, 0)->text();
  }
else if (col == 3){
  if (text.contains("(A)")){//vect XYAM curve
    cols[2] = table->item(row, 0)->text() + "(A)";
    text = lst[0] + ": " + cols.join(",");
    }
  else if (!text.contains("(A)") && text.count("(X)") == 1){
    cols[2] = table->item(row, 0)->text() + "(xErr)";
    text = lst[0] + ": " + cols.join(",");
    uncheckCol(4);
    }
  else if (text.count("(X)") == 2){//vect XYXY curve
    cols[2] = table->item(row, 0)->text() + "(X)";
    text = lst[0] + ": " + cols.join(",");
    }
  }
else if (col == 4){
  if (text.contains("(M)")){//vect XYAM curve
    cols[3] = table->item(row, 0)->text() + "(M)";
    text = lst[0] + ": " + cols.join(",");
    }
  else if (!text.contains("(M)") && text.count("(X)") == 1){
    cols[2] = table->item(row, 0)->text() + "(yErr)";
    text = lst[0] + ": " + cols.join(",");
    uncheckCol(3);
    }
  else if (text.count("(Y)") == 2){//vect XYXY curve
    cols[3] = table->item(row, 0)->text() + "(Y)";
    text = lst[0] + ": " + cols.join(",");
    }
  }

//change associations for error bars depending on the curve "index"
QString old_as = plotAssociationsList[index];
for (int i=0; i<(int)plotAssociationsList.count(); i++){
  QString as = plotAssociationsList[i];
  if (as.contains(old_as) && (as.contains("(xErr)") || as.contains("(yErr)"))){
    QStringList ls = as.split(",", QString::SkipEmptyParts);
    as = text + "," + ls[2];
    plotAssociationsList[i] = as;
    }
  }

plotAssociationsList [index] = text;
associations->item(index)->setText(text);
}

void AssociationsDialog::processStateChange(QTableWidgetItem* item)
{
  // Ignore the header column
  if(item->column() == 0)
    return;

  // Ignore uncheck events
  if(item->checkState() == Qt::Unchecked)
    return;

  // Make sure that's the only checked item in the columns
  uncheckCol(item->column());
  item->setCheckState(Qt::Checked);

  // Update association
  updatePlotAssociation(item->row(), item->column());
}
