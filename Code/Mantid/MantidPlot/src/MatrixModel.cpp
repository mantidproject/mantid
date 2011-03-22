/***************************************************************************
	File                 : MatrixModel.cpp
	Project              : QtiPlot
--------------------------------------------------------------------
	Copyright            : (C) 2007 by Ion Vasilief
	Email (use @ for *)  : ion_vasilief*yahoo.fr
	Description          : QtiPlot's matrix model

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
#include <QtGui>
#include <QFile>
#include <QTextStream>

#include "Matrix.h"
#include "MatrixModel.h"
#include "MatrixCommand.h"
#include "muParserScript.h"
#include "ScriptingEnv.h"
#include "analysis/fft2D.h"

#include <gsl/gsl_math.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_errno.h>

#include <qwt_color_map.h>
#include <stdlib.h>

MatrixModel::MatrixModel(QObject *parent)
     : QAbstractTableModel(parent),
	 d_matrix((Matrix*)parent)
{
	init();
	
	if (d_matrix){
		d_txt_format = d_matrix->textFormat().toAscii();
		d_num_precision = d_matrix->precision();
		d_locale = d_matrix->locale();
	}

	d_rows = 0;
	d_cols = 0;

}

MatrixModel::MatrixModel(int rows, int cols, QObject *parent)
     : QAbstractTableModel(parent),
	 d_matrix((Matrix*)parent)
{
	init();
	
	if (d_matrix){
		d_txt_format = d_matrix->textFormat().toAscii();
		d_num_precision = d_matrix->precision();
		d_locale = d_matrix->locale();
	}

	if (canResize(rows, cols)){
		d_rows = rows;
		d_cols = cols;
		int cell = 0;
		int size = rows*cols;
		for (int i = 0; i < size; i++)
            d_data[cell++] = GSL_NAN;
	}
}

MatrixModel::MatrixModel(const QImage& image, QObject *parent)
     : QAbstractTableModel(parent),
	 d_matrix((Matrix*)parent)
{
	init();
    setImage(image);
}

void MatrixModel::init()
{
	d_txt_format = 'g';
	d_num_precision = 6;
	d_locale = QLocale();
	d_direct_matrix = NULL;
	d_inv_matrix = NULL;
	d_inv_perm = NULL;
	
	d_rows = 1;
	d_cols = 1;
	d_data_block_size = QSize(1, 1);
	d_data = (double *)malloc(sizeof(double));
}

void MatrixModel::setImage(const QImage& image)
{
	if (!canResize(image.height(), image.width()))
		return;

    d_rows = image.height();
    d_cols = image.width();
	int cell = 0;
    for (int i=0; i<d_rows; i++ ){
		for (int j=0; j<d_cols; j++)
            d_data[cell++] = qGray(image.pixel (j, i));
    }
}

Qt::ItemFlags MatrixModel::flags(const QModelIndex & index ) const
{
	if (index.isValid())
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	else
		return Qt::ItemIsEnabled;
}

int MatrixModel::rowCount(const QModelIndex & /* parent */) const
{
    return d_rows;
}

int MatrixModel::columnCount(const QModelIndex & /* parent */) const
{
    return d_cols;
}

void MatrixModel::setRowCount(int rows)
{
	if (d_rows == rows)
		return;

   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	if (rows > d_rows )
		insertRows(d_rows, rows - d_rows);
    else if (rows < d_rows )
		removeRows(rows, d_rows - rows);

	QApplication::restoreOverrideCursor();
}

void MatrixModel::setColumnCount(int cols)
{
	if (d_cols == cols)
		return;

   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	if (cols > d_cols )
		insertColumns(d_cols, cols - d_cols);
    else if (cols < d_cols )
		removeColumns(cols, d_cols - cols);

	QApplication::restoreOverrideCursor();
}

void MatrixModel::setDimensions(int rows, int cols)
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	if (rows < d_rows){//first remove rows (faster)
		removeRows(rows, d_rows - rows);
		setColumnCount(cols);
	} else {
		setColumnCount(cols);
		if (rows > d_rows )
			insertRows(d_rows, rows - d_rows);
	}

	QApplication::restoreOverrideCursor();
}

double MatrixModel::cell(int row, int col) const
{
    int i = d_cols*row + col;
    double val = d_data[i];
    if (i < 0 || i>= d_rows*d_cols || gsl_isnan (val))
        return 0.0;

	return val;
}

