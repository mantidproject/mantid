#ifndef MANTIDMATRIX_H
#define MANTIDMATRIX_H

#include <QHeaderView>
#include <QTableView>
#include <QPrinter>
#include <QMessageBox>
#include <QAction>
#include <QVector>
#include <QThread>
#include <QMap>

#include <Poco/NObserver.h>

#include "MantidAPI/Workspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "../UserFunction.h"
#include "../MdiSubWindow.h"
#include "../Graph.h"

#include <qwt_double_rect.h>
#include <qwt_color_map.h>

#include <math.h>
#include <string>
#include <iostream>

class QLabel;
class QStackedWidget;
class QShortcut;
class MantidMatrixModel;
class MantidMatrix;
class ApplicationWindow;
class Graph3D;
class MultiLayer;
class QTabWidget;
class UpdateDAEThread;

using namespace Mantid::API;

class MantidMatrixFunction: public UserHelperFunction
{
public:
    MantidMatrixFunction(MantidMatrix* wsm):m_matrix(wsm){}
    double operator()(double x, double y);
    void init();
private:
    MantidMatrix* m_matrix;
    double m_dx,m_dy;
};
/** MantidMatrix is the class that represents a Qtiplot window for displaying workspaces.
    It has separate tabs for displaying spectrum values, bin boundaries, and errors.

    @author Roman Tolchenov, Tessella Support Services plc

    Copyright &copy; 2007 STFC Rutherford Appleton Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>

*/
class MantidMatrix: public MdiSubWindow
{
    Q_OBJECT


public:

	MantidMatrix(Mantid::API::Workspace_sptr ws, ApplicationWindow* parent, const QString& label, const QString& name = QString(), int start=-1, int end=-1);
    ~MantidMatrix();

    void connectTableView(QTableView*,MantidMatrixModel*);
	MantidMatrixModel * model(){return m_modelY;};
	MantidMatrixModel * modelY(){return m_modelY;};
	MantidMatrixModel * modelX(){return m_modelX;};
	MantidMatrixModel * modelE(){return m_modelE;};
	QItemSelectionModel * selectionModel(){return m_table_viewY->selectionModel();};
	QItemSelectionModel * selectionModelY(){return m_table_viewY->selectionModel();};
	QItemSelectionModel * selectionModelX(){return m_table_viewX->selectionModel();};
	QItemSelectionModel * selectionModelE(){return m_table_viewE->selectionModel();};

    int numRows()const{return m_rows;}
    int numCols()const{return m_cols;}
	double dataX(int row, int col) const;
	double dataY(int row, int col) const;
	double dataE(int row, int col) const;
    int indexX(double s)const;

    Mantid::API::Workspace_sptr workspace(){return m_workspace;}

    const char **matrixIcon(){return m_matrix_icon;}
    //void copy(Matrix *m);
    ApplicationWindow *appWindow(){return m_appWindow;}
    Graph3D *plotGraph3D(int style);
    MultiLayer* plotGraph2D(Graph::CurveType type);
    void setGraph1D(MultiLayer* ml, Table* t=0);
    void removeWindow();
    void getSelectedRows(int& i0,int& i1);
    void getSelectedColumns(int& i0,int& i1);

    int workspaceIndex(int row){return row + m_startRow;}
    bool yShown(){return m_tabs->currentIndex() == 0;}
    QTableView *activeView();
    MantidMatrixModel *activeModel();

    bool isHistogram(){return m_histogram;}

signals:
    void needsUpdating();
    void needChangeWorkspace(Mantid::API::Workspace_sptr ws);
    void needDeleteWorkspace();

public slots:

    void changeWorkspace(Mantid::API::Workspace_sptr ws);
    void deleteWorkspace();
    void tst();

	//! Return the width of all columns
	int columnsWidth(){return m_column_width;};
	//! Set the width of all columns
	void setColumnsWidth(int width);

	//! Return the content of the cell as a string
	QString text(int row, int col);
	//! Return the value of the cell as a double
	double cell(int row, int col);

    //! Returns the X value corresponding to column 1
	double xStart(){return x_start;};
	//! Returns the X value corresponding to the last column
	double xEnd(){return x_end;};
	//! Returns the Y value corresponding to row 1
	double yStart(){return y_start;};
	//! Returns the Y value corresponding to the last row
	double yEnd(){return y_end;};

	//! Returns the step of the X axis
	double dx(){return fabs(x_end - x_start)/(double)(numCols() - 1);};
	//! Returns the step of the Y axis
	double dy(){return fabs(y_end - y_start)/(double)(numRows() - 1);};

	//! Returns the bounding rect of the matrix coordinates
  	QwtDoubleRect boundingRect();
	//! Set the X and Y coordinate intervals

	 //! Min and max values of the matrix.
  	void range(double *min, double *max);
	 //! Set min and max values of the matrix.
  	void setRange(double min, double max);

    void goTo(int row,int col);
    //! Scroll to row (row starts with 1)
	void goToRow(int row);
	//! Scroll to column (column starts with 1)
	void goToColumn(int col);
    void copySelection();

	//! Allocate memory for a matrix buffer
	static double** allocateMatrixData(int rows, int columns);
	//! Free memory used for a matrix buffer
	static void freeMatrixData(double **data, int rows);

