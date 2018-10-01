
/***************************************************************************
    File                 : Table.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief,
                           Tilman Hoener zu Siederdissen,
                           Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Table worksheet class

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
#ifndef TABLE_H
#define TABLE_H

#include <QTableWidget>
#include <QVarLengthArray>

#include "Graph.h"
#include "MdiSubWindow.h"
#include "Scripted.h"
#include "ScriptingEnv.h"

#include "MantidQtWidgets/Common/IProjectSerialisable.h"

class QTableWidgetItem;

class MyTable : public QTableWidget {
  Q_OBJECT
public:
  MyTable(QWidget *parent = nullptr, const char *name = nullptr);
  MyTable(int numRows, int numCols, QWidget *parent = nullptr,
          const char *name = nullptr);
  void blockResizing(bool yes);
  QString text(int row, int col) const;
  void setText(int row, int col, const QString &txt);
  bool isColumnReadOnly(int col) const;
  void setColumnReadOnly(int col, bool on);
  void insertColumns(int col, int count = 1);
  void insertRows(int row, int count = 1);
  void removeRows(const QVector<int> &rows);
  bool isColumnSelected(int col, bool full = false) const;
  bool isRowSelected(int row, bool full = false) const;
  bool isSelected(int row, int col) const;
  bool hasSelection() const;
  int topSelectedRow() const;
  int bottomSelectedRow() const;
  int leftSelectedColumn() const;
  int rightSelectedColumn() const;
  void selectCell(int row, int col);
  void ensureCellVisible(int row, int col);
  void swapColumns(int col1, int col2);

signals:
  void unwantedResize();

private:
  void resizeData(int n);
  QTableWidgetItem *addNewItem(int row, int col);
  void makeItemPrototype();
  bool m_blockResizing; // a workaround to prevent unwanted resizes
  QTableWidgetItem *m_itemPrototype;
};

/**\brief MDI window providing a spreadsheet table with column logic.
 *
 * \section future Future Plans
 * Port to the Model/View approach used in Qt4 and get rid of the Qt3Support
 * dependency.
 * [ assigned to thzs ]
 */
class Table : public MdiSubWindow, public Scripted {
  Q_OBJECT

public:
  enum PlotDesignation {
    All = -1,
    None = 0,
    X = 1,
    Y = 2,
    Z = 3,
    xErr = 4,
    yErr = 5,
    Label = 6
  };
  enum ColType {
    Numeric = 0,
    Text = 1,
    Date = 2,
    Time = 3,
    Month = 4,
    Day = 5
  };
  enum ImportMode {
    NewColumns, //!< add file as new columns to this table
    NewRows,    //!< add file as new rows to this table
    Overwrite   //!< replace content of table with the imported file
  };

  Table(ScriptingEnv *env, int r, int c, const QString &label, QWidget *parent,
        const QString &name = QString(), Qt::WFlags f = nullptr);

  int topSelectedRow() const { return d_table->topSelectedRow(); }
  int bottomSelectedRow() const { return d_table->bottomSelectedRow(); }
  int leftSelectedColumn() const { return d_table->leftSelectedColumn(); }
  int rightSelectedColumn() const { return d_table->rightSelectedColumn(); }
  bool hasSelection() const { return d_table->hasSelection(); }

  //! Sets the number of significant digits
  void setNumericPrecision(int prec);
  //! Updates the decimal separators in the numerical columns on user request
  void updateDecimalSeparators();
  //! Updates the decimal separators when importing ASCII files on user request
  void updateDecimalSeparators(const QLocale &oldSeparators);
  void setAutoUpdateValues(bool on = true);

public slots:
  MyTable *table() { return d_table; };
  void copy(Table *m);
  int numRows();
  int numCols();
  void setNumRows(int rows);
  void setNumCols(int cols);
  void resizeRows(int);
  void resizeCols(int);

  //! Return the value of the cell as a double
  double cell(int row, int col);
  void setCell(int row, int col, double val);

  QString text(int row, int col);
  QStringList columnsList();
  QStringList colNames() { return col_label; }
  QString colName(int col);
  void setColName(int col, const QString &text, bool enumerateRight = false);
  QString colLabel(int col) { return col_label[col]; };
  int colIndex(const QString &name);

  int colPlotDesignation(int col) { return col_plot_type[col]; };
  void setColPlotDesignation(int col, PlotDesignation pd);
  virtual void setPlotDesignation(PlotDesignation pd,
                                  bool rightColumns = false);
  QList<int> plotDesignations() { return col_plot_type; };

  void setHeader(QStringList header);
  void loadHeader(QStringList header);
  void setHeaderColType();
  void setText(int row, int col, const QString &text);
  void setRandomValues();
  void setAscValues();
  void setTextAlignment(int row, int col, QFlags<Qt::AlignmentFlag> alignment);