void MatrixModel::setCell(int row, int col, double val)
{
	int i = d_cols*row + col;
    if (i < 0 || i>= d_rows*d_cols)
		return;

    d_data[i] = val;
}

QString MatrixModel::text(int row, int col)
{
	double val = cell(row,col);

	if (d_matrix){
		QLocale locale = d_matrix->locale();
		return locale.toString(val, d_matrix->textFormat().toAscii(), d_matrix->precision());
	}
	return d_locale.toString(val, d_txt_format, d_num_precision);
}

void MatrixModel::setText(int row, int col, const QString& text)
{
	int i = d_cols*row + col;
    if (i < 0 || i>= d_rows*d_cols)
		return;

 	if (text.isEmpty())
		d_data[i] = GSL_NAN;
	else {
		if (d_matrix)
			d_data[i] = d_matrix->locale().toDouble(text);
		else
			d_data[i] = d_locale.toDouble(text);
	}
}

double MatrixModel::data(int row, int col) const
{
    int i = d_cols*row + col;
    if (i < 0 || i>= d_rows*d_cols)
        return 0.0;

	return d_data[i];
}

double MatrixModel::x(int col) const
{
	if (col < 0 || col >= d_cols)
        return 0.0;

	double start = d_matrix->xStart();
	double end = d_matrix->xEnd();
	if (start < end)
		return start + col*d_matrix->dx();
	else
		return start - col*d_matrix->dx();

	return 0.0;
}

double MatrixModel::y(int row) const
{
	if (row < 0 || row >= d_rows)
        return 0.0;

	double start = d_matrix->yStart();
	double end = d_matrix->yEnd();
	if (start < end)
		return start + row*d_matrix->dy();
	else
		return start - row*d_matrix->dy();

	return 0.0;
}

QVariant MatrixModel::headerData ( int section, Qt::Orientation orientation, int role) const
{
    if (!d_matrix || d_matrix->headerViewType() == Matrix::ColumnRow)
        return QAbstractItemModel::headerData(section, orientation, role);

    QLocale locale = d_locale;
	int prec = d_num_precision;
	char fmt = d_txt_format;
	if (d_matrix){
		locale = d_matrix->locale();
		fmt = d_matrix->textFormat().toAscii();
		prec = d_matrix->precision();
	}

    if (role == Qt::DisplayRole || role == Qt::EditRole){
        if (orientation == Qt::Horizontal){
            double start = d_matrix->xStart();
            double end = d_matrix->xEnd();
            double dx = d_matrix->dx();
            if (start < end)
                return QVariant(locale.toString(start + section*dx, fmt, prec));
            else
                return QVariant(locale.toString(start - section*dx, fmt, prec));
        } else if (orientation == Qt::Vertical){
            double start = d_matrix->yStart();
            double end = d_matrix->yEnd();
            double dy = d_matrix->dy();
            if (start < end)
                return QVariant(locale.toString(start + section*dy, fmt, prec));
            else
                return QVariant(locale.toString(start - section*dy, fmt, prec));
        }
    }
	return QAbstractItemModel::headerData(section, orientation, role);
}

QVariant MatrixModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
		return QVariant();

    int i = d_cols*index.row() + index.column();
	double val = d_data[i];
    if (gsl_isnan (val))
        return QVariant();

	if (role == Qt::DisplayRole || role == Qt::EditRole){
		if (d_matrix)
			return QVariant(d_matrix->locale().toString(val, d_matrix->textFormat().toAscii(), d_matrix->precision()));
		else
			return QVariant(d_locale.toString(val, d_txt_format, d_num_precision));
	} else
		return QVariant();
}

bool MatrixModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
	if (!index.isValid())
		return false;

	int i = d_cols*index.row() + index.column();
	double valBefore = d_data[i];
 	if(role == Qt::EditRole){
		if (value.toString().isEmpty())
			d_data[i] = GSL_NAN;
		else
			d_data[i] = value.toDouble();
	}

	if(index.row() + 1 >= d_rows){
        insertRows(d_rows, 1);
		d_matrix->resetView();
	}

    d_matrix->undoStack()->push(new MatrixEditCellCommand(this, index, valBefore, d_data[i],
                                tr("Edited cell") + " (" + QString::number(index.row() + 1) + "," +
                                QString::number(index.column() + 1) + ")"));
    d_matrix->notifyChanges();
	d_matrix->moveCell(index);
	return false;
}

