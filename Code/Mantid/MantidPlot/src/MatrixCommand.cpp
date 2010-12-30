/***************************************************************************
    File                 : MatrixCommand.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2008 by Ion Vasilief,
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Matrix undo/redo commands

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

#include "MatrixCommand.h"
#include <QApplication>
#include <gsl/gsl_math.h>

/*************************************************************************/
/*           Class MatrixEditCellCommand                                 */
/*************************************************************************/
MatrixEditCellCommand::MatrixEditCellCommand(MatrixModel *model, const QModelIndex & index,
                        double valBefore, double valAfter, const QString & text):
QUndoCommand(text),
d_model(model),
d_index(index),
d_val_before(valBefore),
d_val_after(valAfter)
{
    setText(model->matrix()->objectName() + ": " + text);
}

void MatrixEditCellCommand::redo()
{
    if (!d_model)
        return;

    Matrix *m = d_model->matrix();
    if (m){
        d_model->setCell(d_index.row(), d_index.column(), d_val_after);
        m->resetView();
        m->notifyChanges();
    }
}

void MatrixEditCellCommand::undo()
{
    if (!d_model)
        return;

    Matrix *m = d_model->matrix();
    if (m){
        d_model->setCell(d_index.row(), d_index.column(), d_val_before);
        m->resetView();
        m->notifyChanges();
    }
}

/*************************************************************************/
/*           Class MatrixSetFormulaCommand                               */
/*************************************************************************/
MatrixSetFormulaCommand::MatrixSetFormulaCommand(Matrix *m, const QString& oldFormula,
                        const QString& newFormula, const QString & text):
QUndoCommand(text),
d_matrix(m),
d_old_formula(oldFormula),
d_new_formula(newFormula)
{
    setText(m->objectName() + ": " + text);
}

void MatrixSetFormulaCommand::redo()
{
    if (!d_matrix)
        return;

    d_matrix->setFormula(d_new_formula);
}

void MatrixSetFormulaCommand::undo()
{
    if (!d_matrix)
        return;

    d_matrix->setFormula(d_old_formula);
}

/*************************************************************************/
/*           Class MatrixSetViewCommand                                  */
/*************************************************************************/
MatrixSetViewCommand::MatrixSetViewCommand(Matrix *m, Matrix::ViewType oldView,
                    Matrix::ViewType newView, const QString & text):
QUndoCommand(text),
d_matrix(m),
d_old_view(oldView),
d_new_view(newView)
{
    setText(m->objectName() + ": " + text);
}

void MatrixSetViewCommand::redo()
{
    if (!d_matrix)
        return;

    d_matrix->setViewType(d_new_view);
}

void MatrixSetViewCommand::undo()
{
    if (!d_matrix)
        return;

    d_matrix->setViewType(d_old_view);
}

/*************************************************************************/
/*           Class MatrixSetHeaderViewCommand                            */
/*************************************************************************/
MatrixSetHeaderViewCommand::MatrixSetHeaderViewCommand(Matrix *m, Matrix::HeaderViewType oldView,
                    Matrix::HeaderViewType newView, const QString & text):
QUndoCommand(text),
d_matrix(m),
d_old_view(oldView),
d_new_view(newView)
{
    setText(m->objectName() + ": " + text);
}

void MatrixSetHeaderViewCommand::redo()
{
    if (!d_matrix)
        return;

    d_matrix->setHeaderViewType(d_new_view);
}

void MatrixSetHeaderViewCommand::undo()
{
    if (!d_matrix)
        return;

    d_matrix->setHeaderViewType(d_old_view);
}

/*************************************************************************/
/*           Class MatrixSetColWidthCommand                              */
/*************************************************************************/
MatrixSetColWidthCommand::MatrixSetColWidthCommand(Matrix *m, int oldWidth, int newWidth, const QString & text):
QUndoCommand(text),
d_matrix(m),
d_old_width(oldWidth),
d_new_width(newWidth)
{
    setText(m->objectName() + ": " + text);
}

void MatrixSetColWidthCommand::redo()
{
    if (!d_matrix)
        return;

    d_matrix->setColumnsWidth(d_new_width);
}

