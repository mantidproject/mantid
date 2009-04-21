#include "MantidMatrix.h"
#include "MantidUI.h"
#include "../Graph3D.h"
#include "../ApplicationWindow.h"
#include "../Spectrogram.h"
#include "MantidMatrixDialog.h"
#include "MantidDataObjects/Workspace2D.h"
#include "Preferences.h"
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
#include <QCheckBox>
#include <QTabWidget>

#include <stdlib.h>
#include <iostream>
#include <algorithm>

using namespace Mantid::API;

MantidMatrix::MantidMatrix(Mantid::API::MatrixWorkspace_sptr ws, ApplicationWindow* parent, const QString& label, const QString& name, int start, int end)
: MdiSubWindow(label, parent, name, 0),m_funct(this),m_histogram(false),
  y_start(0.0),y_end(0.0),m_min(0),m_max(0),m_are_min_max_set(false),
  m_replaceObserver(*this,&MantidMatrix::handleReplaceWorkspace),
  m_deleteObserver(*this,&MantidMatrix::handleDeleteWorkspace),
  m_rowBegin(-1), m_rowEnd(-1), m_colBegin(-1), m_colEnd(-1)
{
    m_appWindow = parent;
    m_strName = name.toStdString();
    m_workspace = ws;
    setup(ws,start,end);
	setWindowTitle(name);
	setName(name); 
	setIcon( QPixmap(matrixIcon()) );

    m_modelY = new MantidMatrixModel(this,ws.get(),m_rows,m_cols,m_startRow,MantidMatrixModel::Y);
    m_table_viewY = new QTableView();
    connectTableView(m_table_viewY,m_modelY);
    setColumnsWidth(0,MantidPreferences::MantidMatrixColumnWidthY());
    setNumberFormat(0,MantidPreferences::MantidMatrixNumberFormatY(),
                      MantidPreferences::MantidMatrixNumberPrecisionY());

    m_modelX = new MantidMatrixModel(this,ws.get(),m_rows,m_cols,m_startRow,MantidMatrixModel::X);
    m_table_viewX = new QTableView();
    connectTableView(m_table_viewX,m_modelX);
    setColumnsWidth(1,MantidPreferences::MantidMatrixColumnWidthX());
    setNumberFormat(1,MantidPreferences::MantidMatrixNumberFormatX(),
                      MantidPreferences::MantidMatrixNumberPrecisionX());

    m_modelE = new MantidMatrixModel(this,ws.get(),m_rows,m_cols,m_startRow,MantidMatrixModel::E);
    m_table_viewE = new QTableView();
    connectTableView(m_table_viewE,m_modelE);
    setColumnsWidth(2,MantidPreferences::MantidMatrixColumnWidthE());
    setNumberFormat(2,MantidPreferences::MantidMatrixNumberFormatE(),
                      MantidPreferences::MantidMatrixNumberPrecisionE());

    m_YTabLabel = QString("Y values");
    m_XTabLabel = QString("X values");
    m_ETabLabel = QString("Errors");

    m_tabs = new QTabWidget(this);
    m_tabs->insertTab(0,m_table_viewY, m_YTabLabel);
    m_tabs->insertTab(1,m_table_viewX, m_XTabLabel); 
    m_tabs->insertTab(2,m_table_viewE, m_ETabLabel);
    setWidget(m_tabs);

    setGeometry(50, 50, QMIN(5, numCols())*m_table_viewY->horizontalHeader()->sectionSize(0) + 55,
                (QMIN(10,numRows())+1)*m_table_viewY->verticalHeader()->sectionSize(0)+100);
 
    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_replaceObserver);
    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_deleteObserver);

    connect(this,SIGNAL(needChangeWorkspace(Mantid::API::MatrixWorkspace_sptr)),this,SLOT(changeWorkspace(Mantid::API::MatrixWorkspace_sptr)));
    connect(this,SIGNAL(needDeleteWorkspace()),this,SLOT(deleteWorkspace()));
    connect(this, SIGNAL(closedWindow(MdiSubWindow*)), this, SLOT(selfClosed(MdiSubWindow*)));

    askOnCloseEvent(false);
}

MantidMatrix::~MantidMatrix()
{
    Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_replaceObserver);
    Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_deleteObserver);
    delete m_modelY;
    delete m_modelX;
    delete m_modelE;
}