bool MatrixModel::canResize(int rows, int cols)
{
	if (rows <= 0 || cols <= 0 || INT_MAX/rows < cols){ //avoid integer overflow
		QApplication::restoreOverrideCursor();
    	QMessageBox::critical(d_matrix, tr("MantidPlot") + " - " + tr("Input Size Error"),
    	tr("The dimensions you have specified are not acceptable!") + "\n" +
		tr("Please enter positive values for which the product rows*columns does not exceed the maximum integer value available on your system!"));
		return false;
	}
	
    if (d_data_block_size.width()*d_data_block_size.height() >= rows*cols)
		return true;

    double *new_data = (double *)realloc(d_data, rows*cols*sizeof(double));
    if (new_data){
        d_data = new_data;
		d_data_block_size = QSize(rows, cols);
        return true;
    }

    QApplication::restoreOverrideCursor();
    QMessageBox::critical(d_matrix, tr("MantidPlot") + " - " + tr("Memory Allocation Error"),
    tr("Not enough memory, operation aborted!"));
    return false;
}

bool MatrixModel::removeColumns(int column, int count, const QModelIndex & parent)
{
	beginRemoveColumns(parent, column, column + count - 1);

    d_cols -= count;
	d_data_block_size = QSize(d_rows, d_cols);

    int size = d_rows*d_cols;
    for (int i = column; i < size; i++){
	    int aux = (i - column)/d_cols + 1;
        d_data[i] = d_data[i + aux*count];
	}

    d_data = (double *)realloc (d_data, size*sizeof(double));

	endRemoveColumns();
	return true;
}

bool MatrixModel::insertColumns(int column, int count, const QModelIndex & parent)
{
    if (!canResize(d_rows, d_cols + count))
        return false;

	beginInsertColumns(parent, column, column + count - 1);

    int offset = column + count;
    int oldCell = d_rows*d_cols - 1;
	d_cols += count;
    int cell = d_rows*d_cols - 1;
    for (int i = d_rows - 1; i >= 0; i--){
        for (int j = d_cols - 1; j >= offset; j--)
            d_data[cell--] = d_data[oldCell--];

        for (int j = offset - 1; j >= column; j--)
            d_data[cell--] = GSL_NAN;

        for (int j = column - 1; j >= 0; j--)
            d_data[cell--] = d_data[oldCell--];
    }

	endInsertColumns();
	return true;
}

bool MatrixModel::insertRows(int row, int count, const QModelIndex & parent)
{
    if (!canResize(d_rows + count, d_cols))
        return false;

	beginInsertRows(parent, row, row + count - 1);

    int oldSize = d_rows*d_cols;
	d_rows += count;

    int insertedCells = count*d_cols;
    int startCell = row*d_cols;
    for (int i = oldSize - 1; i >= startCell; i--)
        d_data[i + insertedCells] = d_data[i];
    for (int i = 0; i < insertedCells; i++)
        d_data[startCell++] = GSL_NAN;

	endInsertRows();
	return true;
}

bool MatrixModel::removeRows(int row, int count, const QModelIndex & parent)
{
	beginRemoveRows(parent, row, row + count - 1);

    d_rows -= count;
	d_data_block_size = QSize(d_rows, d_cols);

	int removedCells = count*d_cols;
	int size = d_rows*d_cols;
	for (int i = row*d_cols; i < size; i++)
        d_data[i] = d_data[i + removedCells];

    d_data = (double *)realloc(d_data, size * sizeof(double));

	endRemoveRows();
	return true;
}

QString MatrixModel::saveToString()
{
	QString s = "<data>\n";
	int cols = d_cols - 1;
	for(int i = 0; i < d_rows; i++){
		int aux = d_cols*i;
		bool emptyRow = true;
		for(int j = 0; j < d_cols; j++){
			if (gsl_finite(d_data[aux + j])){
				emptyRow = false;
				break;
			}
		}
		if (emptyRow)
			continue;

		s += QString::number(i) + "\t";
		for(int j = 0; j < cols; j++){
			double val = d_data[aux + j];
			if (gsl_finite(val))
				s += QString::number(val, 'e', 16);
			s += "\t";
		}
		double val = d_data[aux + cols];
		if (gsl_finite(val))
			s += QString::number(val, 'e', 16);
		s += "\n";
	}
	return s + "</data>\n";
}