  virtual void cellEdited(int, int col);
  void moveCurrentCell();
  bool isEmptyRow(int row);
  bool isEmptyColumn(int col);
  void onColumnHeaderDoubleClick();

  void print() override;
  void print(const QString &fileName);
  void exportPDF(const QString &fileName) override;

  //! \name Event Handlers
  //@{
  bool eventFilter(QObject *object, QEvent *e) override;
  void customEvent(QEvent *e) override;
  //@}v

  //! \name Column Operations
  //@{
  void removeCol();
  void removeCol(const QStringList &list);
  void insertCol();
  void insertCols(int start, int count);
  void addCol(PlotDesignation pd = Y);
  void addColumns(int c);
  void moveColumn(int, int, int);
  void swapColumns(int, int);
  void moveColumnBy(int cols);
  void hideSelectedColumns();
  void showAllColumns();
  void hideColumn(int col, bool = true);
  bool isColumnHidden(int col) { return d_table->isColumnHidden(col); };
  void resizeColumnsToContents();
  //@}

  //! \name Sorting
  //@{
  /**\brief Sort the current column in ascending order.
   * \sa sortColDesc(), sortColumn()
   */
  void sortColAsc();
  /**\brief Sort the current column in descending order.
   * \sa sortColAsc(), sortColumn()
   */
  void sortColDesc();

  /**\brief Sort the specified column.
   * @param col :: the column to be sorted
   * @param order :: 0 means ascending, anything else means descending
   */
  virtual void sortColumn(int col = -1, int order = 0);

  /**\brief Sort the specified columns.
   * @param cols :: the columns to be sorted
   * @param type :: 0 means sort individually (as in sortColumn()), anything
   * else means together
   * @param order :: 0 means ascending, anything else means descending
   * @param leadCol :: for sorting together, the column which determines the
   * permutation
   */
  virtual void sortColumns(const QStringList &cols, int type = 0, int order = 0,
                           const QString &leadCol = QString());

  /**\brief Display a dialog with some options for sorting all columns.
   *
   * The sorting itself is done using sort(int,int,const QString&).
   */
  virtual void sortTableDialog();
  //! Sort all columns as in sortColumns(const QStringList&,int,int,const
  //! QString&).
  void sort(int type = 0, int order = 0, const QString &leadCol = QString());
  //! Sort selected columns as in sortColumns(const QStringList&,int,int,const
  //! QString&).
  void sortColumns(int type = 0, int order = 0,
                   const QString &leadCol = QString());

  /**\brief Display a dialog with some options for sorting the selected columns.
   *
   * The sorting itself is done using sortColumns(int,int,const QString&).
   */
  void sortColumnsDialog();
  //@}

  //! \name Normalization
  //@{
  void normalizeCol(int col = -1);
  void normalizeSelection();
  void normalize();
  //@}

  QVarLengthArray<double> col(int ycol);
  int firstXCol();
  bool noXColumn();
  bool noYColumn();
  int colX(int col);
  int colY(int col);

  QStringList getCommands() { return commands; };
  //! Set all column formulae.
  void setCommands(const QStringList &com);
  //! Set all column formulae.
  void setCommands(const QString &com);
  //! Set formula for column col.
  void setCommand(int col, const QString &com);
  //! Compute specified cells from column formula.
  bool calculate(int col, int startRow, int endRow, bool forceMuParser = true,
                 bool notifyChanges = true);
  //! Compute specified cells from column formula (optimized for muParser).
  bool muParserCalculate(int col, int startRow, int endRow,
                         bool notifyChanges = true);
  //! Compute selected cells from column formulae; use current cell if there's
  //! no selection.
  bool calculate();
  //! Recalculates values in all columns with formulas containing @param
  //! columnName
  void updateValues(Table *, const QString &columnName);

  //! \name Row Operations
  //@{
  void deleteSelectedRows();
  virtual void deleteRows(int startRow, int endRow);
  void insertRow();
  void insertRow(int row); // Mantid
  void addRows(int num);   // Mantid
  virtual void insertRows(int atRow, int num);
  //@}

  //! Selection Operations
  //@{
  void cutSelection();
  void copySelection();
  void clearSelection();
  void pasteSelection();
  void selectAllTable();
  void deselect();
  void clear();
  //@}

  QStringList selectedColumns();
  QStringList selectedYColumns();
  QStringList selectedXColumns();
  QStringList selectedYLabels();
  QStringList drawableColumnSelection();
  QStringList YColumns();
  int selectedColsNumber();

  void setColumnWidth(int width, bool allCols);
  void setColumnWidth(int col, int width);
  int columnWidth(int col);
  QStringList columnWidths();
  void setColWidths(const QStringList &widths);