void MantidMatrix::setup(Mantid::API::MatrixWorkspace_sptr ws, int start, int end)
{
    if (!ws.get())
    {
        QMessageBox::critical(0,"WorkspaceMatrixModel error","2D workspace expected.");
        m_rows = 0;
    	m_cols = 0; 
        m_startRow = 0;
        m_endRow = 0;
        return;
    }

    m_workspaceTotalHist = ws->getNumberHistograms();
    m_startRow = (start<0 || start>=m_workspaceTotalHist)?0:start;
    m_endRow   = (end<0 || end>=m_workspaceTotalHist || end < start)? m_workspaceTotalHist - 1 : end;
    m_rows = m_endRow - m_startRow + 1;
	  m_cols = ws->blocksize(); 
    if ( ws->isHistogramData() ) m_histogram = true;
    connect(this,SIGNAL(needsUpdating()),this,SLOT(repaintAll()));

    x_start = ws->readX(0)[0];
    if (ws->readX(0).size() != ws->readY(0).size()) x_end = ws->readX(0)[ws->blocksize()];
    else
        x_end = ws->readX(0)[ws->blocksize()-1];
        
    // This is only meaningful if a 2D (or greater) workspace
    if (ws->axes() > 1)
    {
      const Mantid::API::Axis* const ax = ws->getAxis(1);
      y_start = (*ax)(m_startRow);
      y_end   = (*ax)(m_endRow);
    }

    m_bk_color = QColor(128, 255, 255);
    m_matrix_icon = mantid_matrix_xpm;
    m_column_width = 100;

    m_funct.init();

}

void MantidMatrix::connectTableView(QTableView* view,MantidMatrixModel*model)
{
    view->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
    view->setSelectionMode(QAbstractItemView::ContiguousSelection);// only one contiguous selection supported
    view->setModel(model);
    view->setCornerButtonEnabled(false);
    view->setFocusPolicy(Qt::StrongFocus);
    
    QPalette pal = view->palette();
    pal.setColor(QColorGroup::Base, m_bk_color);
    view->setPalette(pal);
    
    // set header properties
    QHeaderView* hHeader = (QHeaderView*)view->horizontalHeader();
    hHeader->setMovable(false);
    hHeader->setResizeMode(QHeaderView::Fixed);
    hHeader->setDefaultSectionSize(m_column_width);
    
    view->resizeRowToContents(0);
    int row_height = view->rowHeight(0);

	QHeaderView* vHeader = (QHeaderView*)view->verticalHeader();
    vHeader->setDefaultSectionSize(row_height);
    vHeader->setResizeMode(QHeaderView::Fixed);
    vHeader->setMovable(false);

}

double MantidMatrix::cell(int row, int col)
{
	return m_modelY->data(row, col);
}

QString MantidMatrix::text(int row, int col)
{
    return QString::number(activeModel()->data(row, col));
}

/** Sets new column width in a table view(s). 
    @param width New column width in pixels. All columns have the same width.
    @param all If true the change will be applied to all three table views.
*/
void MantidMatrix::setColumnsWidth(int width, bool all)
{
    if (all)
    {
        m_table_viewY->horizontalHeader()->setDefaultSectionSize(width);
        m_table_viewX->horizontalHeader()->setDefaultSectionSize(width);
        m_table_viewE->horizontalHeader()->setDefaultSectionSize(width);

        int cols = numCols();
        for(int i=0; i<cols; i++)
        {
            m_table_viewY->setColumnWidth(i, width);
            m_table_viewX->setColumnWidth(i, width);
            m_table_viewE->setColumnWidth(i, width);
        }
        MantidPreferences::MantidMatrixColumnWidth(width);
    }
    else
    {
        QTableView* table_view = activeView();
        table_view->horizontalHeader()->setDefaultSectionSize(width);
        int cols = numCols();
        for(int i=0; i<cols; i++)
            table_view->setColumnWidth(i, width);
        switch (m_tabs->currentIndex())
        {
        case 0: MantidPreferences::MantidMatrixColumnWidthY(width);break;
        case 1: MantidPreferences::MantidMatrixColumnWidthX(width);break;
        case 2: MantidPreferences::MantidMatrixColumnWidthE(width);break;
        }
    }

	emit modifiedWindow(this);
}

/**  Sets column width to one table view.
     @param i ordinal number of the view. 0 - Y, 1 - X, 2 - Error
     @param width New column width in pixels. All columns have the same width.
*/
void MantidMatrix::setColumnsWidth(int i,int width)
{

    QTableView* table_view;
    switch(i)
    {
    case 0: table_view =  m_table_viewY; MantidPreferences::MantidMatrixColumnWidthY(width); break;
    case 1: table_view =  m_table_viewX; MantidPreferences::MantidMatrixColumnWidthX(width); break;
    case 2: table_view =  m_table_viewE; MantidPreferences::MantidMatrixColumnWidthE(width); break;
    default: table_view = activeView();
    };

    table_view->horizontalHeader()->setDefaultSectionSize(width);
    int cols = numCols();
    for(int i=0; i<cols; i++)
        table_view->setColumnWidth(i, width);

	emit modifiedWindow(this);
}