QImage MatrixModel::renderImage()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	
	QImage image(QSize(d_cols, d_rows), QImage::Format_RGB32);
	QwtLinearColorMap color_map = d_matrix->colorMap();

	double minValue = 0.0, maxValue = 0.0;
	d_matrix->range(&minValue, &maxValue);
    const QwtDoubleInterval intensityRange = QwtDoubleInterval (minValue, maxValue);
    for ( int i = 0; i < d_rows; i++ ){
    	QRgb *line = (QRgb *)image.scanLine(i);
		for ( int j = 0; j < d_cols; j++){
		    double val = cell(i,j);//d_data[i*d_cols + j];
		    if (gsl_isnan (val))
                *line++ = color_map.rgb(intensityRange, 0.0);
			else if(fabs(val) < HUGE_VAL)
				*line++ = color_map.rgb(intensityRange, val);
		}
     }
     QApplication::restoreOverrideCursor();
	 return image;
}

bool MatrixModel::importASCII(const QString &fname, const QString &sep, int ignoredLines,
    bool stripSpaces, bool simplifySpaces, const QString& commentString, int importAs,
	const QLocale& locale, int endLineChar, int maxRows)
{
	int rows = 0;
	QString name = MdiSubWindow::parseAsciiFile(fname, commentString, endLineChar, ignoredLines, maxRows, rows);
	if (name.isEmpty())
		return false;
	QFile f(name);
	if (!f.open(QIODevice::ReadOnly))
		return false;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QTextStream t(&f);
	QLocale l = d_locale;
	if (d_matrix)
		l = d_matrix->locale();
	bool updateDecimalSeparators = (l != locale) ? true : false;

	QString s = t.readLine();
	if (simplifySpaces)
		s = s.simplifyWhiteSpace();
	else if (stripSpaces)
		s = s.stripWhiteSpace();

	QStringList line = s.split(sep);
	int cols = line.size();

	int startRow = 1, startCol = 0;
	switch(importAs){
		case Matrix::Overwrite:
			if (d_cols != cols)
				setColumnCount(cols);
			if (d_rows != rows)
				setRowCount(rows);
		break;
		case Matrix::NewColumns:
			startCol = d_cols;
			setColumnCount(d_cols + cols);
			if (d_rows < rows)
				setRowCount(rows);
		break;
		case Matrix::NewRows:
			startRow = d_rows;
			if (d_cols < cols)
				setColumnCount(cols);
			setRowCount(d_rows + rows);
		break;
	}

	for (int j = startCol; j<d_cols; j++){
		int aux = j - startCol;
		if (cols > aux){
			if (updateDecimalSeparators)
				setCell(0, j, locale.toDouble(line[aux]));
			else
				setText(0, j, line[aux]);
		}
	}

	qApp->processEvents(QEventLoop::ExcludeUserInput);
	for (int i = startRow; i<d_rows; i++){
		s = t.readLine();
		if (simplifySpaces)
			s = s.simplifyWhiteSpace();
		else if (stripSpaces)
			s = s.stripWhiteSpace();
		line = s.split(sep);
		int lc = line.size();
		if (lc > cols)
			setColumnCount(d_cols + lc - cols);

		for (int j = startCol; j<d_cols; j++){
			int aux = j - startCol;
		    if (lc > aux){
				if (updateDecimalSeparators)
					setCell(i, j, locale.toDouble(line[aux]));
				else
					setText(i, j, line[aux]);
			}
		}
	}
    f.remove();	//remove temp file
	if (d_matrix)
		d_matrix->resetView();
	QApplication::restoreOverrideCursor();
	return true;
}

void MatrixModel::setNumericFormat(char f, int prec)
{
	if (d_txt_format == f && d_num_precision == prec)
		return;

	d_txt_format = f;
	d_num_precision = prec;
}

