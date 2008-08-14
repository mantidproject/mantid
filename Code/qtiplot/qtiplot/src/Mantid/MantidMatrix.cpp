#include "MantidMatrix.h"
#include "MantidUI.h"
#include "../Graph3D.h"
#include "../ApplicationWindow.h"
#include "../Spectrogram.h"
#include "../../../../Mantid/DataHandling/src/LoadDAE/idc.h"

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
: MdiSubWindow(label, parent, name, 0),m_workspace(ws),m_funct(this)
{
    if (!m_workspace)
    {
        QMessageBox::critical(0,"WorkspaceMatrixModel error","2D workspace expected.");
        m_rows = 0;
    	m_cols = 0; 
        m_startRow = 0;
        m_endRow = 0;
        return;
    }

    m_startRow = (start<0 || start>=ws->getNumberHistograms())?0:start;
    m_endRow   = (end<0 || end>=ws->getNumberHistograms() || end < start)?ws->getNumberHistograms()-1:end;
    m_rows = m_endRow - m_startRow + 1;
	m_cols = ws->blocksize(); 
    m_filter = filter;
    m_maxv = maxv;
    m_histogram = false;
    if ( m_workspace->blocksize() || m_workspace->dataX(0).size() != m_workspace->dataY(0).size() ) m_histogram = true;
    m_DAEname = "";
    m_spectrum_min = -1;
    m_spectrum_max = -1;
    m_canUpdateDAE = false;
    m_updateDAEThread = 0;
    m_needDAERepaint = false;
    m_updateFrequency = 0;
    connect(this,SIGNAL(needsUpdating()),this,SLOT(repaintAll()));

    x_start = ws->dataX(0)[0];
    x_end = ws->dataX(0)[ws->blocksize()];
    // What if y is not a spectrum number?
    y_start = double(m_startRow);
    y_end = double(m_endRow);

    m_bk_color = QColor(128, 255, 255);
    m_matrix_icon = mantid_matrix_xpm;
    m_column_width = 100;
    m_model = new MantidMatrixModel(this,ws,m_rows,m_cols,m_startRow,m_endRow,filter,maxv,MantidMatrixModel::Y);

    m_table_view = new QTableView();
    m_table_view->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
    m_table_view->setSelectionMode(QAbstractItemView::ContiguousSelection);// only one contiguous selection supported
    m_table_view->setModel(m_model);
    m_table_view->setEditTriggers(QAbstractItemView::DoubleClicked);
    m_table_view->setFocusPolicy(Qt::StrongFocus);
    m_table_view->setFocus();

    QPalette pal = m_table_view->palette();
	pal.setColor(QColorGroup::Base, m_bk_color);
	m_table_view->setPalette(pal);

	// set header properties
	QHeaderView* hHeader = (QHeaderView*)m_table_view->horizontalHeader();
	hHeader->setMovable(false);
	hHeader->setResizeMode(QHeaderView::Fixed);
	hHeader->setDefaultSectionSize(m_column_width);

    int cols = numCols();
	for(int i=0; i<cols; i++)
		m_table_view->setColumnWidth(i, m_column_width);

	QHeaderView* vHeader = (QHeaderView*)m_table_view->verticalHeader();
	vHeader->setMovable(false);
	vHeader->setResizeMode(QHeaderView::ResizeToContents);

//-------------------------- X Values View
    m_modelX = new MantidMatrixModel(this,ws,m_rows,m_cols,m_startRow,m_endRow,filter,maxv,MantidMatrixModel::X);

    m_table_viewX = new QTableView();
    m_table_viewX->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
    m_table_viewX->setSelectionMode(QAbstractItemView::ContiguousSelection);// only one contiguous selection supported
    m_table_viewX->setModel(m_modelX);
    m_table_viewX->setEditTriggers(QAbstractItemView::DoubleClicked);
    m_table_viewX->setFocusPolicy(Qt::StrongFocus);
    //m_table_viewX->setFocus();

	// set header properties
	hHeader = (QHeaderView*)m_table_viewX->horizontalHeader();
	hHeader->setMovable(false);
	hHeader->setResizeMode(QHeaderView::Fixed);
	hHeader->setDefaultSectionSize(m_column_width);

    cols = numCols();
	for(int i=0; i<cols; i++)
		m_table_viewX->setColumnWidth(i, m_column_width);

	vHeader = (QHeaderView*)m_table_viewX->verticalHeader();
	vHeader->setMovable(false);
	vHeader->setResizeMode(QHeaderView::ResizeToContents);

    pal = m_table_viewX->palette();
	pal.setColor(QColorGroup::Base, m_bk_color);
	m_table_viewX->setPalette(pal);

//-------------------------- E Values View
    m_modelE = new MantidMatrixModel(this,ws,m_rows,m_cols,m_startRow,m_endRow,filter,maxv,MantidMatrixModel::E);

    m_table_viewE = new QTableView();
    m_table_viewE->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
    m_table_viewE->setSelectionMode(QAbstractItemView::ContiguousSelection);// only one contiguous selection supported
    m_table_viewE->setModel(m_modelE);
    m_table_viewE->setEditTriggers(QAbstractItemView::DoubleClicked);
    m_table_viewE->setFocusPolicy(Qt::StrongFocus);
    //m_table_viewE->setFocus();

	// set header properties
	hHeader = (QHeaderView*)m_table_viewE->horizontalHeader();
	hHeader->setMovable(false);
	hHeader->setResizeMode(QHeaderView::Fixed);
	hHeader->setDefaultSectionSize(m_column_width);

    cols = numCols();
	for(int i=0; i<cols; i++)
		m_table_viewE->setColumnWidth(i, m_column_width);

	vHeader = (QHeaderView*)m_table_viewE->verticalHeader();
	vHeader->setMovable(false);
	vHeader->setResizeMode(QHeaderView::ResizeToContents);

    pal = m_table_viewE->palette();
	pal.setColor(QColorGroup::Base, m_bk_color);
	m_table_viewE->setPalette(pal);

    m_tabs = new QTabWidget(this);
    m_tabs->insertTab(0,m_table_view,"Y values");
    m_tabs->insertTab(1,m_table_viewX,"X values");
    m_tabs->insertTab(2,m_table_viewE,"Errors");
    setWidget(m_tabs);

//--------------------------

    //setWidget(m_table_view);

    // recreate keyboard shortcut
	//d_select_all_shortcut = new QShortcut(QKeySequence(tr("Ctrl+A", "Matrix: select all")), this);
	//connect(d_select_all_shortcut, SIGNAL(activated()), d_table_view, SLOT(selectAll()));

    setGeometry(50, 50, QMIN(5, numCols())*m_table_view->horizontalHeader()->sectionSize(0) + 55,
                (QMIN(10,numRows())+1)*m_table_view->verticalHeader()->sectionSize(0)+100);

	setWindowTitle(name);
	setName(name);
	setIcon( QPixmap(matrixIcon()) );

    m_funct.init();
}