/**  Returns the width of a column.
     @param i ordinal number of the view. 0 - Y, 1 - X, 2 - Error
     @return The column width in pixels. All columns have the same width.
*/
int MantidMatrix::columnsWidth(int i)
{
    switch(i)
    {
    case 0: return m_table_viewY->columnWidth(0);
    case 1: return m_table_viewX->columnWidth(0);
    case 2: return m_table_viewE->columnWidth(0);
    };
    return activeView()->columnWidth(0);
}

/**  Return the pointer to the active table view.
*/
QTableView *MantidMatrix::activeView()
{
    switch (m_tabs->currentIndex())
    {
    case 0: return m_table_viewY;
    case 1: return m_table_viewX;
    case 2: return m_table_viewE;
    }
    return m_table_viewY;
}

/**  Returns the pointer to the active model.
*/
MantidMatrixModel *MantidMatrix::activeModel()
{
    switch (m_tabs->currentIndex())
    {
    case 0: return m_modelY;
    case 1: return m_modelX;
    case 2: return m_modelE;
    }
    return m_modelY;
}

/**  Copies the current selection in the active table view into the system clipboard.
*/
void MantidMatrix::copySelection()
{
    QItemSelectionModel *selModel = activeView()->selectionModel();
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
				s += text(i, j) + "\t";
			s += text(i,right) + eol;
		}
	}
	// Copy text into the clipboard
	QApplication::clipboard()->setText(s.trimmed());
}

/**  Returns minimum and maximum values in the matrix.
     If setRange(...) has not been called it returns the true smalles ang largest Y-values in the matrix,
     otherwise the values set with setRange(...) are returned. These are needed in plotGraph2D to set
     the range of the third, colour axis.
*/
void MantidMatrix::range(double *min, double *max)
{
    if (!m_are_min_max_set)
    {
	    m_min = cell(0, 0);
	    m_max = m_min;
	    int rows = numRows();
	    int cols = numCols();

	    for(int i=0; i<rows; i++){
		    for(int j=0; j<cols; j++){
			    double aux = cell(i, j);
			    if (aux <= m_min)
				    m_min = aux;

			    if (aux >= m_max)
				    m_max = aux;
		    }
	    }
        m_are_min_max_set = true;
    }

    *min = m_min;
    *max = m_max;
}

/**  Sets new minimum and maximum Y-values which can be displayed in a 2D graph
*/
void MantidMatrix::setRange(double min, double max)
{
    m_min = min;
    m_max = max;
    m_are_min_max_set = true;
}

double** MantidMatrix::allocateMatrixData(int rows, int columns)
{
	double** data = (double **)malloc(rows * sizeof (double*));
	if(!data){
		QMessageBox::critical(0, tr("QtiPlot") + " - " + tr("Memory Allocation Error"),
		tr("Not enough memory, operation aborted!"));
		return NULL;
	}

	for ( int i = 0; i < rows; ++i){
		data[i] = (double *)malloc(columns * sizeof (double));
		if(!data[i]){
		    for ( int j = 0; j < i; j++)
                free(data[j]);
		    free(data);

			QMessageBox::critical(0, tr("QtiPlot") + " - " + tr("Memory Allocation Error"),
			tr("Not enough memory, operation aborted!"));
			return NULL;
		}
	}
	return data;
}

void MantidMatrix::freeMatrixData(double **data, int rows)
{
	for ( int i = 0; i < rows; i++)
		free(data[i]);

	free(data);
}

void MantidMatrix::goTo(int row,int col)
{
	if(row < 1 || row > numRows())
		return;
	if(col < 1 || col > numCols())
		return;

	activeView()->selectionModel()->select(activeModel()->index(row - 1, col - 1), QItemSelectionModel::ClearAndSelect);
	activeView()->scrollTo(activeModel()->index(row - 1, col - 1), QAbstractItemView::PositionAtTop);
}

void MantidMatrix::goToRow(int row)
{
	if(row < 1 || row > numRows())
		return;
	
	//	activeView()->selectRow(row - 1); //For some reason, this did not highlight the row at all, hence the stupid line below
	activeView()->selectionModel()->select(QItemSelection(activeModel()->index(row - 1, 0), activeModel()->index(row - 1, numCols() - 1)), QItemSelectionModel::ClearAndSelect);

	activeView()->scrollTo(activeModel()->index(row - 1, 0), QAbstractItemView::PositionAtCenter);
}