void MatrixModel::transpose()
{
	int size = d_rows*d_cols;
	double *data = d_matrix->initWorkspace(size);
	if (!data)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	for(int i = 0; i < size; i++)
		data[i] = d_data[i];

	int old_cols = d_cols;
	d_cols = d_rows;
	d_rows = old_cols;
	int aux = 0;
	for(int i = 0; i < d_rows; i++){
		for(int j = 0; j < d_cols; j++)
			d_data[aux++] = data[j*old_cols + i];
	}
	d_matrix->freeWorkspace();
	QApplication::restoreOverrideCursor();
}

void MatrixModel::flipVertically()
{
	int size = d_rows*d_cols;
	double *data = d_matrix->initWorkspace(size);
	if (!data)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	for(int i = 0; i < size; i++)
		data[i] = d_data[i];

	int aux = 0;
	for(int i = 0; i < d_rows; i++){
		int row = (d_rows - i - 1)*d_cols;
		for(int j = 0; j < d_cols; j++)
			d_data[aux++] = data[row++];
	}
	d_matrix->freeWorkspace();
	QApplication::restoreOverrideCursor();
}

void MatrixModel::flipHorizontally()
{
	int size = d_rows*d_cols;
	double *data = d_matrix->initWorkspace(size);
	if (!data)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	for(int i = 0; i < size; i++)
		data[i] = d_data[i];

	int aux = 0;
	for(int i = 0; i < d_rows; i++){
		int row = i*d_cols;
		for(int j = d_cols - 1; j >= 0; j--)
			d_data[aux++] = data[row + j];
	}
	d_matrix->freeWorkspace();
	QApplication::restoreOverrideCursor();
}

void MatrixModel::rotate90(bool clockwise)
{
	int size = d_rows*d_cols;
	double *data = d_matrix->initWorkspace(size);
	if (!data)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	for(int i = 0; i < size; i++)
		data[i] = d_data[i];

    int old_rows = d_rows;
    int old_cols = d_cols;
	d_cols = d_rows;
	d_rows = old_cols;
	if (clockwise){
        int cell = 0;
        int aux = old_rows - 1;
        for(int i = 0; i < d_rows; i++){
            for(int j = 0; j < d_cols; j++)
                d_data[cell++] = data[(aux - j)*old_cols + i];
        }
	} else {
	    int cell = 0;
	    int aux = old_cols - 1;
        for(int i = 0; i < d_rows; i++){
            int k = aux - i;
            for(int j = 0; j < d_cols; j++)
                d_data[cell++] = data[j*old_cols + k];
        }
	}
	d_matrix->freeWorkspace();
	QApplication::restoreOverrideCursor();
}

bool MatrixModel::initWorkspace()
{
	gsl_set_error_handler_off();

	if (!d_direct_matrix)
		d_direct_matrix = gsl_matrix_alloc(d_rows, d_cols);
	if (!d_inv_matrix)
		d_inv_matrix = gsl_matrix_alloc(d_rows, d_cols);
	if (!d_inv_perm)
    	d_inv_perm = gsl_permutation_alloc(d_cols);
	if (!d_direct_matrix || !d_inv_matrix || !d_inv_perm){
		QApplication::restoreOverrideCursor();
		QMessageBox::critical(d_matrix, tr("MantidPlot") + " - " + tr("Memory Allocation Error"),
		tr("Not enough memory, operation aborted!"));
		return false;
	}
	return true;
}

void MatrixModel::invert()
{
	initWorkspace();
	if(!d_direct_matrix || !d_inv_matrix || !d_inv_perm)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	int i, aux = 0;
	for(i=0; i<d_rows; i++){
		for(int j=0; j<d_cols; j++)
			gsl_matrix_set(d_direct_matrix, i, j, d_data[aux++]);
	}

	gsl_linalg_LU_decomp(d_direct_matrix, d_inv_perm, &i);
	gsl_linalg_LU_invert(d_direct_matrix, d_inv_perm, d_inv_matrix);

	gsl_matrix_free(d_direct_matrix);
	d_direct_matrix = NULL;
	gsl_permutation_free(d_inv_perm);
	d_inv_perm = NULL;

    aux = 0;
	for(int i=0; i<d_rows; i++){
		for(int j=0; j<d_cols; j++)
			d_data[aux++] = gsl_matrix_get(d_inv_matrix, i, j);
	}
	gsl_matrix_free(d_inv_matrix);
	d_inv_matrix = NULL;
	QApplication::restoreOverrideCursor();
}