void MatrixSetColWidthCommand::undo()
{
    if (!d_matrix)
        return;

    d_matrix->setColumnsWidth(d_old_width);
}

/*************************************************************************/
/*           Class MatrixSetPrecisionCommand                             */
/*************************************************************************/
MatrixSetPrecisionCommand::MatrixSetPrecisionCommand(Matrix *m, const QChar& oldFormat,
                const QChar& newFormat, int oldPrec, int newPrec, const QString & text):
QUndoCommand(text),
d_matrix(m),
d_old_format(oldFormat),
d_new_format(newFormat),
d_old_prec(oldPrec),
d_new_prec(newPrec)
{
    setText(m->objectName() + ": " + text);
}

void MatrixSetPrecisionCommand::redo()
{
    if (!d_matrix)
        return;

    d_matrix->setNumericFormat(d_new_format, d_new_prec);
}

void MatrixSetPrecisionCommand::undo()
{
    if (!d_matrix)
        return;

    d_matrix->setNumericFormat(d_old_format, d_old_prec);
}

/*************************************************************************/
/*           Class MatrixSetCoordinatesCommand                           */
/*************************************************************************/
MatrixSetCoordinatesCommand::MatrixSetCoordinatesCommand(Matrix *m, double oxs, double oxe,
    double oys, double oye, double nxs, double nxe, double nys, double nye, const QString & text):
QUndoCommand(text),
d_matrix(m),
d_old_xs(oxs),
d_old_xe(oxe),
d_old_ys(oys),
d_old_ye(oye),
d_new_xs(nxs),
d_new_xe(nxe),
d_new_ys(nys),
d_new_ye(nye)
{
    setText(m->objectName() + ": " + text);
}

void MatrixSetCoordinatesCommand::redo()
{
    if (!d_matrix)
        return;

    d_matrix->setCoordinates(d_new_xs, d_new_xe, d_new_ys, d_new_ye);
}

void MatrixSetCoordinatesCommand::undo()
{
    if (!d_matrix)
        return;

    d_matrix->setCoordinates(d_old_xs, d_old_xe, d_old_ys, d_old_ye);
}

/*************************************************************************/
/*           Class MatrixSetColorMapCommand                              */
/*************************************************************************/
MatrixSetColorMapCommand::MatrixSetColorMapCommand(Matrix *m, Matrix::ColorMapType type_before,
							const QwtLinearColorMap& map_before, Matrix::ColorMapType type_after,
							const QwtLinearColorMap& map_after, const QString & text):
QUndoCommand(text),
d_matrix(m),
d_map_type_before(type_before),
d_map_type_after(type_after)
{
    setText(m->objectName() + ": " + text);

	d_map_before = QwtLinearColorMap(map_before);
	d_map_after = QwtLinearColorMap(map_after);
}

void MatrixSetColorMapCommand::redo()
{
    if (!d_matrix)
        return;

	switch(d_map_type_after){
		case Matrix::GrayScale:
			d_matrix->setGrayScale();
		break;

		case Matrix::Rainbow:
			d_matrix->setRainbowColorMap();
		break;

		case Matrix::Custom:
			d_matrix->setColorMap(d_map_after);
		break;
	}
}

void MatrixSetColorMapCommand::undo()
{
    if (!d_matrix)
        return;

    	switch(d_map_type_before){
		case Matrix::GrayScale:
			d_matrix->setGrayScale();
		break;

		case Matrix::Rainbow:
			d_matrix->setRainbowColorMap();
		break;

		case Matrix::Custom:
			d_matrix->setColorMap(d_map_before);
		break;
	}
}

/*************************************************************************/
/*           Class MatrixDeleteRowsCommand                               */
/*************************************************************************/
MatrixDeleteRowsCommand::MatrixDeleteRowsCommand(MatrixModel *model, int startRow, int count, double* data, const QString& text):
QUndoCommand(text),
d_model(model),
d_start_row(startRow),
d_count(count),
d_data(data)
{
    setText(model->matrix()->objectName() + ": " + text);
}