void MantidMatrix::goToColumn(int col)
{
	if(col < 1 || col > numCols())
		return;

	//	activeView()->selectColumn(col - 1); //For some reason, this did not highlight the row at all, hence the stupid line below
	activeView()->selectionModel()->select(QItemSelection(activeModel()->index(0, col - 1), activeModel()->index(numRows() - 1, col - 1)), QItemSelectionModel::ClearAndSelect);
	activeView()->scrollTo(activeModel()->index(0, col - 1), QAbstractItemView::PositionAtCenter);

}

double MantidMatrix::dataX(int row, int col) const
{
    if (!m_workspace || row >= numRows() || col >= m_workspace->readX(row + m_startRow).size()) return 0.;
    double res = m_workspace->readX(row + m_startRow)[col];
    return res;

}

double MantidMatrix::dataY(int row, int col) const
{
    if (!m_workspace || row >= numRows() || col >= numCols()) return 0.;
    double res = m_workspace->readY(row + m_startRow)[col];
    return res;

}

double MantidMatrix::dataE(int row, int col) const
{
    if (!m_workspace || row >= numRows() || col >= numCols()) return 0.;
    double res = m_workspace->readE(row + m_startRow)[col];
    if (res == 0.) res = 1.;//  quick fix of the fitting problem
    return res;

}

int MantidMatrix::indexX(double s)const
{
    int n = m_workspace->blocksize();

    if (n == 0 || s < m_workspace->readX(0)[0] || s > m_workspace->readX(0)[n-1]) return -1;

    int i = 0, j = n-1, k = n/2;
    double ss;
    int it;
    for(it=0;it<n;it++)
    {
        ss = m_workspace->readX(0)[k];
        if (ss == s || abs(i - j) <2) break;
        if (s > ss) i = k;
        else
            j = k;
        k = i + (j - i)/2;
    }

    return i;
}

QString MantidMatrix::workspaceName() const
{
  return QString::fromStdString(m_strName);
}

/*void MantidMatrix::copy(Matrix *m)
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
}*/

QwtDoubleRect MantidMatrix::boundingRect()
{
    int rows = numRows();
    int cols = numCols();
    double dx = fabs(x_end - x_start)/(double)(cols - 1);
    double dy = fabs(y_end - y_start)/(double)(rows - 1);

    return QwtDoubleRect(QMIN(x_start, x_end) - 0.5*dx, QMIN(y_start, y_end) - 0.5*dy,
						 fabs(x_end - x_start) + dx, fabs(y_end - y_start) + dy).normalized();
}

//----------------------------------------------------------------------------
void MantidMatrixFunction::init()
{
    int nx = m_matrix->numCols();
    int ny = m_matrix->numRows();

    m_dx = (m_matrix->xEnd() - m_matrix->xStart()) / (nx > 1? nx - 1 : 1);
    m_dy = (m_matrix->yEnd() - m_matrix->yStart()) / (ny > 1? ny - 1 : 1);

    if (m_dx == 0.) m_dx = 1.;//?
    if (m_dy == 0.) m_dy = 1.;//?
}

double MantidMatrixFunction::operator()(double x, double y)
{
    x += 0.5*m_dx;
    y -= 0.5*m_dy;
    	
    int i = abs((y - m_matrix->yStart())/m_dy);
    int j = abs((x - m_matrix->xStart())/m_dx);

    int jj = m_matrix->indexX(x);
    if (jj >= 0) j = jj;

    if (i >= 0 && i < m_matrix->numRows() && j >=0 && j < m_matrix->numCols())
	    return m_matrix->dataY(i,j);
    else
	    return 0.0;
}
//----------------------------------------------------------------------------

Graph3D * MantidMatrix::plotGraph3D(int style)
{
	QApplication::setOverrideCursor(Qt::WaitCursor);

    ApplicationWindow* a = applicationWindow();
	QString labl = a->generateUniqueName(tr("Graph"));

	Graph3D *plot = new Graph3D("", a);
	plot->resize(500,400);
	plot->setWindowTitle(labl);
	plot->setName(labl);
    plot->setTitle(tr("Workspace ")+name());
	a->customPlot3D(plot);
	plot->customPlotStyle(style);
    int resCol = numCols() / 200;
    int resRow = numRows() / 200;
    plot->setResolution( qMax(resCol,resRow) );

    double zMin =  1e300;
    double zMax = -1e300;
    for(int i=0;i<numRows();i++)
    for(int j=0;j<numCols();j++)
    {
        if (cell(i,j) < zMin) zMin = cell(i,j);
        if (cell(i,j) > zMax) zMax = cell(i,j);
    }
    
	plot->addFunction("", xStart(), xEnd(), yStart(), yEnd(), zMin, zMax, numCols(), numRows(), static_cast<UserHelperFunction*>(&m_funct));
	
    const Mantid::API::Axis* ax = m_workspace->getAxis(0);
    std::string s;
    if ( ax->unit() ) s = ax->unit()->caption() + " / " + ax->unit()->label();
    else
        s = "X Axis";
    plot->setXAxisLabel(tr(s.c_str()));
    
    if ( m_workspace->axes() > 1 )
    {
      ax = m_workspace->getAxis(1);
      if (ax->isNumeric()) 
      {
        if ( ax->unit() ) s = ax->unit()->caption() + " / " + ax->unit()->label();
        else
          s = "Y Axis";
        plot->setYAxisLabel(tr(s.c_str())); 
      }
      else
        plot->setYAxisLabel(tr("Spectrum"));
    }

    plot->setZAxisLabel(tr(m_workspace->YUnit().c_str())); 

    a->initPlot3D(plot);
    plot->askOnCloseEvent(false);
	QApplication::restoreOverrideCursor();

    return plot;
}