	int verticalHeaderWidth(){return m_table_viewY->verticalHeader()->width();}

    void dependantClosed(MdiSubWindow* w);
    void selfClosed(MdiSubWindow* w);
    void repaintAll();
    void closeDependants();

protected:

    void setup(Mantid::API::Workspace_sptr ws, int start=-1, int end=-1);

    void handleReplaceWorkspace(WorkspaceBeforeReplaceNotification_ptr pNf);
    Poco::NObserver<MantidMatrix, WorkspaceBeforeReplaceNotification> m_replaceObserver;

    void handleDeleteWorkspace(WorkspaceDeleteNotification_ptr pNf);
    Poco::NObserver<MantidMatrix, WorkspaceDeleteNotification> m_deleteObserver;

    ApplicationWindow *m_appWindow;
    Mantid::API::Workspace_sptr m_workspace;
    QTabWidget *m_tabs;
    QTableView *m_table_viewY;
    QTableView *m_table_viewX;
    QTableView *m_table_viewE;
    MantidMatrixModel *m_modelY;
    MantidMatrixModel *m_modelX;
    MantidMatrixModel *m_modelE;
    QColor m_bk_color;
    const char **m_matrix_icon;
	double x_start, //!< X value corresponding to column 1
	x_end,  //!< X value corresponding to the last column
	y_start,  //!< Y value corresponding to row 1
	y_end;  //!< Y value corresponding to the last row
    int m_rows,m_cols;
    int m_startRow;
    int m_endRow;
    bool m_histogram;
    double m_min;           // Saved minimum Y-value
    double m_max;           // Saved maximum Y-value
    bool m_are_min_max_set; // If true ::range does not iterate over WS to find min and max but uses m_min and m_max instead

    // MDI windows created by this MantidMatrix
    QVector<MultiLayer*> m_plots2D;
    QMap< MultiLayer*,Table* > m_plots1D;

	//! Column width in pixels;
	int m_column_width;
    MantidMatrixFunction m_funct;

    QAction *m_actionShowX;

    private:
    //name of the underlying workspace
    std::string m_strName;
};

/**
    MantidMatrixModel is an implementation of QAbstractTableModel which is an 
    interface between the data (workspace) and the widget displaying it (QTableView).
    It presents spectrum data (Type Y), bin boundaries (Type X), and errors (Type E) 
    as a table.
*/
class MantidMatrixModel:public QAbstractTableModel
{
    Q_OBJECT
public:
      typedef enum {Y,X,E} Type;
    MantidMatrixModel(QObject *parent, 
                      Mantid::API::Workspace_sptr ws, 
                      int rows,
                      int cols,
                      int start, 
                      Type type):
      QAbstractTableModel(parent),m_type(type)
      {
          setup(ws,rows,cols,start);
      }

    /// Call this function if the workspace has changed
    void setup(Mantid::API::Workspace_sptr ws, 
                      int rows,
                      int cols,
                      int start)
    {
        m_workspace = ws;
        m_rows = rows;
        m_cols = cols;
        m_startRow = start >= 0? start : 0;
        if (ws->blocksize() != 0)
            m_colNumCorr = ws->isHistogramData() ? 1 : 0;
        else
            m_colNumCorr = 0;
    }

    /// Implementation of QAbstractTableModel::rowCount() -- number of rows (spectra) that can be shown
    int rowCount(const QModelIndex &parent = QModelIndex()) const{return m_rows;}

    /// Implementation of QAbstractTableModel::columnCount() -- number of columns. If type is X it is
    /// the numner of bin boundaries. If type is Y or E it is the number of data values.
    int columnCount(const QModelIndex &parent = QModelIndex()) const{return m_type == X? m_cols + m_colNumCorr : m_cols;}

    double data(int row, int col) const
    {
        double val;
        if (m_type == X)
        {
            val = m_workspace->readX(row + m_startRow)[col];
        }
        else if (m_type == Y)
        {
            val = m_workspace->readY(row + m_startRow)[col];
        }
        else
        {
            val = m_workspace->readE(row + m_startRow)[col];
        }
        return val;
    }

    /// Implementation of QAbstractTableModel::data(...). QTableView uses this function
    /// to retrieve data for displaying.
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const   
    {
        if (role != Qt::DisplayRole) return QVariant();
        return section;
    }

    Qt::ItemFlags flags(const QModelIndex & index ) const
    {
	    if (index.isValid())
            return Qt::ItemIsSelectable;
	    else
            return Qt::ItemIsEnabled;
    }

public slots:
    /// Signals QTableView that the data have changed.
    void resetData(){reset();}
private:
    Mantid::API::Workspace_sptr m_workspace;
    int m_startRow; ///< starting workspace index to display
    int m_endRow;   ///< ending workspace index to display
    int m_rows,m_cols; ///< numbers of rows and columns
    int m_colNumCorr;  ///< == 1 for histograms and == 0 for point data
    QLocale m_locale;
    Type m_type;///< The type: X for bin boundaries, Y for the spectrum data, E for errors
};

/// Required by Qt to us Mantid::API::Workspace_sptr as a parameter type in signals
Q_DECLARE_METATYPE(Mantid::API::Workspace_sptr)

#endif
