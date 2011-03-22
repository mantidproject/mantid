/***************************************************************************
    File                 : ImportASCIIDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006,2007 by Ion Vasilief, Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, knut.franke*gmx.de
    Description          : Import ASCII file(s) dialog

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
#ifndef IMPORTFILESDIALOG_H
#define IMPORTFILESDIALOG_H

#include "ExtensibleFileDialog.h"

#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QTableView>

class QGroupBox;
class QPushButton;
class QStackedWidget;
class Matrix;
class MatrixModel;

#include <q3table.h>
#include <q3header.h>

class PreviewTable : public Q3Table
{
public:
    PreviewTable(int numRows, int numCols, QWidget * parent = 0, const char * name = 0);

	void importASCII(const QString &fname, const QString &sep, int ignoredLines, bool renameCols,
		bool stripSpaces, bool simplifySpaces, bool importComments, const QString& commentString,
		int importMode, int endLine, int maxRows);

	void resetHeader();
	void clear();
	void updateDecimalSeparators(const QLocale& oldSeparators);
	void setNumericPrecision(int prec) {d_numeric_precision = prec;};

private:
	void setHeader();
	void addColumns(int c);
	QStringList comments, col_label;
	int d_numeric_precision;
};

class PreviewMatrix : public QTableView
{
public:
    PreviewMatrix(QWidget *parent, Matrix * m = 0);

	void importASCII(const QString &fname, const QString &sep, int ignoredLines,
		bool stripSpaces, bool simplifySpaces, const QString& commentString,
		int importAs, const QLocale& locale, int endLine, int maxRows);

	void clear();

private:
	 MatrixModel *d_matrix_model;
};

//! Import ASCII file(s) dialog
class ImportASCIIDialog: public ExtensibleFileDialog
{
	Q_OBJECT

public:
	//! Possible destinations for the imported data.
	/**
	 * Important: Keep this in sync with the initialization of #d_import_mode in initAdvancedOptions().
	 */
	enum ImportMode {
		NewTables, //!< create a new table for each file (default)
		NewMatrices, //!< create a new matrix for each file
		NewWorkspace, //!< create a new workspace  for each file 
		NewColumns, //!< add each file as new columns to the current table
		NewRows, //!< add each file as new rows to the current table
		Overwrite //!< replace content of current table with the selected file (like importing a single file in previous versions of QtiPlot)
		
	};

	//! Constructor
	/**
	 * @param importModeEnabled :: flag: enable/disable import mode combo box
	 * @param parent :: parent widget (only affects placement of the dialog)
	 * @param extended :: flag: show/hide the advanced options on start-up
	 * @param flags :: window flags
	 */
	ImportASCIIDialog(bool new_windows_only, QWidget * parent = 0, bool extended = true, Qt::WFlags flags = 0 );

	//! Return the selected import mode
	/**
	 * \sa ImportMode
	 */
	ImportMode importMode() const { return (ImportMode) d_import_mode->currentIndex(); }
	//! Return the selected column separator.
	const QString columnSeparator() const;
	//! Return the number of lines to be skipped at the start of each file.
	int ignoredLines() const { return d_ignored_lines->value(); }
	//! Whether to rename columns based on the first (non-skipped) line.
	bool renameColumns() const { return d_rename_columns->isChecked(); }
	//! Whether to replace sequences of whitespace charecters with a single space.
	bool simplifySpaces() const { return d_simplify_spaces->isChecked(); }
	//! Whether to remove whitespace from beginning and end of lines.
	bool stripSpaces() const { return d_strip_spaces->isChecked(); }
	//! Whether the user wants the import options to be saved.
	bool readOnly() const {return d_read_only->isChecked(); }

	//! Set the selected columns separator.
	void setColumnSeparator(const QString &sep);

	//! Returns a locale having the decimal separators set to user custom settings.
	QLocale decimalSeparators();
	//! Whether the user wants the decimal separators to be changed to application settings.
	bool updateDecimalSeparators() const { return d_import_dec_separators->isChecked(); };

    //! Returns a string used to comment lines when importing ASCII files
	QString commentString(){return d_comment_string->text();};

    //! Returns true if the second line of the ASCII file should be used to set comments in table
    bool importComments(){return d_import_comments->isChecked();};
	
	//! Returns the convention used for the end line character!
	inline int endLineChar(){return boxEndLine->currentIndex();};
	///
	QString getselectedColumnSeparator();
		
private slots:
	//! Display help for advanced options.
	void displayHelp();
	//! For #Overwrite mode, allow only one file to be selected.
	void updateImportMode(int mode);
	void preview();
	void changePreviewFile(const QString& path);
	//! Enable/Disable options which are only available for tables.
	void enableTableOptions(bool on);

private:
	void initPreview(int previewMode);
	void previewTable();
	void previewMatrix();

	void closeEvent(QCloseEvent*);
	//! Initialise #d_advanced_options and everything it contains.
	void initAdvancedOptions();
	void setNewWindowsOnly(bool on);
	void addColumnSeparators();
	void addColumnSeparatorsforLoadAscii();
	

	//! Container widget for all advanced options.
	QGroupBox *d_advanced_options;
	QCheckBox *d_read_only, *d_import_dec_separators;
	QPushButton *d_help_button;
	// the actual options
	QComboBox *d_import_mode, *d_column_separator, *boxDecimalSeparator, *boxEndLine;
	QSpinBox *d_ignored_lines, *d_preview_lines_box;
	QCheckBox *d_rename_columns, *d_simplify_spaces, *d_strip_spaces, *d_import_comments;
	QLineEdit *d_comment_string;
	PreviewTable *d_preview_table;
	PreviewMatrix *d_preview_matrix;
	QCheckBox *d_preview_button;
	QStackedWidget *d_preview_stack;
	QString d_current_path;
};

#endif