/** Creates a MultiLayer graph and plots this MantidMatrix as a Spectrogram.

    @param type The "curve" type.
    @return Pointer to the created graph.
*/
MultiLayer* MantidMatrix::plotGraph2D(Graph::CurveType type)
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    ApplicationWindow *a = applicationWindow();
	MultiLayer* g = a->multilayerPlot(a->generateUniqueName(tr("Graph")));
    m_plots2D<<g;
    connect(g, SIGNAL(closedWindow(MdiSubWindow*)), this, SLOT(dependantClosed(MdiSubWindow*)));
    a->connectMultilayerPlot(g);
    Graph* plot = g->activeGraph();
	a->setPreferences(plot);
    plot->setTitle(tr("Workspace ") + name());
    const Mantid::API::Axis* ax;
    ax = m_workspace->getAxis(0);
    std::string s;
    if (ax->unit().get()) s = ax->unit()->caption() + " / " + ax->unit()->label();
    else
        s = "X axis";
    plot->setXAxisTitle(tr(s.c_str()));
    if ( m_workspace->axes() > 1 )
    {
      ax = m_workspace->getAxis(1);
      if (ax->isNumeric()) 
      {
        if ( ax->unit() ) s = ax->unit()->caption() + " / " + ax->unit()->label();
        else
          s = "Y Axis";
        plot->setYAxisTitle(tr(s.c_str())); 
      }
      else
        plot->setYAxisTitle(tr("Spectrum"));
    }

    // Set the range on the thirs, colour axis
    double minz, maxz;
    range(&minz,&maxz);
    plot->plotSpectrogram(&m_funct, numRows(), numCols(), boundingRect(), minz, maxz, type);

    plot->setAutoScale();
    g->askOnCloseEvent(false);

	QApplication::restoreOverrideCursor();
	return g;
}

void MantidMatrix::setSpectrumGraph(MultiLayer *ml, Table* t)
{
    MantidUI::setUpSpectrumGraph(ml,name(),workspace());
    connect(ml, SIGNAL(closedWindow(MdiSubWindow*)), this, SLOT(dependantClosed(MdiSubWindow*)));
    if (t) 
    {
        m_plots1D[ml] = t;
        connect(t, SIGNAL(closedWindow(MdiSubWindow*)), this, SLOT(dependantClosed(MdiSubWindow*)));
    }
    else
        m_plots2D<<ml;
}

void MantidMatrix::setBinGraph(MultiLayer *ml, Table* t)
{
    MantidUI::setUpBinGraph(ml,name(),workspace());
    connect(ml, SIGNAL(closedWindow(MdiSubWindow*)), this, SLOT(dependantClosed(MdiSubWindow*)));
    if (t) 
    {
        m_plots1D[ml] = t;
        connect(t, SIGNAL(closedWindow(MdiSubWindow*)), this, SLOT(dependantClosed(MdiSubWindow*)));
    }
    else
        m_plots2D<<ml;
}

// Remove all references to the MantidMatrix
void MantidMatrix::removeWindow()
{
	QList<MdiSubWindow *> windows = applicationWindow()->windowsList();
	foreach(MdiSubWindow *w, windows){
		if (w->isA("Graph3D") && ((Graph3D*)w)->userFunction()->hlpFun() == &m_funct)
			((Graph3D*)w)->clearData();
		else if (w->isA("MultiLayer")){
			QList<Graph *> layers = ((MultiLayer*)w)->layersList();
			foreach(Graph *g, layers){
				for (int i=0; i<g->curves(); i++){
                    Spectrogram *sp = (Spectrogram *)g->plotItem(i);
                    if (sp && sp->rtti() == QwtPlotItem::Rtti_PlotSpectrogram && sp->funct() == &m_funct)
                        g->removeCurve(i);
				}
			}
		}
	}
}