void MatrixDeleteRowsCommand::redo()
{
    if (!d_model)
        return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    d_model->removeRows(d_start_row, d_count);
	QApplication::restoreOverrideCursor();
}

void MatrixDeleteRowsCommand::undo()
{
    if (!d_model)
        return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    d_model->insertRows(d_start_row, d_count);
	double *data = d_model->dataVector();
	int cols = d_model->columnCount();
	int size = cols * d_count;
	int cell = d_start_row*cols;
	for (int i = 0; i<size; i++)
		data[cell++] = d_data[i];

	QApplication::restoreOverrideCursor();
}

/*************************************************************************/
/*           Class MatrixInsertRowCommand                                */
/*************************************************************************/
MatrixInsertRowCommand::MatrixInsertRowCommand(MatrixModel *model, int startRow, const QString& text):
QUndoCommand(text),
d_model(model),
d_start_row(startRow)
{
    setText(model->matrix()->objectName() + ": " + text);
}

void MatrixInsertRowCommand::redo()
{
    if (!d_model)
        return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    d_model->insertRows(d_start_row, 1);
	QApplication::restoreOverrideCursor();
}

void MatrixInsertRowCommand::undo()
{
    if (!d_model)
        return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    d_model->removeRows(d_start_row, 1);
	QApplication::restoreOverrideCursor();
}

/*************************************************************************/
/*           Class MatrixDeleteColsCommand                               */
/*************************************************************************/
MatrixDeleteColsCommand::MatrixDeleteColsCommand(MatrixModel *model, int startCol, int count, double* data, const QString& text):
QUndoCommand(text),
d_model(model),
d_start_col(startCol),
d_count(count),
d_data(data)
{
    setText(model->matrix()->objectName() + ": " + text);
}

void MatrixDeleteColsCommand::redo()
{
    if (!d_model)
        return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    d_model->removeColumns(d_start_col, d_count);
	QApplication::restoreOverrideCursor();
}

void MatrixDeleteColsCommand::undo()
{
    if (!d_model)
        return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    d_model->insertColumns(d_start_col, d_count);
	double *data = d_model->dataVector();
	int rows = d_model->rowCount();
	int cols = d_model->columnCount();
	for (int i = 0; i<rows; i++){
		int aux = i*cols + d_start_col;
		int aux2 = i*d_count;
		for (int j = 0; j<d_count; j++)
			data[aux++] = d_data[aux2++];
	}
	QApplication::restoreOverrideCursor();
}

/*************************************************************************/
/*           Class MatrixInsertColCommand                                */
/*************************************************************************/
MatrixInsertColCommand::MatrixInsertColCommand(MatrixModel *model, int startCol, const QString& text):
QUndoCommand(text),
d_model(model),
d_start_col(startCol)
{
    setText(model->matrix()->objectName() + ": " + text);
}

void MatrixInsertColCommand::redo()
{
    if (!d_model)
        return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    d_model->insertColumns(d_start_col, 1);
	QApplication::restoreOverrideCursor();
}

void MatrixInsertColCommand::undo()
{
    if (!d_model)
        return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    d_model->removeColumns(d_start_col, 1);
	QApplication::restoreOverrideCursor();
}

/*************************************************************************/
/*           Class MatrixSetSizeCommand                                */
/*************************************************************************/
MatrixSetSizeCommand::MatrixSetSizeCommand(MatrixModel *model, const QSize& oldSize, const QSize& newSize, double *data, const QString& text):
QUndoCommand(text),
d_model(model),
d_old_size(oldSize),
d_new_size(newSize),
d_backup(data)
{
    setText(model->matrix()->objectName() + ": " + text);
}

void MatrixSetSizeCommand::redo()
{
    if (!d_model)
        return;

    d_model->setDimensions(d_new_size.width(), d_new_size.height());
	d_model->matrix()->resetView();
}

