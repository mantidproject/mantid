#include "MantidMatrix.h"
#include "MantidUI.h"
//#include "Graph.h"
//#include "ApplicationWindow.h"

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

MantidMatrix::MantidMatrix(Mantid::API::Workspace_sptr ws, ApplicationWindow* parent, const QString& label, const QString& name, int start, int end, bool filter, double maxv)
: MdiSubWindow(label, parent, name, 0),m_workspace(ws)
{
    m_bk_color = QColor(128, 255, 255);
    m_matrix_icon = mantid_matrix_xpm;
    m_column_width = 100;
    m_model = new MantidMatrixModel(this,ws,start,end,filter,maxv);

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

    setWidget(m_table_view);
    // recreate keyboard shortcut
	//d_select_all_shortcut = new QShortcut(QKeySequence(tr("Ctrl+A", "Matrix: select all")), this);
	//connect(d_select_all_shortcut, SIGNAL(activated()), d_table_view, SLOT(selectAll()));

    setGeometry(50, 50, QMIN(5, numCols())*m_table_view->horizontalHeader()->sectionSize(0) + 55,
                (QMIN(10,numRows())+1)*m_table_view->verticalHeader()->sectionSize(0));

	setWindowTitle(name);
	setName(name);
	setIcon( QPixmap(matrixIcon()) );
}

int MantidMatrix::numRows()
{
    return m_model->rowCount();
}

int MantidMatrix::numCols()
{
    return m_model->columnCount();
}

double MantidMatrix::cell(int row, int col)
{
	return m_model->data(row, col);
}

QString MantidMatrix::text(int row, int col)
{
    return QString::number(m_model->data(row, col));
}

void MantidMatrix::setColumnsWidth(int width)
{
	if (m_column_width == width)
		return;

    m_column_width = width;
    m_table_view->horizontalHeader()->setDefaultSectionSize(m_column_width);

    int cols = numCols();
    for(int i=0; i<cols; i++)
        m_table_view->setColumnWidth(i, width);

	emit modifiedWindow(this);
}

void MantidMatrix::copySelection()
{
	QItemSelectionModel *selModel = m_table_view->selectionModel();
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

	m_table_view->scrollTo(m_model->index(row - 1, col - 1), QAbstractItemView::PositionAtTop);
}

void MantidMatrix::goToRow(int row)
{
	if(row < 1 || row > numRows())
		return;

	m_table_view->selectRow(row - 1);
	m_table_view->scrollTo(m_model->index(row - 1, 0), QAbstractItemView::PositionAtTop);
}

void MantidMatrix::goToColumn(int col)
{
	if(col < 1 || col > numCols())
		return;

	m_table_view->selectColumn(col - 1);
	m_table_view->scrollTo(m_model->index(0, col - 1), QAbstractItemView::PositionAtCenter);
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
	delete m_model;
}