void MantidMatrix::getSelectedRows(int& i0,int& i1)
{
  i0 = m_rowBegin;
  i1 = m_rowEnd;
}

/*
   Returns indices of the first and last selected rows in i0 and i1.
*/
bool MantidMatrix::setSelectedRows()
{
  QTableView *tv = activeView();
  QItemSelectionModel *selModel = tv->selectionModel();
  if( !selModel  ) return false;

  QPoint localCursor = tv->mapFromGlobal(QCursor::pos());
  //This is due to what I think is a bug in Qt where it seems to include
  //the height  of the horizontalHeader bar in determining the row that the mouse pointer
  //is pointing at. This results in the last row having an undefined index so we need to trick
  // the function by asking for a different postion
  localCursor.ry() -=  tv->horizontalHeader()->height();

  if( localCursor.x() > tv->verticalHeader()->width() ) return false;
  QModelIndex cursorIndex = tv->indexAt(localCursor);

  if( selModel->selection().contains(cursorIndex)  && 
      selModel->selection().front().left() == 0 && 
      selModel->selection().front().right() == tv->horizontalHeader()->count() - 1 )
  {
    m_rowBegin = selModel->selection().front().top();
    m_rowEnd = selModel->selection().front().bottom();
  }
  else
  {
    m_rowBegin = m_rowEnd = cursorIndex.row();
    tv->selectRow(m_rowBegin);
  }
  if( m_rowBegin == -1 || m_rowEnd == -1 ) return false;

  return true;
}

void MantidMatrix::getSelectedColumns(int& i0,int& i1)
{
  i0 = m_colBegin;
  i1 = m_colEnd;
}

/*
   Returns indices of the first and last selected column in i0 and i1.
*/
bool MantidMatrix::setSelectedColumns()
{
  QTableView *tv = activeView();
  QItemSelectionModel *selModel = tv->selectionModel();
  if( !selModel ) return false;

  QPoint localCursor = tv->mapFromGlobal(QCursor::pos());
  //This is due to what I think is a bug in Qt where it seems to include
  //the width of the verticalHeader bar in determining the column that the mouse pointer
  //is pointing at
  localCursor.rx() -=  tv->verticalHeader()->width();
  if( localCursor.y() > tv->horizontalHeader()->height() ) return false;

  QModelIndex index = tv->indexAt(localCursor);
  QModelIndex cursorIndex = index.sibling(index.row(), index.column()); 

  if( selModel->selection().contains(cursorIndex) )
  {
    m_colBegin = selModel->selection().front().left();
    m_colEnd = selModel->selection().front().right();
  }
  else
  {
    m_colBegin = m_colEnd = tv->indexAt(localCursor).column();
    tv->selectColumn(m_colBegin);
  }

  // tv->horizontalHeader()->count() can be small...
  //if( m_colBegin == 0 && m_colEnd == tv->horizontalHeader()->count() - 1 ) return false;

  return true;
}

void MantidMatrix::tst()
{
    std::cerr<<"2D plots: "<<m_plots2D.size()<<'\n';
    std::cerr<<"1D plots: "<<m_plots1D.size()<<'\n';
}

void MantidMatrix::dependantClosed(MdiSubWindow* w)
{
  if( w->isA("Table") )
  {
    QMap<MultiLayer*,Table*>::iterator itr;
    for( itr = m_plots1D.begin(); itr != m_plots1D.end(); ++itr )
    {
      if( itr.value() == (Table*)w )
      {
          m_plots1D.erase(itr);
          break;
      }
    }
  }
  else if (w->isA("MultiLayer")) 
  {
    int i = m_plots2D.indexOf((MultiLayer*)w);
    if (i >= 0) m_plots2D.remove(i);
    else
    {
      QMap<MultiLayer*,Table*>::iterator i = m_plots1D.find((MultiLayer*)w);
      if (i != m_plots1D.end())
	{
	  if (i.value() != 0) 
                {
                    i.value()->askOnCloseEvent(false);
                    i.value()->close();
                }
                m_plots1D.erase(i);
            }
        }
    }
}

