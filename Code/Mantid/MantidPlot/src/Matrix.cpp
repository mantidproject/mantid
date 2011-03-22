/***************************************************************************
    File                 : Matrix.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2004-2007 by Ion Vasilief, Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, knut.franke*gmx.de
    Description          : Matrix worksheet class

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
#include "Matrix.h"
#include "MatrixCommand.h"
#include "Graph.h"
#include "ApplicationWindow.h"
#include "muParserScript.h"
#include "ScriptingEnv.h"
#include "pixmaps.h"

#include <QtGlobal>
#include <QTextStream>
#include <QList>
#include <QEvent>
#include <QContextMenuEvent>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QHeaderView>
#include <QApplication>
#include <QVarLengthArray>
#include <QClipboard>
#include <QShortcut>
#include <QPrinter>
#include <QPrintDialog>
#include <QPainter>
#include <QLocale>
#include <QItemDelegate>
#include <QLabel>
#include <QStackedWidget>
#include <QImageWriter>
#include <QSvgGenerator>
#include <QFile>
#include <QUndoStack>

#include <stdlib.h>
#include <stdio.h>

#include <gsl/gsl_math.h>
#include <gsl/gsl_linalg.h>

Matrix::Matrix(ScriptingEnv *env, const QString& label, ApplicationWindow* parent, const QString& name, Qt::WFlags f)
: MdiSubWindow(label, parent, name, f), Scripted(env)
{
  m_bk_color = QColor(255, 255, 128);
  m_matrix_icon = getQPixmap("matrix_xpm");
}

Matrix::Matrix(ScriptingEnv *env, int r, int c, const QString& label, ApplicationWindow* parent, const QString& name, Qt::WFlags f)
: MdiSubWindow(label, parent, name, f), Scripted(env)
{
  m_bk_color = QColor(255, 255, 128);
  m_matrix_icon = getQPixmap("matrix_xpm");
  initTable(r, c);
}

Matrix::Matrix(ScriptingEnv *env, const QImage& image, const QString& label, ApplicationWindow* parent, const QString& name, Qt::WFlags f)
: MdiSubWindow(label, parent, name, f), Scripted(env)
{
  m_bk_color = QColor(255, 255, 128);
  m_matrix_icon = getQPixmap("matrix_xpm");
  initImage(image);
}

void Matrix::initGlobals()
{
  d_workspace = NULL;
  d_table_view = NULL;
  imageLabel = NULL;

  d_header_view_type = ColumnRow;
  d_color_map_type = GrayScale;
  d_color_map = QwtLinearColorMap(Qt::black, Qt::white);
  d_column_width = 100;

  formula_str = "";
  txt_format = 'f';
  num_precision = 6;
  x_start = 1.0;
  x_end = 10.0;
  y_start = 1.0;
  y_end = 10.0;

  d_stack = new QStackedWidget();
  d_stack->setFocusPolicy(Qt::StrongFocus);
  setWidget(d_stack);

  d_undo_stack = new QUndoStack();
  d_undo_stack->setUndoLimit(applicationWindow()->matrixUndoStackSize());
}

void Matrix::initTable(int rows, int cols)
{
  initGlobals();
  d_view_type = TableView;

  d_matrix_model = new MatrixModel(rows, cols, this);
  initTableView();

  // resize the table
  setGeometry(50, 50, QMIN(_Matrix_initial_columns_, cols)*d_table_view->horizontalHeader()->sectionSize(0) + 55,
      (QMIN(_Matrix_initial_rows_,rows)+1)*d_table_view->verticalHeader()->sectionSize(0));
}

void Matrix::initImage(const QImage& image)
{
  initGlobals();
  d_view_type = ImageView;

  d_matrix_model = new MatrixModel(image, this);
  initImageView();

  int w = image.width();
  int h = image.height();
  if (w <= 500 && h <= 400){
    int size = QMAX(w, h);
    imageLabel->resize(size, size);
  } else
    imageLabel->resize(500, 500);

  displayImage(image);
}

double Matrix::cell(int row, int col)
{
  return d_matrix_model->cell(row, col);
}

void Matrix::setCell(int row, int col, double value)
{
  d_matrix_model->setCell(row, col, value);
}

QString Matrix::text(int row, int col)
{
  return d_matrix_model->text(row, col);
}

void Matrix::setText (int row, int col, const QString & new_text )
{
  d_matrix_model->setText(row, col, new_text);
}

void Matrix::setCoordinates(double xs, double xe, double ys, double ye)
{
  if (x_start == xs && x_end == xe &&	y_start == ys && y_end == ye)
    return;

  x_start = xs;
  x_end = xe;
  y_start = ys;
  y_end = ye;

  emit modifiedWindow(this);
}

QString Matrix::saveToString(const QString &info, bool saveAsTemplate)
{
  bool notTemplate = !saveAsTemplate;
  QString s = "<matrix>\n";
  if (notTemplate)
    s += QString(objectName()) + "\t";
  s += QString::number(numRows())+"\t";
  s += QString::number(numCols())+"\t";
  if (notTemplate)
    s += birthDate() + "\n";
  s += info;
  s += "ColWidth\t" + QString::number(d_column_width)+"\n";
  s += "<formula>\n" + formula_str + "\n</formula>\n";
  s += "TextFormat\t" + QString(txt_format) + "\t" + QString::number(num_precision) + "\n";
  if (notTemplate)
    s += "WindowLabel\t" + windowLabel() + "\t" + QString::number(captionPolicy()) + "\n";
  s += "Coordinates\t" + QString::number(x_start,'g',15) + "\t" +QString::number(x_end,'g',15) + "\t";
  s += QString::number(y_start,'g',15) + "\t" + QString::number(y_end,'g',15) + "\n";
  s += "ViewType\t" + QString::number((int)d_view_type) + "\n";
  s += "HeaderViewType\t" + QString::number((int)d_header_view_type) + "\n";

  if (d_color_map_type != Custom)
    s += "ColorPolicy\t" + QString::number(d_color_map_type) + "\n";
  else {
    s += "<ColorMap>\n";
    s += "\t<Mode>" + QString::number(d_color_map.mode()) + "</Mode>\n";
    s += "\t<MinColor>" + d_color_map.color1().name() + "</MinColor>\n";
    s += "\t<MaxColor>" + d_color_map.color2().name() + "</MaxColor>\n";
    QwtArray <double> colors = d_color_map.colorStops();
    int stops = (int)colors.size();
    s += "\t<ColorStops>" + QString::number(stops - 2) + "</ColorStops>\n";
    for (int i = 1; i < stops - 1; i++){
      s += "\t<Stop>" + QString::number(colors[i]) + "\t";
      s += QColor(d_color_map.rgb(QwtDoubleInterval(0,1), colors[i])).name();
      s += "</Stop>\n";
    }
    s += "</ColorMap>\n";
  }

  if (notTemplate)
    s += d_matrix_model->saveToString();
  s +="</matrix>\n";
  return s;
}

QString Matrix::saveAsTemplate(const QString &info)
{
  return saveToString(info, true);
}

void Matrix::restore(const QStringList &lst)
{
  QStringList l;
  QStringList::const_iterator i = lst.begin();

  l = (*i++).split("\t");
  setColumnsWidth(l[1].toInt());

  l = (*i++).split("\t");
  if (l[0] == "Formula")
    formula_str = l[1];
  else if (l[0] == "<formula>"){
    for(formula_str=""; i != lst.end() && *i != "</formula>"; i++)
      formula_str += *i + "\n";
    formula_str.truncate(formula_str.length()-1);
    i++;
  }

  l = (*i++).split("\t");
  if (l[1] == "f")
    setTextFormat('f', l[2].toInt());
  else
    setTextFormat('e', l[2].toInt());

  l = (*i++).split("\t");
  x_start = l[1].toDouble();
  x_end = l[2].toDouble();
  y_start = l[3].toDouble();
  y_end = l[4].toDouble();

  l = (*i++).split("\t");
  d_view_type = (Matrix::ViewType)l[1].toInt();
  l = (*i++).split("\t");
  d_header_view_type = (Matrix::HeaderViewType)l[1].toInt();
  l = (*i++).split("\t");
  d_color_map_type = (Matrix::ColorMapType)l[1].toInt();

  if (lst.contains ("<ColorMap>")){
    QStringList aux;
    while (*i != "</ColorMap>"){
      aux << *i;
      i++;
    }
    setColorMap(aux);
  }

  if (d_view_type == ImageView){
    if (d_table_view)
      delete d_table_view;
    if (d_select_all_shortcut)
      delete d_select_all_shortcut;
    initImageView();
    d_stack->setCurrentWidget(imageLabel);
    if (d_color_map_type == Rainbow)
      setRainbowColorMap();
  }
  resetView();
}

void Matrix::setNumericFormat(const QChar& f, int prec)
{
  if (txt_format == f && num_precision == prec)
    return;

  txt_format = f;
  num_precision = prec;

  resetView();
  emit modifiedWindow(this);
  QApplication::restoreOverrideCursor();
}

void Matrix::setTextFormat(const QChar &format, int precision)
{
  txt_format = format;
  num_precision = precision;
}

void Matrix::setColumnsWidth(int width)
{
  if (d_column_width == width)
    return;

  d_column_width = width;
  d_table_view->horizontalHeader()->setDefaultSectionSize(d_column_width);

  if (d_view_type == TableView){
    int cols = numCols();
    for(int i=0; i<cols; i++)
      d_table_view->setColumnWidth(i, width);
  }

  emit modifiedWindow(this);
}

void Matrix::setDimensions(int rows, int cols)
{
  int r = numRows();
  int c = numCols();
  if (r == rows && c == cols)
    return;

  if (rows <= 0 || cols <= 0 || INT_MAX/rows < cols) //avoid integer overflow
    return;

  if(rows*cols > r*c && !d_matrix_model->canResize(rows, cols))
    return;

  double *buffer = d_matrix_model->dataCopy();
  if (buffer)
    d_undo_stack->push(new MatrixSetSizeCommand(d_matrix_model, QSize(r, c), QSize(rows, cols), buffer,
        tr("Set Dimensions") + " " + QString::number(rows) + "x" + QString::number(cols)));
  else if(ignoreUndo()){
    d_matrix_model->setDimensions(rows, cols);
    resetView();
  }
  emit modifiedWindow(this);
}

double Matrix::integrate()
{
  int rows = numRows() - 1;
  int cols = numCols() - 1;
  double sum = 0.0;
  for(int i=0; i<rows; i++){
    int i1 = i + 1;
    for(int j=0; j<cols; j++){
      int j1 = j + 1;
      sum += 0.25*(d_matrix_model->cell(i, j) + d_matrix_model->cell(i, j1) +
          d_matrix_model->cell(i1, j) + d_matrix_model->cell(i1, j1));
    }
  }
  return sum*dx()*dy();
}

double Matrix::determinant()
{
  int rows = numRows();
  int cols = numCols();

  if (rows != cols){
    QMessageBox::critical((ApplicationWindow *)applicationWindow(), tr("MantidPlot - Error"),
        tr("Calculation failed, the matrix is not square!"));
    return GSL_POSINF;
  }

  gsl_set_error_handler_off();

  gsl_matrix *A = gsl_matrix_alloc(rows, cols);
  gsl_permutation * p = gsl_permutation_alloc(rows);
  if (!A || !p){
    QApplication::restoreOverrideCursor();
    QMessageBox::critical((ApplicationWindow *)applicationWindow(),
        tr("MantidPlot") + " - " + tr("Memory Allocation Error"),
        tr("Not enough memory, operation aborted!"));
    return 0.0;
  }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  double *data = d_matrix_model->dataVector();
  int i, cell = 0;
  for(i=0; i<rows; i++)
    for(int j=0; j<cols; j++)
      gsl_matrix_set(A, i, j, data[cell++]);


  gsl_linalg_LU_decomp(A, p, &i);
  double det = gsl_linalg_LU_det(A, i);

  gsl_matrix_free(A);
  gsl_permutation_free(p);

  QApplication::restoreOverrideCursor();
  return det;
}

void Matrix::invert()
{
  if (numRows() != numCols()){
    QMessageBox::critical((ApplicationWindow *)applicationWindow(), tr("MantidPlot - Error"),
        tr("Inversion failed, the matrix is not square!"));
    return;
  }
  if(d_matrix_model->initWorkspace())
    d_undo_stack->push(new MatrixSymmetryOperation(d_matrix_model, Invert, tr("Invert")));
}

void Matrix::transpose()
{
  initWorkspace(numRows()*numCols());
  if (!d_workspace)
    return;

  d_undo_stack->push(new MatrixSymmetryOperation(d_matrix_model, Transpose, tr("Transpose")));
}

void Matrix::flipVertically()
{
  initWorkspace(numRows()*numCols());
  if (!d_workspace)
    return;

  d_undo_stack->push(new MatrixSymmetryOperation(d_matrix_model, FlipVertically, tr("Flip Vertically")));
}

void Matrix::flipHorizontally()
{
  initWorkspace(numRows()*numCols());
  if (!d_workspace)
    return;

  d_undo_stack->push(new MatrixSymmetryOperation(d_matrix_model, FlipHorizontally, tr("Flip Horizontally")));
}

void Matrix::rotate90(bool clockwise)
{
  initWorkspace(numRows()*numCols());
  if (!d_workspace)
    return;

  if (clockwise)
    d_undo_stack->push(new MatrixSymmetryOperation(d_matrix_model, RotateClockwise, tr("Rotate 90�")));
  else
    d_undo_stack->push(new MatrixSymmetryOperation(d_matrix_model, RotateCounterClockwise, tr("Rotate -90�")));
}

bool Matrix::canCalculate(bool useMuParser)
{
  if (formula_str.isEmpty())
    return false;

  if (useMuParser){
    muParserScript *mup = new muParserScript(scriptingEnv(), formula_str, this, QString("<%1>").arg(objectName()), false);
    connect(mup, SIGNAL(error(const QString&,const QString&,int)), scriptingEnv(), SIGNAL(error(const QString&, const QString&,int)));

    double *ri = mup->defineVariable("i");
    double *rr = mup->defineVariable("row");
    double *cj = mup->defineVariable("j");
    double *cc = mup->defineVariable("col");
    double *x = mup->defineVariable("x");
    double *y = mup->defineVariable("y");

    if (!mup->compile())
      return false;

    double r = 1.0;
    *ri = r; *rr = r; *y = r;
    double c = 1.0; *cj = c; *cc = c; *x = c;
    int codeLines = mup->codeLines();
    if (codeLines == 1 && gsl_isnan(mup->evalSingleLine()))
      return false;
    else if (codeLines > 1){
      QVariant res = mup->eval();
      if (!res.canConvert(QVariant::Double))
        return false;
    }
  } else {
    Script *script = scriptingEnv()->newScript(formula_str, this, QString("<%1>").arg(objectName()), false);
    connect(script, SIGNAL(error(const QString&,const QString&,int)), scriptingEnv(), SIGNAL(error(const QString&,const QString&,int)));
    connect(script, SIGNAL(print(const QString&)), scriptingEnv(), SIGNAL(print(const QString&)));
    if (!script->compile())
      return false;

    double r = 1.0;
    script->setDouble(r, "i");
    script->setDouble(r, "row");
    double c = 1.0;
    script->setDouble(c, "j");
    script->setDouble(c, "col");
    double x = 1.0;
    script->setDouble(x, "x");
    double y = 1.0;
    script->setDouble(y, "y");

    QVariant res = script->eval();
    if (!res.canConvert(QVariant::Double))
      return false;
  }
  return true;
}

bool Matrix::muParserCalculate(int startRow, int endRow, int startCol, int endCol)
{
  double *buffer = d_matrix_model->dataCopy(startRow, endRow, startCol, endCol);
  if (buffer){
    d_undo_stack->push(new MatrixUndoCommand(d_matrix_model, MuParserCalculate, startRow, endRow,
        startCol, endCol, buffer, tr("Calculate Values")));
    emit modifiedWindow(this);
    return true;
  } else if(ignoreUndo()){
    d_matrix_model->muParserCalculate(startRow, endRow, startCol, endCol);
    emit modifiedWindow(this);
    return true;
  }
  return false;
}

bool Matrix::calculate(int startRow, int endRow, int startCol, int endCol, bool forceMuParser)
{
  if (QString(scriptingEnv()->name()) == "muParser" || forceMuParser)
    return muParserCalculate(startRow, endRow, startCol, endCol);

  double *buffer = d_matrix_model->dataCopy(startRow, endRow, startCol, endCol);
  if (buffer){
    d_undo_stack->push(new MatrixUndoCommand(d_matrix_model, Calculate, startRow, endRow,
        startCol, endCol, buffer, tr("Calculate Values")));
    emit modifiedWindow(this);
    return true;
  } else if(ignoreUndo()){
    d_matrix_model->calculate(startRow, endRow, startCol, endCol);
    emit modifiedWindow(this);
    return true;
  }
  return false;
}

void Matrix::clearSelection()
{
  if (d_view_type == ImageView)
    return;

  QItemSelectionModel *selModel = d_table_view->selectionModel();
  if (!selModel || !selModel->hasSelection())
    return;

  const QItemSelectionRange sel = selModel->selection()[0];
  int startRow = sel.top();
  int endRow = sel.bottom();
  int startCol = sel.left();
  int endCol = sel.right();
  double *buffer = d_matrix_model->dataCopy(startRow, endRow, startCol, endCol);
  if (buffer){
    d_undo_stack->push(new MatrixUndoCommand(d_matrix_model, Clear, startRow, endRow, startCol, endCol, buffer, tr("Clear Selection")));
    emit modifiedWindow(this);
  } else if (ignoreUndo()){
    d_matrix_model->clear(startRow, endRow, startCol, endCol);
    emit modifiedWindow(this);
  }
}

void Matrix::copySelection()
{
  if (d_view_type == ImageView)
    return;

  QItemSelectionModel *selModel = d_table_view->selectionModel();
  QString s = "";
  QString eol = applicationWindow()->endOfLine();
  if (!selModel->hasSelection()){
    QModelIndex index = selModel->currentIndex();
    s = text(index.row(), index.column());
  } else {
    QItemSelection sel = selModel->selection();
    QListIterator<QItemSelectionRange> it(sel);
    if(!it.hasNext())
      return;

    QItemSelectionRange cur = it.next();
    int top = cur.top();
    int bottom = cur.bottom();
    int left = cur.left();
    int right = cur.right();
    for(int i=top; i<=bottom; i++){
      for(int j=left; j<right; j++)
        s += d_matrix_model->text(i, j) + "\t";
      s += d_matrix_model->text(i,right) + eol;
    }
  }
  // Copy text into the clipboard
  QApplication::clipboard()->setText(s.trimmed());
}

void Matrix::pasteSelection()
{
  if (d_view_type == ImageView)
    return;

  QString text = QApplication::clipboard()->text();
  if (text.isEmpty())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QStringList linesList = text.split(applicationWindow()->endOfLine(), QString::SkipEmptyParts);
  int rows = linesList.size();
  if (!rows)
    return;

  int cols = linesList[0].split("\t").count();
  for (int i = 1; i < rows; i++){
    int aux = linesList[i].split("\t").count();
    if (aux > cols)
      cols = aux;
  }

  int topRow = 0, leftCol = 0;
  QItemSelectionModel *selModel = d_table_view->selectionModel();
  if (selModel->hasSelection()){
    QItemSelectionRange sel = selModel->selection()[0];
    topRow = sel.top();
    leftCol = sel.left();
  }

  int oldRows = numRows();
  int bottomRow = topRow + rows - 1;
  if (bottomRow > oldRows - 1)
    bottomRow = oldRows - 1;

  int oldCols = numCols();
  int rightCol = leftCol + cols - 1;
  if (rightCol > oldCols - 1)
    rightCol = oldCols - 1;

  double *clipboardBuffer = (double *)malloc(rows*cols*sizeof(double));
  if (!clipboardBuffer){
    QMessageBox::critical(this, tr("MantidPlot") + " - " + tr("Memory Allocation Error"),
        tr("Not enough memory, operation aborted!"));
    QApplication::restoreOverrideCursor();
    return;
  }

  QLocale locale = this->locale(); //Better use QLocale::system() ??
  int cell = 0;
  for(int i = 0; i < rows; i++){
    QStringList cells = linesList[i].split("\t");
    int size = cells.count();
    for(int j = 0; j<cols; j++){
      if (j >= size){
        clipboardBuffer[cell++] = GSL_NAN;
        continue;
      }
      bool numeric = true;
      double value = locale.toDouble(cells[j], &numeric);
      if (numeric)
        clipboardBuffer[cell++] = value;
      else
        clipboardBuffer[cell++] = GSL_NAN;
    }
  }

  QApplication::restoreOverrideCursor();

  double *backupBuffer = d_matrix_model->dataCopy(topRow, bottomRow, leftCol, rightCol);
  if (backupBuffer){
    d_undo_stack->push(new MatrixPasteCommand(d_matrix_model, topRow, bottomRow,
        leftCol, rightCol, clipboardBuffer, rows, cols, backupBuffer, oldRows,
        oldCols, tr("Paste")));
    emit modifiedWindow(this);
  } else if (ignoreUndo()){
    d_matrix_model->pasteData(clipboardBuffer, topRow, leftCol, rows, cols);
    emit modifiedWindow(this);
  }
}

void Matrix::cutSelection()
{
  copySelection();
  clearSelection();
}

void Matrix::deleteSelectedRows()
{
  QItemSelectionModel *selModel = d_table_view->selectionModel();
  if (!selModel || !selModel->hasSelection())
    return;

  int startRow = -1;
  int count = 0;
  int rows = numRows();
  for (int i=0; i<rows; i++){
    if (selModel->isRowSelected (i, QModelIndex())){
      if (startRow < 0)
        startRow = i;
      ++count;
    }
  }
  if (startRow < 0 || !count)
    return;

  double *buffer = d_matrix_model->dataCopy(startRow, startRow + count - 1, 0, numCols() - 1);
  if (buffer){
    d_undo_stack->push(new MatrixDeleteRowsCommand(d_matrix_model, startRow, count, buffer, tr("Delete Rows") + " " +
        QString::number(startRow + 1) + " - " + QString::number(startRow + count)));
    emit modifiedWindow(this);
  } else if (ignoreUndo()){
    d_matrix_model->removeRows(startRow, count);
    d_table_view->reset();
    emit modifiedWindow(this);
  }
}

void Matrix::deleteSelectedColumns()
{
  QItemSelectionModel *selModel = d_table_view->selectionModel();
  if (!selModel || !selModel->hasSelection())
    return;

  int startCol = -1;
  int count = 0;
  int cols = numCols();
  for (int i=0; i<cols; i++){
    if (selModel->isColumnSelected(i, QModelIndex())){
      if (startCol < 0)
        startCol = i;
      ++count;
    }
  }
  if (startCol < 0 || !count)
    return;

  double *buffer = d_matrix_model->dataCopy(0, numRows() - 1, startCol, startCol + count - 1);
  if (buffer){
    d_undo_stack->push(new MatrixDeleteColsCommand(d_matrix_model, startCol, count, buffer, tr("Delete Columns") + " " +
        QString::number(startCol + 1) + " - " + QString::number(startCol + count)));
    emit modifiedWindow(this);
  } else if (ignoreUndo()){
    d_matrix_model->removeColumns(startCol, count);
    d_table_view->reset();
    emit modifiedWindow(this);
  }
}

int Matrix::numSelectedRows()
{
  QItemSelectionModel *selModel = d_table_view->selectionModel();
  if (!selModel || !selModel->hasSelection())
    return 0;

  int rows = numRows();
  int count = 0;
  for (int i = 0; i<rows; i++){
    if (selModel->isRowSelected (i, QModelIndex()))
      count++;
  }
  return count;
}

int Matrix::numSelectedColumns()
{
  QItemSelectionModel *selModel = d_table_view->selectionModel();
  if (!selModel || !selModel->hasSelection())
    return 0;

  int cols = numCols();
  int count = 0;
  for (int i = 0; i<cols; i++){
    if (selModel->isColumnSelected (i, QModelIndex()))
      count++;
  }
  return count;
}

void Matrix::insertRow()
{
  QItemSelectionModel *selModel = d_table_view->selectionModel();
  if (!selModel || !selModel->hasSelection())
    return;

  QModelIndex index = selModel->currentIndex();
  if (!index.isValid())
    return;

  if (!d_matrix_model->canResize(numRows() + 1, numCols()))
    return;

  d_undo_stack->push(new MatrixInsertRowCommand(d_matrix_model, index.row(), tr("Insert Row") + " " +
      QString::number(index.row() + 1)));
  d_table_view->reset();
  emit modifiedWindow(this);
}

void Matrix::insertColumn()
{
  QItemSelectionModel *selModel = d_table_view->selectionModel();
  if (!selModel || !selModel->hasSelection())
    return;

  QModelIndex index = selModel->currentIndex();
  if (!index.isValid())
    return;

  if (!d_matrix_model->canResize(numRows(), numCols() + 1))
    return;

  d_undo_stack->push(new MatrixInsertColCommand(d_matrix_model, index.column(), tr("Insert Column") + " " +
      QString::number(index.column() + 1)));
  d_table_view->reset();
  emit modifiedWindow(this);
}

void Matrix::customEvent(QEvent *e)
{
  if (e->type() == SCRIPTING_CHANGE_EVENT)
    scriptingChangeEvent((ScriptingChangeEvent*)e);
}

void Matrix::exportRasterImage(const QString& fileName, int quality)
{
  d_matrix_model->renderImage().save(fileName, 0, quality);
}

void Matrix::exportToFile(const QString& fileName)
{
  if ( fileName.isEmpty() ){
    QMessageBox::critical(this, tr("MantidPlot - Error"), tr("Please provide a valid file name!"));
    return;
  }

  if (fileName.contains(".eps") || fileName.contains(".pdf") || fileName.contains(".ps")){
    exportVector(fileName);
    return;
  } else if(fileName.contains(".svg")){
    exportSVG(fileName);
    return;
  } else {
    QList<QByteArray> list = QImageWriter::supportedImageFormats();
    for(int i=0 ; i<list.count() ; i++){
      if (fileName.contains( "." + list[i].toLower())){
        d_matrix_model->renderImage().save(fileName, list[i], 100);
        return;
      }
    }
    QMessageBox::critical(this, tr("MantidPlot - Error"), tr("File format not handled, operation aborted!"));
  }
}

void Matrix::exportSVG(const QString& fileName)
{
#if QT_VERSION >= 0x040300
  if (d_view_type != ImageView)
    return;

  int width = numRows();
  int height = numCols();

  QSvgGenerator svg;
  svg.setFileName(fileName);
  svg.setSize(QSize(width, height));

  QPainter p(&svg);
  p.drawImage (QRect(0, 0, width, height), d_matrix_model->renderImage());
  p.end();
#endif
}

void Matrix::exportPDF(const QString& fileName)
{
  print(fileName);
}

void Matrix::print()
{
  print(QString());
}

void Matrix::print(const QString& fileName)
{
  QPrinter printer;
  printer.setColorMode (QPrinter::GrayScale);

  if (!fileName.isEmpty()){
    printer.setCreator("MantidPlot");
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
  } else {
    QPrintDialog printDialog(&printer);
    if (printDialog.exec() != QDialog::Accepted)
      return;
  }
  printer.setFullPage( true );
  QPainter p;
  if ( !p.begin(&printer ) )
    return; // paint on printer
  int dpiy = printer.logicalDpiY();
  const int margin = (int) ( (1/2.54)*dpiy ); // 1 cm margins

  if (d_view_type == ImageView){
    p.drawImage (printer.pageRect(), d_matrix_model->renderImage());
    return;
  }

  QHeaderView *vHeader = d_table_view->verticalHeader();

  int rows = numRows();
  int cols = numCols();
  int height = margin;
  int i, vertHeaderWidth = vHeader->width();
  int right = margin + vertHeaderWidth;

  // print header
  p.setFont(QFont());
  QString header_label = d_matrix_model->headerData(0, Qt::Horizontal).toString();
  QRect br = p.boundingRect(br, Qt::AlignCenter, header_label);
  p.drawLine(right, height, right, height+br.height());
  QRect tr(br);

  for(i=0; i<cols; i++){
    int w = d_table_view->columnWidth(i);
    tr.setTopLeft(QPoint(right,height));
    tr.setWidth(w);
    tr.setHeight(br.height());
    header_label = d_matrix_model->headerData(i, Qt::Horizontal).toString();
    p.drawText(tr, Qt::AlignCenter, header_label,-1);
    right += w;
    p.drawLine(right, height, right, height+tr.height());

    if (right >= printer.width()-2*margin )
      break;
  }

  p.drawLine(margin + vertHeaderWidth, height, right-1, height);//first horizontal line
  height += tr.height();
  p.drawLine(margin, height, right-1, height);

  // print table values
  for(i=0;i<rows;i++){
    right = margin;
    QString cell_text = d_matrix_model->headerData(i, Qt::Horizontal).toString()+"\t";
    tr = p.boundingRect(tr, Qt::AlignCenter, cell_text);
    p.drawLine(right, height, right, height+tr.height());

    br.setTopLeft(QPoint(right,height));
    br.setWidth(vertHeaderWidth);
    br.setHeight(tr.height());
    p.drawText(br,Qt::AlignCenter,cell_text,-1);
    right += vertHeaderWidth;
    p.drawLine(right, height, right, height+tr.height());

    for(int j=0; j<cols; j++){
      int w = d_table_view->columnWidth (j);
      cell_text = text(i,j)+"\t";
      tr = p.boundingRect(tr,Qt::AlignCenter,cell_text);
      br.setTopLeft(QPoint(right,height));
      br.setWidth(w);
      br.setHeight(tr.height());
      p.drawText(br, Qt::AlignCenter, cell_text, -1);
      right += w;
      p.drawLine(right, height, right, height+tr.height());

      if (right >= printer.width()-2*margin )
        break;
    }
    height += br.height();
    p.drawLine(margin, height, right-1, height);

    if (height >= printer.height()-margin ){
      printer.newPage();
      height = margin;
      p.drawLine(margin, height, right, height);
    }
  }
}

void Matrix::exportVector(const QString& fileName, int res, bool color, bool keepAspect, QPrinter::PageSize pageSize)
{
  if (d_view_type != ImageView)
    return;

  if ( fileName.isEmpty() ){
    QMessageBox::critical(this, tr("MantidPlot - Error"), tr("Please provide a valid file name!"));
    return;
  }

  QPrinter printer;
  printer.setCreator("MantidPlot");
  printer.setFullPage(true);
  if (res)
    printer.setResolution(res);

  printer.setOutputFileName(fileName);
  if (fileName.contains(".eps"))
    printer.setOutputFormat(QPrinter::PostScriptFormat);

  if (color)
    printer.setColorMode(QPrinter::Color);
  else
    printer.setColorMode(QPrinter::GrayScale);

  int cols = numCols();
  int rows = numRows();
  QRect rect = QRect(0, 0, cols, rows);
  if (pageSize == QPrinter::Custom)
    printer.setPageSize(Graph::minPageSize(printer, rect));
  else
    printer.setPageSize(pageSize);

  double aspect = (double)cols/(double)rows;
  if (aspect < 1)
    printer.setOrientation(QPrinter::Portrait);
  else
    printer.setOrientation(QPrinter::Landscape);

  if (keepAspect){// export should preserve aspect ratio
    double page_aspect = double(printer.width())/double(printer.height());
    if (page_aspect > aspect){
      int margin = (int) ((0.1/2.54)*printer.logicalDpiY()); // 1 mm margins
      int height = printer.height() - 2*margin;
      int width = int(height*aspect);
      int x = (printer.width()- width)/2;
      rect = QRect(x, margin, width, height);
    } else if (aspect >= page_aspect){
      int margin = (int) ((0.1/2.54)*printer.logicalDpiX()); // 1 mm margins
      int width = printer.width() - 2*margin;
      int height = int(width/aspect);
      int y = (printer.height()- height)/2;
      rect = QRect(margin, y, width, height);
    }
  } else {
    int x_margin = (int) ((0.1/2.54)*printer.logicalDpiX()); // 1 mm margins
    int y_margin = (int) ((0.1/2.54)*printer.logicalDpiY()); // 1 mm margins
    int width = printer.width() - 2*x_margin;
    int height = printer.height() - 2*y_margin;
    rect = QRect(x_margin, y_margin, width, height);
  }

  QPainter paint(&printer);
  paint.drawImage(rect, d_matrix_model->renderImage());
  paint.end();
}

void Matrix::range(double *min, double *max)
{
  double d_min = cell(0, 0);
  double d_max = d_min;
  int rows = numRows();
  int cols = numCols();

  for(int i=0; i<rows; i++){
    for(int j=0; j<cols; j++){
      double aux = cell(i, j);
      if (aux <= d_min)
        d_min = aux;

      if (aux >= d_max)
        d_max = aux;
    }
  }

  *min = d_min;
  *max = d_max;
}

double** Matrix::allocateMatrixData(int rows, int columns)
{
  double** data = (double **)malloc(rows * sizeof (double*));
  if(!data){
    QMessageBox::critical(0, tr("MantidPlot") + " - " + tr("Memory Allocation Error"),
        tr("Not enough memory, operation aborted!"));
    return NULL;
  }

  for ( int i = 0; i < rows; ++i){
    data[i] = (double *)malloc(columns * sizeof (double));
    if(!data[i]){
      for ( int j = 0; j < i; j++)
        free(data[j]);
      free(data);

      QMessageBox::critical(0, tr("MantidPlot") + " - " + tr("Memory Allocation Error"),
          tr("Not enough memory, operation aborted!"));
      return NULL;
    }
  }
  return data;
}

void Matrix::freeMatrixData(double **data, int rows)
{
  for ( int i = 0; i < rows; i++)
    free(data[i]);

  free(data);
}

void Matrix::goToRow(int row)
{
  if(row < 1 || row > numRows())
    return;

  if (d_view_type == ImageView)
    d_undo_stack->push(new MatrixSetViewCommand(this, d_view_type, TableView, tr("Set Data Mode")));
  d_table_view->selectRow(row - 1);
  d_table_view->scrollTo(d_matrix_model->index(row - 1, 0), QAbstractItemView::PositionAtTop);
}

void Matrix::goToColumn(int col)
{
  if(col < 1 || col > numCols())
    return;

  if (d_view_type == ImageView)
    d_undo_stack->push(new MatrixSetViewCommand(this, d_view_type, TableView, tr("Set Data Mode")));
  d_table_view->selectColumn(col - 1);
  d_table_view->scrollTo(d_matrix_model->index(0, col - 1), QAbstractItemView::PositionAtCenter);
}

void Matrix::moveCell(const QModelIndex& index)
{
  if (!index.isValid())
    return;

  d_table_view->setCurrentIndex(d_matrix_model->index(index.row() + 1, index.column()));
}

void Matrix::copy(Matrix *m)
{
  if (!m)
    return;

  MatrixModel *mModel = m->matrixModel();
  if (!mModel)
    return;

  x_start = m->xStart();
  x_end = m->xEnd();
  y_start = m->yStart();
  y_end = m->yEnd();

  int rows = numRows();
  int cols = numCols();

  txt_format = m->textFormat();
  num_precision = m->precision();

  double *data = d_matrix_model->dataVector();
  double *m_data = mModel->dataVector();
  int size = rows*cols;
  for (int i=0; i<size; i++)
    data[i] = m_data[i];

  d_header_view_type = m->headerViewType();
  d_view_type = m->viewType();
  setColumnsWidth(m->columnsWidth());
  formula_str = m->formula();
  d_color_map_type = m->colorMapType();
  d_color_map = m->colorMap();

  if (d_view_type == ImageView){
    if (d_table_view)
      delete d_table_view;
    if (d_select_all_shortcut)
      delete d_select_all_shortcut;
    initImageView();
    d_stack->setCurrentWidget(imageLabel);
  }
  resetView();
}

void Matrix::displayImage(const QImage& image)
{
  if (!imageLabel)
    return;

  QImage im(imageLabel->size(), QImage::Format_RGB32);
  im.fill(0);
  QPainter p(&im);
  p.drawImage(0, 0, image.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
  p.end();
  imageLabel->setPixmap(QPixmap::fromImage(im));
}

void Matrix::setViewType(ViewType type, bool renderImage)
{
  if (d_view_type == type)
    return;

  d_view_type = type;

  if (d_view_type == ImageView){
    delete d_table_view;
    delete d_select_all_shortcut;
    initImageView();
    if (renderImage)
      displayImage(d_matrix_model->renderImage());
    d_stack->setCurrentWidget(imageLabel);
  } else if (d_view_type == TableView){
    delete imageLabel;
    initTableView();
    d_stack->setCurrentWidget(d_table_view);
  }
  emit modifiedWindow(this);
}

void Matrix::initImageView()
{
  imageLabel = new QLabel();
  imageLabel->setBackgroundRole(QPalette::Base);
  imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
  imageLabel->setScaledContents(true);
  d_stack->addWidget(imageLabel);
}

void Matrix::initTableView()
{
  d_table_view = new QTableView();
  d_table_view->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
  d_table_view->setSelectionMode(QAbstractItemView::ContiguousSelection);// only one contiguous selection supported
  d_table_view->setModel(d_matrix_model);
  d_table_view->setEditTriggers(QAbstractItemView::DoubleClicked);
  d_table_view->setFocusPolicy(Qt::StrongFocus);
  d_table_view->setFocus();

  QPalette pal = d_table_view->palette();
  pal.setColor(QColorGroup::Base, m_bk_color);
  d_table_view->setPalette(pal);

  // set header properties
  QHeaderView* hHeader = (QHeaderView*)d_table_view->horizontalHeader();
  hHeader->setMovable(false);
  hHeader->setResizeMode(QHeaderView::Fixed);
  hHeader->setDefaultSectionSize(d_column_width);

  int cols = numCols();
  for(int i=0; i<cols; i++)
    d_table_view->setColumnWidth(i, d_column_width);

  QHeaderView* vHeader = (QHeaderView*)d_table_view->verticalHeader();
  vHeader->setMovable(false);
  vHeader->setResizeMode(QHeaderView::ResizeToContents);

  d_stack->addWidget(d_table_view);

  // recreate keyboard shortcut
  d_select_all_shortcut = new QShortcut(QKeySequence(tr("Ctrl+A", "Matrix: select all")), this);
  connect(d_select_all_shortcut, SIGNAL(activated()), d_table_view, SLOT(selectAll()));
}

QImage Matrix::image()
{
  return d_matrix_model->renderImage();
}

void Matrix::importImage(const QString& fn)
{
  QImage image(fn);
  if (image.isNull())
    return;

  double *buffer = d_matrix_model->dataCopy();
  if (buffer){
    d_undo_stack->push(new MatrixSetImageCommand(d_matrix_model, image, d_view_type, 0,
        numRows() - 1, 0, numCols() - 1, buffer, tr("Import Image") + " \"" + fn + "\""));
    setWindowLabel(fn);
    emit modifiedWindow(this);
  } else if (ignoreUndo()){
    d_matrix_model->setImage(image);
    setViewType(ImageView, false);
    displayImage(image);
    setWindowLabel(fn);
    emit modifiedWindow(this);
  }
}

void Matrix::setGrayScale()
{
  d_color_map_type = GrayScale;
  d_color_map = QwtLinearColorMap(Qt::black, Qt::white);
  if (d_view_type == ImageView)
    displayImage(d_matrix_model->renderImage());
  emit modifiedWindow(this);
}

void Matrix::setRainbowColorMap()
{
  d_color_map_type = Rainbow;

  d_color_map = QwtLinearColorMap(Qt::blue, Qt::red);
  d_color_map.addColorStop(0.25, Qt::cyan);
  d_color_map.addColorStop(0.5, Qt::green);
  d_color_map.addColorStop(0.75, Qt::yellow);

  if (d_view_type == ImageView)
    displayImage(d_matrix_model->renderImage());
  emit modifiedWindow(this);
}

void Matrix::setColorMap(const QwtLinearColorMap& map)
{
  d_color_map_type = Custom;
  d_color_map = map;
  if (d_view_type == ImageView)
    displayImage(d_matrix_model->renderImage());

  emit modifiedWindow(this);
}

void Matrix::setColorMap(const QStringList& lst)
{
  d_color_map_type = Custom;

  QStringList::const_iterator line = lst.begin();
  QString s = (*line).stripWhiteSpace();

  int mode = s.remove("<Mode>").remove("</Mode>").stripWhiteSpace().toInt();
  s = *(++line);
  QColor color1 = QColor(s.remove("<MinColor>").remove("</MinColor>").stripWhiteSpace());
  s = *(++line);
  QColor color2 = QColor(s.remove("<MaxColor>").remove("</MaxColor>").stripWhiteSpace());

  d_color_map = QwtLinearColorMap(color1, color2);
  d_color_map.setMode((QwtLinearColorMap::Mode)mode);

  s = *(++line);
  int stops = s.remove("<ColorStops>").remove("</ColorStops>").stripWhiteSpace().toInt();
  for (int i = 0; i < stops; i++){
    s = (*(++line)).stripWhiteSpace();
    QStringList l = QStringList::split("\t", s.remove("<Stop>").remove("</Stop>"));
    d_color_map.addColorStop(l[0].toDouble(), QColor(l[1]));
  }
}

void Matrix::setColorMapType(ColorMapType mapType)
{
  d_color_map_type = mapType;

  if (d_color_map_type == GrayScale)
    setGrayScale();
  else if (d_color_map_type == Rainbow)
    setRainbowColorMap();
}

void Matrix::resetView()
{
  if (d_view_type == ImageView)
    displayImage(d_matrix_model->renderImage());
  else if (d_view_type == TableView){
    d_table_view->horizontalHeader()->setDefaultSectionSize(d_column_width);
    d_table_view->horizontalHeader()->reset();
    d_table_view->verticalHeader()->reset();
    d_table_view->reset();
    QSize size = this->size();
    this->resize(QSize(size.width() + 1, size.height()));
    this->resize(size);
  }
}

void Matrix::setHeaderViewType(HeaderViewType type)
{
  if (d_header_view_type == type)
    return;

  d_header_view_type = type;

  if (d_view_type == TableView)
    resetView();
  emit modifiedWindow(this);
}

QwtDoubleRect Matrix::boundingRect()
{
  int rows = numRows();
  int cols = numCols();
  double dx = fabs(x_end - x_start)/(double)(cols - 1);
  double dy = fabs(y_end - y_start)/(double)(rows - 1);

  return QwtDoubleRect(QMIN(x_start, x_end) - 0.5*dx, QMIN(y_start, y_end) - 0.5*dy,
      fabs(x_end - x_start) + dx, fabs(y_end - y_start) + dy).normalized();
}

void Matrix::fft(bool inverse)
{
  double *buffer = d_matrix_model->dataCopy();
  if (buffer){
    QString commandText = inverse ? tr("Inverse FFT") : tr("Forward FFT");
    d_undo_stack->push(new MatrixFftCommand(inverse, d_matrix_model, 0, numRows() - 1,
        0, numCols() - 1, buffer, commandText));
    emit modifiedWindow(this);
  } else if (ignoreUndo()){
    d_matrix_model->fft(inverse);
    emit modifiedWindow(this);
  }
}

bool Matrix::exportASCII(const QString& fname, const QString& separator, bool exportSelection)
{
  QFile f(fname);
  if ( !f.open( QIODevice::WriteOnly ) ){
    QApplication::restoreOverrideCursor();
    QMessageBox::critical(this, tr("MantidPlot - ASCII Export Error"),
        tr("Could not write to file: <br><h4>%1</h4><p>Please verify that you have the right to write to this location!").arg(fname));
    return false;
  }

  int rows = numRows();
  int cols = numCols();
  QTextStream t( &f );
  QString eol = applicationWindow()->endOfLine();
  if (exportSelection && d_view_type == TableView){
    QModelIndexList selectedIndexes = d_table_view->selectionModel()->selectedIndexes();
    int topRow = selectedIndexes[0].row();
    int bottomRow = topRow;
    int leftCol = selectedIndexes[0].column();
    int rightCol = leftCol;
    foreach(QModelIndex index, selectedIndexes){
      int row = index.row();
      if (row < topRow)
        topRow = row;
      if (row > bottomRow)
        bottomRow = row;

      int col = index.column();
      if (col < leftCol)
        leftCol = col;
      if (col > rightCol)
        rightCol = col;
    }

    for (int i = topRow; i <= bottomRow; i++){
      for (int j = leftCol; j < rightCol; j++){
        t << d_matrix_model->text(i, j);
        t << separator;
      }
      t << d_matrix_model->text(i, rightCol);
      t << eol;
    }
  } else {
    for (int i=0; i<rows; i++) {
      for (int j=0; j<cols-1; j++){
        t << d_matrix_model->text(i,j);
        t << separator;
      }
      t << d_matrix_model->text(i, cols-1);
      t << eol;
    }
  }
  f.close();
  return true;
}

void Matrix::importASCII(const QString &fname, const QString &sep, int ignoredLines,
    bool stripSpaces, bool simplifySpaces, const QString& commentString,
    ImportMode importAs, const QLocale& locale, int endLineChar, int maxRows)
{
  double *buffer = d_matrix_model->dataCopy();
  if (buffer){
    d_undo_stack->push(new MatrixImportAsciiCommand(fname, sep, ignoredLines, stripSpaces,
        simplifySpaces, commentString, importAs, locale, endLineChar, maxRows,
        d_matrix_model, 0, numRows() - 1, 0, numCols() - 1, buffer,
        tr("Import ASCII File") + " \"" + fname + "\""));
    emit modifiedWindow(this);
  } else if (ignoreUndo()){
    d_matrix_model->importASCII(fname, sep, ignoredLines, stripSpaces, simplifySpaces,
        commentString, importAs, locale, endLineChar, maxRows);
    emit modifiedWindow(this);
  }
}

bool Matrix::ignoreUndo()
{
  QString msg = tr("Due to memory limitations it will not be possible to undo this change. Do you want to continue anyways?");
  return (QMessageBox::Yes == QMessageBox::warning((ApplicationWindow *)applicationWindow(),
      tr("MantidPlot") + " - " + tr("Warning"), msg, QMessageBox::Yes, QMessageBox::Cancel));
}

double* Matrix::initWorkspace(int size)
{
  if (!d_workspace)
    d_workspace = (double *)malloc(size * sizeof (double));

  if (!d_workspace)
    QMessageBox::critical((ApplicationWindow *)applicationWindow(), tr("MantidPlot") + " - " + tr("Memory Allocation Error"),
        tr("Not enough memory, operation aborted!"));

  return d_workspace;
}

Matrix::~Matrix()
{
  delete d_undo_stack;
  delete d_matrix_model;
}
