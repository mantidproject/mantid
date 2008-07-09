#ifndef MANTIDMATRIX_H
#define MANTIDMATRIX_H

#include <QHeaderView>
#include <QTableView>
#include <QPrinter>
#include <QMessageBox>

#include "MantidAPI/Workspace.h"
#include "../UserFunction.h"
#include "../MdiSubWindow.h"
#include "../Graph.h"

#include <qwt_double_rect.h>
#include <qwt_color_map.h>

#include <math.h>

class QLabel;
class QStackedWidget;
class QShortcut;
class MantidMatrixModel;
class MantidMatrix;
class ApplicationWindow;
class Graph3D;
class MultiLayer;

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

class MantidMatrix: public MdiSubWindow
{
    Q_OBJECT


public:

	MantidMatrix(Mantid::API::Workspace_sptr ws, ApplicationWindow* parent, const QString& label, const QString& name = QString(), int start=-1, int end=-1, bool filter=false, double maxv=0);
    ~MantidMatrix();

	MantidMatrixModel * model(){return m_model;};
	QItemSelectionModel * selectionModel(){return m_table_view->selectionModel();};

    int numRows()const{return m_rows;}
    int numCols()const{return m_cols;}
	double dataX(int row, int col) const;
	double dataE(int row, int col) const;
    int indexX(double s)const;

    const char **matrixIcon(){return m_matrix_icon;}
    //void copy(Matrix *m);
    ApplicationWindow *appWindow(){return m_appWindow;}
    Graph3D *plotGraph3D(int style);
    MultiLayer* plotGraph2D(Graph::CurveType type);
    void setGraph1D(Graph* g);
    void removeWindow();
    void getSelectedRows(int& i0,int& i1);

public slots:

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

	int verticalHeaderWidth(){return m_table_view->verticalHeader()->width();}

protected:

    ApplicationWindow *m_appWindow;
    Mantid::API::Workspace_sptr m_workspace;
    QTableView *m_table_view;
    MantidMatrixModel *m_model;
    QColor m_bk_color;
    const char **m_matrix_icon;
	double x_start, //!< X value corresponding to column 1
	x_end,  //!< X value corresponding to the last column
	y_start,  //!< Y value corresponding to row 1
	y_end;  //!< Y value corresponding to the last row
    int m_rows,m_cols;
    int m_startRow;
    int m_endRow;
    bool m_filter;
    double m_maxv;

	//! Column width in pixels;
	int m_column_width;
    MantidMatrixFunction m_funct;
};

class MantidMatrixModel:public QAbstractTableModel
{
    Q_OBJECT
public:
    MantidMatrixModel(QObject *parent, Mantid::API::Workspace_sptr ws, int rows,int cols,int start=-1, int end=-1, bool filter=false, double maxv=0):
      QAbstractTableModel(parent),m_workspace(ws),m_filter(filter),m_maxv(maxv),m_rows(rows),m_cols(cols)
      {
          m_startRow = start >= 0? start : 0;
          m_endRow = end >= m_startRow? end : m_workspace->getNumberHistograms();
      }
    int rowCount(const QModelIndex &parent = QModelIndex()) const{return m_rows;}
    int columnCount(const QModelIndex &parent = QModelIndex()) const{return m_cols;}
    double data(int row, int col) const
    {
        double val = m_workspace->dataY(row + m_startRow)[col];
        if (m_filter && val > m_maxv) val = m_maxv;
        return val;
    }
    QVariant data(const QModelIndex &index, int role) const
    {
        
        if (role != Qt::DisplayRole) return QVariant();
        double val = m_workspace->dataY(index.row() + m_startRow)[index.column()];
        return QVariant(m_locale.toString(val,'f',6));
    }
    Qt::ItemFlags flags(const QModelIndex & index ) const
    {
	    if (index.isValid())
            return Qt::ItemIsSelectable;
	    else
            return Qt::ItemIsEnabled;
    }

private:
    Mantid::API::Workspace_sptr m_workspace;
    int m_startRow;
    int m_endRow;
    bool m_filter;
    double m_maxv;
    int m_rows,m_cols;
    QLocale m_locale;
};

#endif