/**
     Repaints all 1D and 2D plots attached to this MantidMatrix
*/
void MantidMatrix::repaintAll()
{
    repaint();

    // Repaint 2D plots
    QVector<MultiLayer*>::iterator vEnd  = m_plots2D.end();
    for( QVector<MultiLayer*>::iterator vItr = m_plots2D.begin(); vItr != vEnd; ++vItr )
    {
      (*vItr)->activeGraph()->replot();
    }

    // Updates the 1D plots by modifying the attached tables
    QMap<MultiLayer*,Table*>::iterator mEnd = m_plots1D.end();
    for(QMap<MultiLayer*,Table*>::iterator mItr = m_plots1D.begin(); mItr != mEnd;  ++mItr)
    {
        Table* t = mItr.value();
        if ( !t ) continue;
        int charsToRemove = t->name().size() + 1;
	int nTableCols(t->numCols());
        for(int col = 1; col < nTableCols; ++col)
        {
	  QString colName = t->colName(col).remove(0,charsToRemove);
	  if( colName.isEmpty() ) break;
	  //Need to determine whether the table was created from plotting a spectrum
	  //or a time bin. A spectrum has a Y column name YS and a bin YB
	  QString ident = colName.left(2);
	  colName.remove(0,2); //This now contains the number in the MantidMatrix
	  int matrixNumber = colName.toInt();
	  if( matrixNumber < 0 ) break;
	  bool errs = (ident[0] == QChar('E')) ? true : false;
	  if( ident[1] == QChar('S') )
	  {
	    if( matrixNumber >= numRows() ) break;
	    int endCount = numCols();
	    for(int j = 0; j < endCount; ++j)
	    {
	      if( errs ) t->setCell(j, col, dataE(matrixNumber, j));
	      else t->setCell(j, col, dataY(matrixNumber, j));
	    }	    
	  }
	  else
	  {
	    if( matrixNumber >= numCols() ) break;
	    int endCount = numRows();
	    for(int j = 0; j < endCount; ++j)
	    {
	      if( errs ) t->setCell(j, col, dataE(j, matrixNumber));
	      else t->setCell(j, col, dataY(j, matrixNumber));
	    }
	  }
        }
        t->notifyChanges();
        Graph *g = mItr.key()->activeGraph();
        if (g) g->setAutoScale();
    }
}

void MantidMatrix::handleReplaceWorkspace(const Poco::AutoPtr<Mantid::Kernel::DataService<Mantid::API::Workspace>::AfterReplaceNotification>& pNf)
{
  if( pNf->object_name() != m_strName || !pNf->object().get() ) return;
  
  Mantid::API::MatrixWorkspace_sptr new_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(m_strName));
  m_workspace = new_workspace;
  emit needChangeWorkspace( new_workspace );
  
}
 
void MantidMatrix::changeWorkspace(Mantid::API::MatrixWorkspace_sptr ws)
{
    if (m_cols != ws->blocksize() ||
        m_workspaceTotalHist != ws->getNumberHistograms())
      { 
        closeDependants();
      }

  
    // Save selection
    QItemSelectionModel *oldSelModel = activeView()->selectionModel();
    QModelIndexList indexList = oldSelModel->selectedIndexes();
    QModelIndex curIndex = activeView()->currentIndex();
    
    setup(ws,-1,-1);
    
    //delete m_modelY;
    m_modelY = new MantidMatrixModel(this,ws.get(),m_rows,m_cols,m_startRow,MantidMatrixModel::Y);
    connectTableView(m_table_viewY,m_modelY);

    //delete m_modelX;
    m_modelX = new MantidMatrixModel(this,ws.get(),m_rows,m_cols,m_startRow,MantidMatrixModel::X);
    connectTableView(m_table_viewX,m_modelX);

    //delete m_modelE;
    m_modelE = new MantidMatrixModel(this,ws.get(),m_rows,m_cols,m_startRow,MantidMatrixModel::E);
    connectTableView(m_table_viewE,m_modelE);

    // Restore selection
    activeView()->setCurrentIndex(curIndex);
    if (indexList.size())
    {
        QItemSelection sel(indexList.first(),indexList.last());
        QItemSelectionModel *selModel = activeView()->selectionModel();
        selModel->select(sel,QItemSelectionModel::Select);
    }

    repaintAll();

} 

void MantidMatrix::closeDependants()
{
  while(m_plots2D.size())
  {
      MultiLayer* ml = m_plots2D.front();
      ml->askOnCloseEvent(false);
      ml->close();// this calls slot dependantClosed() which removes the pointer from m_plots2D
  }
  while(m_plots1D.size())
  {
      MultiLayer* ml = m_plots1D.begin().key();
      ml->askOnCloseEvent(false);
      ml->close();// this calls slot dependantClosed() which removes the pointer from m_plots1D
  }
}