void MatrixSetSizeCommand::undo()
{
    if (!d_model)
        return;
	
	int rows = d_old_size.width();
	int cols = d_old_size.height();
    d_model->setDimensions(rows, cols);
	
	double *data = d_model->dataVector();
    if (!data)
        return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	int cell = 0;
    for (int i = 0; i < rows; i++){
        for (int j = 0; j < cols; j++){
            data[cell] = d_backup[cell];
			cell++;
		}
    }
	d_model->matrix()->resetView();
	QApplication::restoreOverrideCursor();
}

/*************************************************************************/
/*           Class MatrixUndoCommand                           */
/*************************************************************************/
MatrixUndoCommand::MatrixUndoCommand(MatrixModel *model, Matrix::Operation op, int startRow, int endRow, int startCol, int endCol,
									double *data, const QString& text):
QUndoCommand(text),
d_model(model),
d_operation(op),
d_start_row(startRow),
d_end_row(endRow),
d_start_col(startCol),
d_end_col(endCol),
d_data(data)
{
    setText(model->matrix()->objectName() + ": " + text);
}

void MatrixUndoCommand::redo()
{
    if (!d_model)
        return;

	switch(d_operation){
		case Matrix::Clear:
			d_model->clear(d_start_row, d_end_row, d_start_col, d_end_col);
		break;
		case Matrix::Calculate:
			d_model->calculate(d_start_row, d_end_row, d_start_col, d_end_col);
		break;
		case Matrix::MuParserCalculate:
			d_model->muParserCalculate(d_start_row, d_end_row, d_start_col, d_end_col);
		break;
		default:
		break;
	}
    d_model->matrix()->resetView();
}

void MatrixUndoCommand::undo()
{
    if (!d_model)
        return;

    double *data = d_model->dataVector();
    if (!data)
        return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    int cols = d_model->columnCount();
	int aux = 0;
    for (int i = d_start_row; i <= d_end_row; i++){
        int row = i*cols + d_start_col;
        for (int j = d_start_col; j <= d_end_col; j++)
            data[row++] = d_data[aux++];
    }
    d_model->matrix()->resetView();
	QApplication::restoreOverrideCursor();
}

/*************************************************************************/
/*           Class MatrixFftCommand                                      */
/*************************************************************************/
MatrixFftCommand::MatrixFftCommand(bool inverse, MatrixModel *model, int startRow, int endRow,
									int startCol, int endCol, double *data, const QString& text):
MatrixUndoCommand(model, Matrix::FFT, startRow, endRow, startCol, endCol, data, text),
d_inverse(inverse)
{
}

void MatrixFftCommand::redo()
{
    if (!d_model)
        return;

    d_model->fft(d_inverse);
}

/*************************************************************************/
/*           Class MatrixSetImageCommand                           */
/*************************************************************************/
MatrixSetImageCommand::MatrixSetImageCommand(MatrixModel *model, const QImage& image, Matrix::ViewType oldView,
						int startRow, int endRow, int startCol, int endCol, double *data, const QString& text):
MatrixUndoCommand(model, Matrix::SetImage, startRow, endRow, startCol, endCol, data, text),
d_image(image),
d_old_view(oldView)
{
}

void MatrixSetImageCommand::undo()
{
    if (!d_model)
        return;

    d_model->setDimensions(d_end_row - d_start_row + 1, d_end_col - d_start_col + 1);
	d_model->matrix()->setViewType(d_old_view);
	MatrixUndoCommand::undo();
}

void MatrixSetImageCommand::redo()
{
    if (!d_model)
        return;

    d_model->setImage(d_image);
    Matrix *m = d_model->matrix();
	m->setViewType(Matrix::ImageView, false);
	m->displayImage(d_image);
}

/*************************************************************************/
/*           Class MatrixImportAsciiCommand                              */
/*************************************************************************/
MatrixImportAsciiCommand::MatrixImportAsciiCommand(const QString &fname, const QString &sep,
						int ignoredLines, bool stripSpaces, bool simplifySpaces,
						const QString& commentString, Matrix::ImportMode importAs, const QLocale& locale,
						int endLineChar, int maxRows, MatrixModel *model, int startRow, int endRow,
						int startCol, int endCol, double *data, const QString& text):
