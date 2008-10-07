#include "MantidMatrix.h"
#include "MantidUI.h"
#include "../Graph3D.h"
#include "../ApplicationWindow.h"
#include "../Spectrogram.h"
#include "MantidDataObjects/Workspace2D.h"

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

MantidMatrix::MantidMatrix(Mantid::API::Workspace_sptr ws, ApplicationWindow* parent, const QString& label, const QString& name, int start, int end, bool filter, double maxv)
: MdiSubWindow(label, parent, name, 0),m_funct(this),
m_replaceObserver(*this,&MantidMatrix::handleReplaceWorkspace),
m_deleteObserver(*this,&MantidMatrix::handleDeleteWorkspace)
{
    m_appWindow = parent;
    setup(ws,start,end,filter,maxv);
	setWindowTitle(name);
	setName(name); 
	setIcon( QPixmap(matrixIcon()) );

    m_modelY = new MantidMatrixModel(this,ws,m_rows,m_cols,m_startRow,filter,maxv,MantidMatrixModel::Y);
    m_table_viewY = new QTableView();
    connectTableView(m_table_viewY,m_modelY);

    m_modelX = new MantidMatrixModel(this,ws,m_rows,m_cols,m_startRow,filter,maxv,MantidMatrixModel::X);
    m_table_viewX = new QTableView();
    connectTableView(m_table_viewX,m_modelX);

    m_modelE = new MantidMatrixModel(this,ws,m_rows,m_cols,m_startRow,filter,maxv,MantidMatrixModel::E);
    m_table_viewE = new QTableView();
    connectTableView(m_table_viewE,m_modelE);

    m_tabs = new QTabWidget(this);
    m_tabs->insertTab(0,m_table_viewY,"Y values");
    m_tabs->insertTab(1,m_table_viewX,"X values"); 
    m_tabs->insertTab(2,m_table_viewE,"Errors");
    setWidget(m_tabs);

    setGeometry(50, 50, QMIN(5, numCols())*m_table_viewY->horizontalHeader()->sectionSize(0) + 55,
                (QMIN(10,numRows())+1)*m_table_viewY->verticalHeader()->sectionSize(0)+100);
 
    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_replaceObserver);
    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_deleteObserver);
    static bool Workspace_sptr_qRegistered = false;
    if (!Workspace_sptr_qRegistered)
    {
        Workspace_sptr_qRegistered = true;
        qRegisterMetaType<Mantid::API::Workspace_sptr>();
    }
    connect(this,SIGNAL(needChangeWorkspace(Mantid::API::Workspace_sptr)),this,SLOT(changeWorkspace(Mantid::API::Workspace_sptr)));
    connect(this,SIGNAL(needDeleteWorkspace()),this,SLOT(deleteWorkspace()));
    connect(this, SIGNAL(closedWindow(MdiSubWindow*)), this, SLOT(selfClosed(MdiSubWindow*)));
}

MantidMatrix::~MantidMatrix()
{
    Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_replaceObserver);
    Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_deleteObserver);
    delete m_modelY;
    delete m_modelX;
    delete m_modelE;
}

void MantidMatrix::setup(Mantid::API::Workspace_sptr ws, int start, int end, bool filter, double maxv)
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
    m_workspace = ws;
    m_startRow = (start<0 || start>=ws->getNumberHistograms())?0:start;
    m_endRow   = (end<0 || end>=ws->getNumberHistograms() || end < start)?ws->getNumberHistograms()-1:end;
    m_rows = m_endRow - m_startRow + 1;
	m_cols = ws->blocksize(); 
    m_filter = filter;
    m_maxv = maxv;
    m_histogram = false;
    if ( m_workspace->blocksize() || m_workspace->dataX(0).size() != m_workspace->dataY(0).size() ) m_histogram = true;
    connect(this,SIGNAL(needsUpdating()),this,SLOT(repaintAll()));

    x_start = ws->dataX(0)[0];
    if (ws->dataX(0).size() != ws->dataY(0).size()) x_end = ws->dataX(0)[ws->blocksize()];
    else
        x_end = ws->dataX(0)[ws->blocksize()-1];
    // What if y is not a spectrum number?
    y_start = double(m_startRow);
    y_end = double(m_endRow);

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
    view->setEditTriggers(QAbstractItemView::DoubleClicked);
    view->setFocusPolicy(Qt::StrongFocus);
    //view->setFocus();

    QPalette pal = view->palette();
	pal.setColor(QColorGroup::Base, m_bk_color);
	view->setPalette(pal);

	// set header properties
	QHeaderView* hHeader = (QHeaderView*)view->horizontalHeader();
	hHeader->setMovable(false);
	hHeader->setResizeMode(QHeaderView::Fixed);
	hHeader->setDefaultSectionSize(m_column_width);

    int cols = numCols();
	for(int i=0; i<cols; i++)
		view->setColumnWidth(i, m_column_width);

	QHeaderView* vHeader = (QHeaderView*)view->verticalHeader();
	vHeader->setMovable(false);
	vHeader->setResizeMode(QHeaderView::ResizeToContents);

}