void MantidMatrix::setNumberFormat(const QChar& f,int prec, bool all)
{
    if (all)
    {
        modelY()->setFormat(f,prec);
        modelX()->setFormat(f,prec);
        modelE()->setFormat(f,prec);
        MantidPreferences::MantidMatrixNumberFormat(f);
        MantidPreferences::MantidMatrixNumberPrecision(prec);
    }
    else
    {
        activeModel()->setFormat(f,prec);
        switch (m_tabs->currentIndex())
        {
        case 0: MantidPreferences::MantidMatrixNumberFormatY(f);
                MantidPreferences::MantidMatrixNumberPrecisionY(prec);
                break;
        case 1: MantidPreferences::MantidMatrixNumberFormatX(f);
                MantidPreferences::MantidMatrixNumberPrecisionX(prec);
                break;
        case 2: MantidPreferences::MantidMatrixNumberFormatE(f);
                MantidPreferences::MantidMatrixNumberPrecisionE(prec);
                break;
        }
    }
}

void MantidMatrix::setNumberFormat(int i,const QChar& f,int prec, bool all)
{
    
    switch (i)
    {
    case 0: m_modelY->setFormat(f,prec);
            MantidPreferences::MantidMatrixNumberFormatY(f);
            MantidPreferences::MantidMatrixNumberPrecisionY(prec);
            break;
    case 1: m_modelX->setFormat(f,prec);
            MantidPreferences::MantidMatrixNumberFormatX(f);
            MantidPreferences::MantidMatrixNumberPrecisionX(prec);
            break;
    case 2: m_modelE->setFormat(f,prec);
            MantidPreferences::MantidMatrixNumberFormatE(f);
            MantidPreferences::MantidMatrixNumberPrecisionE(prec);
            break;
    }
}

QChar MantidMatrix::numberFormat()
{
    return activeModel()->format();
}

int MantidMatrix::precision()
{
    return activeModel()->precision();
}



void MantidMatrix::setMatrixProperties()
{
    MantidMatrixDialog* dlg = new MantidMatrixDialog(m_appWindow);
    dlg->setMatrix(this);
    dlg->exec();
}

void MantidMatrix::handleDeleteWorkspace(const Poco::AutoPtr<Mantid::Kernel::DataService<Mantid::API::Workspace>::DeleteNotification>& pNf)
{
    if (pNf->object() == m_workspace)
    {
        emit needDeleteWorkspace();
    }
}

void MantidMatrix::deleteWorkspace()
{
    askOnCloseEvent(false);
    close();
}

void MantidMatrix::selfClosed(MdiSubWindow* w)
{
  closeDependants();
}

//-------------------------------
// Python API commands
//------------------------------

void MantidMatrix::goToTab(const QString & name)
{
  if( m_tabs->tabText(m_tabs->currentIndex()) == name ) return;

  if( name == m_YTabLabel )
  {
    m_tabs->setCurrentIndex(0);
  }
  else if( name == m_XTabLabel )
  {
    m_tabs->setCurrentIndex(1);
  }
  else if( name == m_ETabLabel )
  {
    m_tabs->setCurrentIndex(2);
  }
  else return;
}

// ----------   MantidMatrixModel   ------------------ //

/**   MantidMatrixModel constructor.
      @param parent Pointer to the parent MantidMatrix
      @param ws Underlying workspace
      @param rows Number of rows in the workspace to be visible via MantidMatrixModel
      @param cols Number of columns (time bins)
      @param start Starting index 
      @param type Type of the data to display: Y, X, or E
  */
MantidMatrixModel::MantidMatrixModel(QObject *parent, 
                  Mantid::API::MatrixWorkspace* ws, 
                  int rows,
                  int cols,
                  int start, 
                  Type type):
QAbstractTableModel(parent),m_type(type),
m_format('e'),m_prec(6)
  {
      setup(ws,rows,cols,start);
  }

/// Call this function if the workspace has changed
void MantidMatrixModel::setup(Mantid::API::MatrixWorkspace* ws, 
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


double MantidMatrixModel::data(int row, int col) const
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

QVariant MantidMatrixModel::headerData(int section, Qt::Orientation orientation, int role ) const   
{
    if (role != Qt::DisplayRole) return QVariant();
    return section;
}

Qt::ItemFlags MantidMatrixModel::flags(const QModelIndex & index ) const
{
    if (index.isValid())
        return Qt::ItemIsSelectable;
    else
        return Qt::ItemIsEnabled;
}

/**
      @param f Number format:  'f' - fixed, 'e' - scientific.
      @param prec New precision (number of digits after the decimal point) with which the data will
                   be shown in MantidMatrix.
  */
void MantidMatrixModel::setFormat(const QChar& f,int prec)
{
    QString formats = " ef";
    if ( formats.indexOf(f) > 0 )
    {
        m_format = f.toAscii();
        m_prec = prec;
    }
}

QVariant MantidMatrixModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole) return QVariant();
    double val = data(index.row(),index.column());
    return QVariant(m_locale.toString(val,m_format,m_prec));
}