  void setSelectedCol(int col) { selectedCol = col; };
  int selectedColumn() { return selectedCol; };
  int firstSelectedColumn();
  int numSelectedRows();
  bool isRowSelected(int row, bool full = false) {
    return d_table->isRowSelected(row, full);
  }
  bool isColumnSelected(int col, bool full = false) {
    return d_table->isColumnSelected(col, full);
  }
  //! Scroll to row (row starts with 1)
  void goToRow(int row);
  //! Scroll to column (column starts with 1)
  void goToColumn(int col);

  void columnNumericFormat(int col, char *f, int *precision);
  void columnNumericFormat(int col, int *f, int *precision);
  int columnType(int col) { return colTypes[col]; };

  QList<int> columnTypes() { return colTypes; };
  void setColumnTypes(QList<int> ctl) { colTypes = ctl; };
  void setColumnTypes(const QStringList &ctl);
  void setColumnType(int col, ColType val) { colTypes[col] = val; }

  void saveToMemory(double **cells) { d_saved_cells = cells; };
  void saveToMemory();
  void freeMemory();

  bool isReadOnlyColumn(int col);
  void setReadOnlyColumn(int col, bool on = true);
  void setReadOnlyAllColumns(bool on = true);

  QString columnFormat(int col) { return col_format[col]; };
  QStringList getColumnsFormat() { return col_format; };

  void setTextFormat(int col);
  void setColNumericFormat(int f, int prec, int col, bool updateCells = true);
  bool setDateFormat(const QString &format, int col, bool updateCells = true);
  bool setTimeFormat(const QString &format, int col, bool updateCells = true);
  void setMonthFormat(const QString &format, int col, bool updateCells = true);
  void setDayFormat(const QString &format, int col, bool updateCells = true);

  bool exportASCII(const QString &fname, const QString &separator,
                   bool withLabels = false, bool exportComments = false,
                   bool exportSelection = false);
  void importASCII(const QString &fname, const QString &sep, int ignoredLines,
                   bool renameCols, bool stripSpaces, bool simplifySpaces,
                   bool importComments, const QString &commentString,
                   bool readOnly = false, ImportMode importAs = Overwrite,
                   int endLine = 0, int maxRows = -1);

  //! \name Saving and Restoring
  //@{
  std::string saveToProject(ApplicationWindow *app) override;
  std::string saveTableMetadata();

  void restore(QString &spec);

  //! This changes the general background color (color of the table widget, not
  //! the cells)
  void setBackgroundColor(const QColor &col);
  void setTextColor(const QColor &col);
  void setHeaderColor(const QColor &col);
  void setTextFont(const QFont &fnt);
  const QFont &getTextFont();
  void setHeaderFont(const QFont &fnt);

  int verticalHeaderWidth();

  QString colComment(int col) { return comments[col]; };
  void setColComment(int col, const QString &s);
  QStringList colComments() { return comments; };
  void setColComments(const QStringList &lst) { comments = lst; };
  void showComments(bool on = true);
  bool commentsEnabled() { return d_show_comments; }

  static MantidQt::API::IProjectSerialisable *
  loadFromProject(const std::string &lines, ApplicationWindow *app,
                  const int fileVersion);
  /// Returns a list of workspace names that are used by this window
  std::vector<std::string> getWorkspaceNames() override;

  void restore(const QStringList &lst) override;

  //! This slot notifies the main application that the table has been modified.
  //! Triggers the update of 2D plots.
  void notifyChanges();

  //! Notifies the main application that the width of a table column has been
  //! modified by the user.
  void colWidthModified(int, int, int);

  //! is this table editable
  virtual bool isEditable() { return true; }
  //! is this table sortable
  virtual bool isSortable() { return true; }
  //! are the columns fixed - not editable by the GUI
  virtual bool isFixedColumns() { return false; }

signals:
  void changedColHeader(const QString &, const QString &);
  void removedCol(const QString &);
  void modifiedData(Table *, const QString &);
  void optionsDialog();
  void colValuesDialog();
  void resizedTable(QWidget *);
  void showContextMenu(bool selection);
  void createTable(const QString &, int, int, const QString &);

protected:
  void setColPlotDesignation(int col, int pd);

  MyTable *d_table;

private slots:

  void recordSelection();

private:
  void clearCol();

  bool d_show_comments;
  QString specifications, newSpecifications;
  QStringList commands, col_format, comments, col_label;
  QList<int> colTypes, col_plot_type;
  int selectedCol;
  int d_numeric_precision;
  double **d_saved_cells;

  //! Internal function to change the column header
  void setColumnHeader(int index, const QString &label);
};

#endif