double MantidMatrix::cell(int row, int col)
{
	return m_modelY->data(row, col);
}

QString MantidMatrix::text(int row, int col)
{
    return QString::number(activeModel()->data(row, col));
}

void MantidMatrix::setColumnsWidth(int width)
{
	if (m_column_width == width)
		return;

    m_column_width = width;
    m_table_viewY->horizontalHeader()->setDefaultSectionSize(m_column_width);
    m_table_viewX->horizontalHeader()->setDefaultSectionSize(m_column_width);
    m_table_viewE->horizontalHeader()->setDefaultSectionSize(m_column_width);

    int cols = numCols();
    for(int i=0; i<cols; i++)
    {
        m_table_viewY->setColumnWidth(i, width);
        m_table_viewX->setColumnWidth(i, width);
        m_table_viewE->setColumnWidth(i, width);
    }

	emit modifiedWindow(this);
}

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

void MantidMatrix::range(double *min, double *max)
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

	activeView()->scrollTo(activeModel()->index(row - 1, col - 1), QAbstractItemView::PositionAtTop);
}

void MantidMatrix::goToRow(int row)
{
	if(row < 1 || row > numRows())
		return;

	activeView()->selectRow(row - 1);
	activeView()->scrollTo(activeModel()->index(row - 1, 0), QAbstractItemView::PositionAtTop);
}

void MantidMatrix::goToColumn(int col)
{
	if(col < 1 || col > numCols())
		return;

	activeView()->selectColumn(col - 1);
	activeView()->scrollTo(activeModel()->index(0, col - 1), QAbstractItemView::PositionAtCenter);
}

double MantidMatrix::dataX(int row, int col) const
{
    if (!m_workspace || row >= numRows() || col >= m_workspace->dataX(row + m_startRow).size()) return 0.;
    double res = m_workspace->dataX(row + m_startRow)[col];
    return res;

}

double MantidMatrix::dataY(int row, int col) const
{
    if (!m_workspace || row >= numRows() || col >= numCols()) return 0.;
    double res = m_workspace->dataY(row + m_startRow)[col];
    return res;

}

double MantidMatrix::dataE(int row, int col) const
{
    if (!m_workspace || row >= numRows() || col >= numCols()) return 0.;
    double res = m_workspace->dataE(row + m_startRow)[col];
    if (res == 0.) res = 1.;//  quick fix of the fitting problem
    return res;

}

int MantidMatrix::indexX(double s)const
{
    int n = m_workspace->blocksize();

    if (n == 0 || s < m_workspace->dataX(0)[0] || s > m_workspace->dataX(0)[n-1]) return -1;

    int i = 0, j = n-1, k = n/2;
    double ss;
    int it;
    for(it=0;it<n;it++)
    {
        ss = m_workspace->dataX(0)[k];
        if (ss == s || abs(i - j) <2) break;
        if (s > ss) i = k;
        else
            j = k;
        k = i + (j - i)/2;
    }

    return i;
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
	
    Mantid::API::Axis* ax = m_workspace->getAxis(0);
    std::string s;
    if (ax->unit().get()) s = ax->unit()->caption() + " / " + ax->unit()->label();
    else
        s = "X Axis";
    plot->setXAxisLabel(tr(s.c_str()));
    
    ax = m_workspace->getAxis(1);
    if (ax->isNumeric()) 
    {
        if (ax->unit().get()) s = ax->unit()->caption() + " / " + ax->unit()->label();
        else
            s = "Y Axis";
        plot->setYAxisLabel(tr(s.c_str())); 
    }
    else
        plot->setYAxisLabel(tr("Spectrum")); 

    plot->setZAxisLabel(tr("Counts")); 

    a->initPlot3D(plot);
	QApplication::restoreOverrideCursor();

    return plot;
}

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
    Mantid::API::Axis* ax;
    ax = m_workspace->getAxis(0);
    std::string s;
    if (ax->unit().get()) s = ax->unit()->caption() + " / " + ax->unit()->label();
    else
        s = "X axis";
    plot->setXAxisTitle(tr(s.c_str()));
    ax = m_workspace->getAxis(1);
    if (ax->isNumeric()) 
    {
        if (ax->unit().get()) s = ax->unit()->caption() + " / " + ax->unit()->label();
        else
            s = "Y axis";
        plot->setYAxisTitle(tr(s.c_str())); 
    }
    else
        plot->setYAxisTitle(tr("Spectrum")); 

    double minz, maxz;
    range(&minz,&maxz);
	plot->plotSpectrogram(&m_funct, numRows(), numCols(), boundingRect(), minz, maxz, type);

    plot->setAutoScale();

	QApplication::restoreOverrideCursor();
	return g;
}