MatrixUndoCommand(model, Matrix::ImportAscii, startRow, endRow, startCol, endCol, data, text),
d_path(fname),
d_sep(sep),
d_comment(commentString),
d_ignore_lines(ignoredLines),
d_end_line(endLineChar),
d_max_rows(maxRows),
d_strip_spaces(stripSpaces),
d_simplify_spaces(simplifySpaces),
d_mode(importAs),
d_locale(locale)
{
}

void MatrixImportAsciiCommand::redo()
{
    if (!d_model)
        return;

	d_model->importASCII(d_path, d_sep, d_ignore_lines, d_strip_spaces, d_simplify_spaces,
						d_comment, d_mode, d_locale, d_end_line, d_max_rows);
}

/*************************************************************************/
/*           Class MatrixSymmetryOperation                                */
/*************************************************************************/
MatrixSymmetryOperation::MatrixSymmetryOperation(MatrixModel *model, Matrix::Operation op, const QString& text):
QUndoCommand(text),
d_model(model),
d_operation(op)
{
    setText(model->matrix()->objectName() + ": " + text);
}

void MatrixSymmetryOperation::redo()
{
    if (!d_model)
        return;

    switch(d_operation){
		case Matrix::Transpose:
			d_model->transpose();
		break;
		case Matrix::Invert:
			d_model->invert();
		break;
		case Matrix::FlipVertically:
			d_model->flipVertically();
		break;
		case Matrix::FlipHorizontally:
			d_model->flipHorizontally();
		break;
		case Matrix::RotateClockwise:
			d_model->rotate90(true);
		break;
		case Matrix::RotateCounterClockwise:
			d_model->rotate90(false);
		break;
		default:
		break;
	}
	d_model->matrix()->resetView();
}

void MatrixSymmetryOperation::undo()
{
    if (!d_model)
        return;

    switch(d_operation){
		case Matrix::Transpose:
			d_model->transpose();
		break;
		case Matrix::Invert:
			d_model->invert();
		break;
		case Matrix::FlipVertically:
			d_model->flipVertically();
		break;
		case Matrix::FlipHorizontally:
			d_model->flipHorizontally();
		break;
		case Matrix::RotateClockwise:
			d_model->rotate90(false);
		break;
		case Matrix::RotateCounterClockwise:
			d_model->rotate90(true);
		break;
		default:
		break;
	}
	d_model->matrix()->resetView();
}

/*************************************************************************/
/*           Class MatrixPasteCommand                               	 */
/*************************************************************************/
MatrixPasteCommand::MatrixPasteCommand(MatrixModel *model, int startRow, int endRow, int startCol, int endCol,
					double *clipboardData, int rows, int cols, double *backupData, int oldRows, int oldCols,
					const QString& text):
QUndoCommand(text),
d_model(model),
d_start_row(startRow),
d_end_row(endRow),
d_start_col(startCol),
d_end_col(endCol),
d_rows(rows),
d_cols(cols),
d_old_rows(oldRows),
d_old_cols(oldCols),
d_clipboard_data(clipboardData),
d_backup_data(backupData)
{
    setText(model->matrix()->objectName() + ": " + text);
}

void MatrixPasteCommand::redo()
{
    if (!d_model)
        return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));	
    d_model->pasteData(d_clipboard_data, d_start_row, d_start_col, d_rows, d_cols);
	d_model->matrix()->resetView();
	QApplication::restoreOverrideCursor();
}

void MatrixPasteCommand::undo()
{
	if (!d_model)
        return;

 	double *data = d_model->dataVector();
    if (!data)
        return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	if (d_old_rows != d_model->rowCount())
		d_model->setRowCount(d_old_rows);
	if (d_old_cols != d_model->columnCount())
		d_model->setColumnCount(d_old_cols);

    int cols = d_model->columnCount();
	int aux = 0;
    for (int i = d_start_row; i <= d_end_row; i++){
        int row = i*cols + d_start_col;
        for (int j = d_start_col; j <= d_end_col; j++)
            data[row++] = d_backup_data[aux++];
    }
    d_model->matrix()->resetView();
	QApplication::restoreOverrideCursor();
}