void MatrixModel::clear(int startRow, int endRow, int startCol, int endCol)
{
	if (endRow < 0)
		endRow = d_rows - 1;
	if (endCol < 0)
		endCol = d_cols - 1;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    for (int i = startRow; i <= endRow; i++){
        int aux = i*d_cols + startCol;
        for (int j = startCol; j <= endCol; j++){
            d_data[aux++] = GSL_NAN;
        }
    }
	QApplication::restoreOverrideCursor();
}

double * MatrixModel::dataCopy(int startRow, int endRow, int startCol, int endCol)
{
	if (endRow < 0)
		endRow = d_rows - 1;
	if (endCol < 0)
		endCol = d_cols - 1;

    double *buffer = (double *)malloc((endRow - startRow + 1)*(endCol - startCol + 1) * sizeof (double));
	if (!buffer)
		return NULL;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	int aux = 0;
    for (int i = startRow; i <= endRow; i++){
        int row = i*d_cols + startCol;
       	for (int j = startCol; j <= endCol; j++){
            buffer[aux++] = d_data[row++];
        }
    }

	QApplication::restoreOverrideCursor();
	return buffer;
}

bool MatrixModel::muParserCalculate(int startRow, int endRow, int startCol, int endCol)
{
	ScriptingEnv *scriptEnv = d_matrix->scriptingEnv();
    muParserScript *mup = new muParserScript(scriptEnv, d_matrix->formula(), d_matrix, QString("<%1>").arg(d_matrix->objectName()));
	connect(mup, SIGNAL(error(const QString&,const QString&,int)), scriptEnv, SIGNAL(error(const QString&,const QString&,int)));
	connect(mup, SIGNAL(print(const QString&)), scriptEnv, SIGNAL(print(const QString&)));

	if (endRow < 0)
		endRow = d_rows - 1;
	if (endCol < 0)
		endCol = d_cols - 1;
    if (endCol >= d_cols)
		setColumnCount(endCol + 1);
	if (endRow >= d_rows)
        setRowCount(endRow + 1);

	double dx = d_matrix->dx();
	double dy = d_matrix->dy();
    double *ri = mup->defineVariable("i");
    double *rr = mup->defineVariable("row");
    double *cj = mup->defineVariable("j");
    double *cc = mup->defineVariable("col");
    double *x = mup->defineVariable("x");
    double *y = mup->defineVariable("y");

	if (!mup->compile()){
		QApplication::restoreOverrideCursor();
		return false;
	}

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	double x_start = d_matrix->xStart();
	double y_start = d_matrix->yStart();
    if (mup->codeLines() == 1){
        for(int row = startRow; row <= endRow; row++){
            double r = row + 1.0;
            *ri = r; *rr = r;
            *y = y_start + row*dy;
            int aux = row*d_cols + startCol;
            for(int col = startCol; col <= endCol; col++){
                double c = col + 1.0;
                *cj = c; *cc = c;
                *x = x_start + col*dx;
                d_data[aux++] = mup->evalSingleLine();
            }
        }
    } else {
        QVariant res;
        for(int row = startRow; row <= endRow; row++){
            double r = row + 1.0;
            *ri = r; *rr = r;
            *y = y_start + row*dy;
            int aux = row*d_cols + startCol;
            for(int col = startCol; col <= endCol; col++){
                double c = col + 1.0;
                *cj = c; *cc = c;
                *x = x_start + col*dx;
                res = mup->eval();
                if (res.canConvert(QVariant::Double))
                     d_data[aux++] = res.toDouble();
                else
                    d_data[aux++] = GSL_NAN;
                qApp->processEvents();
            }
		}
	}
	QApplication::restoreOverrideCursor();
	return true;
}