void MantidMatrix::setGraph1D(MultiLayer *ml, Table* t)
{
    Graph* g = ml->activeGraph();
    g->setTitle(tr("Workspace ")+name());
    Mantid::API::Axis* ax;
    ax = m_workspace->getAxis(0);
    std::string s;
    if (ax->unit().get()) s = ax->unit()->caption() + " / " + ax->unit()->label();
    else
        s = "X axis";
    g->setXAxisTitle(tr(s.c_str()));
    g->setYAxisTitle(tr("Counts")); 
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
    i0 = i1 = -1;
    QTableView *tv = activeView();
	QItemSelectionModel *selModel = activeView()->selectionModel();
	if (!selModel || !selModel->hasSelection())
		return;

	int rows = numRows();
    bool selectionStarted = false;
	for (int i = 0; i<rows; i++){
		if (selModel->isRowSelected (i, QModelIndex()))
            if (!selectionStarted)
            {
                selectionStarted = true;
                i0 = i1 = i;
            }else
                i1++;
	}
}

void MantidMatrix::tst()
{
    std::cerr<<"2D plots: "<<m_plots2D.size()<<'\n';
    std::cerr<<"1D plots: "<<m_plots1D.size()<<'\n';
}

void MantidMatrix::dependantClosed(MdiSubWindow* w)
{
    if (w->isA("MultiLayer")) 
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

void MantidMatrix::repaintAll()
{
    repaint();
    for(int i=0;i<m_plots2D.size();i++)
    {
        Graph *g = m_plots2D[i]->activeGraph();
        g->replot();
    }
    for(QMap<MultiLayer*,Table*>::iterator it = m_plots1D.begin();it!=m_plots1D.end();it++)
    {
        Table* t = it.value();
        if (!t) continue;
        int charsToRemove = t->name().size() + 1;
        for(int col=1;col<t->numCols();col++)
        {
            QString name = t->colName(col).remove(0,charsToRemove);
            if (name.isEmpty()) break;
            // Retrieve row number from the column name
            QChar c = name[0];
            name.remove(0,1);
            int i = name.toInt();
            if (i < 0 || i >= numRows()) break;
            if (c == 'Y')
                for(int j=0;j<numCols();j++)
                {
                    t->setCell(j,col, dataY(i,j));
                }
        }
        t->notifyChanges();
        Graph *g = it.key()->activeGraph();
        if (g) g->setAutoScale();
    }
}

void MantidMatrix::handleReplaceWorkspace(const Poco::AutoPtr<Mantid::Kernel::DataService<Mantid::API::Workspace>::ReplaceNotification>& pNf)
{
    if (pNf->object() == m_workspace)
    {
        emit needChangeWorkspace(pNf->new_object());
    }
}

void MantidMatrix::changeWorkspace(Mantid::API::Workspace_sptr ws)
{

    if (m_workspace->blocksize() != ws->blocksize() ||
        m_workspace->getNumberHistograms() != ws->getNumberHistograms()) closeDependants();

    // Save selection
    QItemSelectionModel *oldSelModel = activeView()->selectionModel();
    QModelIndexList indexList = oldSelModel->selectedIndexes();
    QModelIndex curIndex = activeView()->currentIndex();

    setup(ws,m_startRow,m_endRow,m_filter,m_maxv);
    
    delete m_modelY;
    m_modelY = new MantidMatrixModel(this,ws,m_rows,m_cols,m_startRow,m_filter,m_maxv,MantidMatrixModel::Y);
    connectTableView(m_table_viewY,m_modelY);

    delete m_modelX;
    m_modelX = new MantidMatrixModel(this,ws,m_rows,m_cols,m_startRow,m_filter,m_maxv,MantidMatrixModel::X);
    connectTableView(m_table_viewX,m_modelX);

    delete m_modelE;
    m_modelE = new MantidMatrixModel(this,ws,m_rows,m_cols,m_startRow,m_filter,m_maxv,MantidMatrixModel::E);
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
    for(int i=0;i<m_plots2D.size();i++)
    {
        m_plots2D[i]->askOnCloseEvent(false);
        m_plots2D[i]->close();
    }
    for(QMap<MultiLayer*,Table*>::iterator it = m_plots1D.begin();it!=m_plots1D.end();it++)
    {
        it.key()->askOnCloseEvent(false);
        it.key()->close();
    }
    m_plots2D.clear();
    m_plots1D.clear();
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