double MantidMatrix::cell(int row, int col)
{
	return m_model->data(row, col);
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
    m_table_view->horizontalHeader()->setDefaultSectionSize(m_column_width);
    m_table_viewX->horizontalHeader()->setDefaultSectionSize(m_column_width);

    int cols = numCols();
    for(int i=0; i<cols; i++)
    {
        m_table_view->setColumnWidth(i, width);
        m_table_viewX->setColumnWidth(i, width);
    }

	emit modifiedWindow(this);
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

MantidMatrix::~MantidMatrix()
{
    clean();
    delete m_model;
    delete m_modelX;
    delete m_modelE;
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

void MantidMatrix::showX()
{
    m_model->showX( !m_model->showX() );
    //m_table_view->repaint();
    repaint();
}

void MantidMatrix::canUpdateDAE(bool yes, int freq)
{
    if (!m_DAEname.empty()) m_canUpdateDAE = yes;
    if (yes)
    {
        if (freq > 0) m_updateFrequency = freq;
        if (m_updateFrequency == 0)
        {
            clean();
            return;
        }
        if (!m_updateDAEThread) m_updateDAEThread = new UpdateDAEThread(this,m_updateFrequency);
        m_updateDAEThread->start();
        m_spectrum_buff = new int[m_workspace->blocksize()+1];
    }
    else
    {
        clean();
    }
}

void MantidMatrix::clean()
{
    if (m_updateDAEThread)
    {
        m_updateDAEThread->terminate();
        m_updateDAEThread->wait();
        delete m_updateDAEThread;
        m_updateDAEThread = 0;
        delete[] m_spectrum_buff;
        m_spectrum_buff = 0;
    }
}

static double dblSqrt(double in)
{
  return sqrt(in);
}

void MantidMatrix::updateDAE()
{
    struct idc_info
    {
	    SOCKET s;
    };

    int ndims, dims_array[1];
    ndims = 1;
    int length = m_workspace->blocksize() + 1;
    dims_array[0] = length;

    idc_handle_t dae_handle;
      
    if (IDCopen(m_DAEname.c_str(), 0, 0, &dae_handle) != 0)
    {
        canUpdateDAE(false);
        QMessageBox::critical(0,"DAE error","Unable to open DAE " + QString::fromStdString(m_DAEname));
        return;
    }

    int hist = 0;
    int ispec = m_spectrum_min;

    if (m_spectrum_min > 0 && m_spectrum_max > 0)
    for(ispec = m_spectrum_min;ispec<=m_spectrum_max;ispec++)
    {
        // Read in spectrum number ispec from DAE
        IDCgetdat(dae_handle, ispec, 1, m_spectrum_buff, dims_array, &ndims);
        // Put it into a vector, discarding the 1st entry, which is rubbish
        // But note that the last (overflow) bin is kept
        std::vector<double> v(m_spectrum_buff + 1, m_spectrum_buff + length);
        // Create and fill another vector for the errors, containing sqrt(count)
        std::vector<double> e(length-1);
        std::transform(v.begin(), v.end(), e.begin(), dblSqrt);
        // Populate the workspace. Loop starts from 1, hence i-1
        //DataObjects::Workspace2D_sptr localWorkspace = 
        boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(m_workspace)->setData(hist, v, e);
        hist++;
    }

    for(size_t i=0;i<m_spectrum_list.size();i++)
    {
        ispec = m_spectrum_list[i];
        // Read in spectrum number ispec from DAE
        IDCgetdat(dae_handle, ispec, 1, m_spectrum_buff, dims_array, &ndims);
        // Put it into a vector, discarding the 1st entry, which is rubbish
        // But note that the last (overflow) bin is kept
        std::vector<double> v(m_spectrum_buff + 1, m_spectrum_buff + length);
        // Create and fill another vector for the errors, containing sqrt(count)
        std::vector<double> e(length-1);
        std::transform(v.begin(), v.end(), e.begin(), dblSqrt);
        // Populate the workspace. Loop starts from 1, hence i-1
        //DataObjects::Workspace2D_sptr localWorkspace = 
        boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(m_workspace)->setData(hist, v, e);
        hist++;
    }

    if (IDCclose(&dae_handle) != 0)
    {
        QMessageBox::critical(0,"DAE error","Problems with connection, updates will stop.");
        canUpdateDAE(false);
    }
    m_needDAERepaint = true;
    emit needsUpdating();
}

void MantidMatrix::tst()
{
    std::cerr<<"2D plots: "<<m_plots2D.size()<<'\n';
    std::cerr<<"1D plots: "<<m_plots1D.size()<<'\n';
}

void MantidMatrix::paintEvent(QPaintEvent *e)
{
    if ( m_needDAERepaint )
    {
        ((MantidMatrixModel*)m_table_view->model())->resetData();  
        ((MantidMatrixModel*)m_table_viewX->model())->resetData();  
        ((MantidMatrixModel*)m_table_viewE->model())->resetData();  
        m_needDAERepaint = false;
    }
    MdiSubWindow::paintEvent(e);
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