bool MatrixModel::calculate(int startRow, int endRow, int startCol, int endCol)
{
	QString formula = d_matrix->formula();
	if (formula.isEmpty())
		return false;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	ScriptingEnv *scriptEnv = d_matrix->scriptingEnv();
	  Script *script = scriptEnv->newScript(formula, this, QString("<%1>").arg(objectName()), false);
	connect(script, SIGNAL(error(const QString&,const QString&,int)), scriptEnv, SIGNAL(error(const QString&,const QString&,int)));
	connect(script, SIGNAL(print(const QString&)), scriptEnv, SIGNAL(print(const QString&)));
	
	if (!script->compile()){
		QApplication::restoreOverrideCursor();
		return false;
	}

	if (endRow < 0)
		endRow = d_rows - 1;
	if (endCol < 0)
		endCol = d_cols - 1;
    if (endCol >= d_cols)
		setColumnCount(endCol + 1);
	if (endRow >= d_rows)
        setRowCount(endRow + 1);

	QVariant res;
	double dx = d_matrix->dx();
	double dy = d_matrix->dy();
	double x_start = d_matrix->xStart();
	double y_start = d_matrix->yStart();
	double r = 0.0, c = 0.0;
	for(int row = startRow; row <= endRow; row++){
	    r = row + 1.0;
		script->setDouble(r, "i");
		script->setDouble(r, "row");
		script->setDouble(y_start + row*dy, "y");
		int aux = row*d_cols + startCol;
		for(int col = startCol; col <= endCol; col++){
		    c = col + 1.0;
			script->setDouble(c, "j");
			script->setDouble(c, "col");
			script->setDouble(x_start + col*dx, "x");
			res = script->eval();
			if (res.canConvert(QVariant::Double))
				d_data[aux++] = res.toDouble();
			else {
				QApplication::restoreOverrideCursor();
				d_data[aux++] = GSL_NAN;
				return false;
			}
		}
		qApp->processEvents();
	}

	QApplication::restoreOverrideCursor();
	return true;
}

void MatrixModel::fft(bool inverse)
{
	int width = d_cols;
    int height = d_rows;

    double **x_int_re = Matrix::allocateMatrixData(height, width); /* real coeff matrix */
    if (!x_int_re)
        return;
    double **x_int_im = Matrix::allocateMatrixData(height, width); /* imaginary coeff  matrix*/
	if (!x_int_im){
        Matrix::freeMatrixData(x_int_re, height);
        return;
	}

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	int cell = 0;
    for (int i = 0; i < height; i++){
        for (int j = 0; j < width; j++){
            x_int_re[i][j] = d_data[cell++];
            x_int_im[i][j] = 0.0;
        }
    }

    if (inverse){
        double **x_fin_re = Matrix::allocateMatrixData(height, width);
        double **x_fin_im = Matrix::allocateMatrixData(height, width);
		if (!x_fin_re || !x_fin_im){
		    Matrix::freeMatrixData(x_int_re, height);
            Matrix::freeMatrixData(x_int_im, height);
			QApplication::restoreOverrideCursor();
			return;
		}

        fft2d_inv(x_int_re, x_int_im, x_fin_re, x_fin_im, width, height);
		cell = 0;
        for (int i = 0; i < height; i++){
            for (int j = 0; j < width; j++){
                double re = x_fin_re[i][j];
                double im = x_fin_im[i][j];
                d_data[cell++] = sqrt(re*re + im*im);
            }
        }
        Matrix::freeMatrixData(x_fin_re, height);
        Matrix::freeMatrixData(x_fin_im, height);
    } else {
        fft2d(x_int_re, x_int_im, width, height);
		cell = 0;
        for (int i = 0; i < height; i++){
            for (int j = 0; j < width; j++){
                double re = x_int_re[i][j];
                double im = x_int_im[i][j];
                d_data[cell++] = sqrt(re*re + im*im);
            }
        }
    }
    Matrix::freeMatrixData(x_int_re, height);
    Matrix::freeMatrixData(x_int_im, height);

	d_matrix->resetView();
	QApplication::restoreOverrideCursor();
}

void MatrixModel::pasteData(double *clipboardBuffer, int topRow, int leftCol, int rows, int cols)
{
	int newCols = leftCol + cols;
	if (newCols > d_cols)
		insertColumns(d_cols, newCols - d_cols);
	
	int newRows = topRow + rows;
	if (newRows > d_rows)
		insertRows(d_rows, newRows - d_rows);
		
	int cell = 0;
	int bottomRow = newRows - 1;
	int rightCol = newCols - 1;
    for (int i = topRow; i <= bottomRow; i++){
        int row = i*d_cols + leftCol;
        for (int j = leftCol; j <= rightCol; j++)
            d_data[row++] = clipboardBuffer[cell++];
    }
}


MatrixModel::~MatrixModel()
{
    free(d_data);
}
