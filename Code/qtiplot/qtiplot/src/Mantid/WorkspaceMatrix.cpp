#include "WorkspaceMatrix.h"
#include "../pixmaps.h"
#include "../ApplicationWindow.h"
#include "../Graph3D.h"

#include <QApplication>
#include <QMessageBox>

WorkspaceMatrix::WorkspaceMatrix(Mantid::API::Workspace_sptr ws, ScriptingEnv *env, const QString& label, ApplicationWindow* parent, const QString& name, Qt::WFlags f, int start, int end, bool filter, double maxv):
Matrix(env, label, parent, name, f),m_funct(this)
{

    d_matrix_model = static_cast<MatrixModel*> (new WorkspaceMatrixModel(ws,this,start,end,filter,maxv));
    
    initGlobals();
	d_view_type = TableView;

    m_bk_color = QColor(128, 255, 128);
    m_matrix_icon = workspace_matrix_xpm;

    initTableView();

    double xs = ws->dataX(0)[0];
    double xe = ws->dataX(0)[ws->blocksize()];
    // What if y is not a spectrum number?
    double ys = double(static_cast<WorkspaceMatrixModel*>(d_matrix_model)->startRow());
    double ye = double(static_cast<WorkspaceMatrixModel*>(d_matrix_model)->endRow());

    setCoordinates(xs,xe,ys,ye);

    m_funct.init();

}

Graph3D * WorkspaceMatrix::plotGraph3D(int style)
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
	
    Mantid::API::Axis* ax = wsModel()->workspace().getAxis(0);
    std::string s = ax->unit()->caption() + " / " + ax->unit()->label();
    plot->setXAxisLabel(tr(s.c_str()));
    
    ax = wsModel()->workspace().getAxis(1);
    if (ax->isNumeric()) 
    {
        s = ax->unit()->caption() + " / " + ax->unit()->label();
        plot->setYAxisLabel(tr(s.c_str())); 
    }
    else
        plot->setYAxisLabel(tr("Spectrum")); 

    plot->setZAxisLabel(tr("Counts")); 

    a->initPlot3D(plot);
	QApplication::restoreOverrideCursor();

    return plot;
}

WorkspaceMatrix::~WorkspaceMatrix()
{
} 

void WorkspaceMatrix::getSelectedRows(int& i0,int& i1)
{
    i0 = i1 = -1;
	QItemSelectionModel *selModel = d_table_view->selectionModel();
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

Table* WorkspaceMatrix::createTableFromSelectedRows(bool visible, bool errs)
{
     //Matrix* m = (Matrix*)activeWindow(MatrixWindow);
     //if (!m || !m->isA("WorkspaceMatrix")) return 0;
     //WorkspaceMatrix *wsm = static_cast<WorkspaceMatrix*>(m);

     int i0,i1; 
     getSelectedRows(i0,i1);
     if (i0 < 0 || i1 < 0) return 0;

     int c = errs?2:1;

     //Table *t = newTable(generateUniqueName(wsm->name()+"-"),wsm->numCols(),c*(i1 - i0 + 1) + 1);

	 Table* t = new Table(scriptEnv, numCols(), c*(i1 - i0 + 1) + 1, "", applicationWindow(), 0);
	 applicationWindow()->initTable(t, applicationWindow()->generateUniqueName(name()+"-"));
     if (visible) t->showNormal();

    
     int kY,kErr;
     for(int i=i0;i<=i1;i++)
     {
         kY = c*(i-i0)+1;
         t->setColName(kY,"Y"+QString::number(i));
         if (errs)
         {
             kErr = 2*(i - i0) + 2;
             t->setColPlotDesignation(kErr,Table::yErr);
             t->setColName(kErr,"Err"+QString::number(i));
         }
         for(int j=0;j<numCols();j++)
         {
             if (i == i0) t->setCell(j,0,dataX(i,j));
             t->setCell(j,kY,cell(i,j)); 
             if (errs) t->setCell(j,kErr,dataE(i,j));
         }
     }
     return t;
 }

void WorkspaceMatrix::createGraphFromSelectedRows(bool visible, bool errs)
{
    Table *t = createTableFromSelectedRows(visible,errs);
    if (!t) return;

    QStringList cn;
    cn<<t->colName(1);
    if (errs) cn<<t->colName(2);
    Graph *g = applicationWindow()->multilayerPlot(t,t->colNames(),Graph::Line)->activeGraph();
    applicationWindow()->polishGraph(g,Graph::Line);
    setGraph1D(g);
}

void WorkspaceMatrixFunction::init()
{
    int nx = m_wsmatrix->numCols();
    int ny = m_wsmatrix->numRows();

    m_dx = (m_wsmatrix->xEnd() - m_wsmatrix->xStart()) / (nx > 1? nx - 1 : 1);
    m_dy = (m_wsmatrix->yEnd() - m_wsmatrix->yStart()) / (ny > 1? ny - 1 : 1);

    if (m_dx == 0.) m_dx = 1.;//?
    if (m_dy == 0.) m_dy = 1.;//?
}

double WorkspaceMatrixFunction::operator()(double x, double y)
{
    x += 0.5*m_dx;
    y -= 0.5*m_dy;
    	
    int i = abs((y - m_wsmatrix->yStart())/m_dy);
    int j = abs((x - m_wsmatrix->xStart())/m_dx);

    int jj = static_cast<WorkspaceMatrixModel*>(m_wsmatrix->matrixModel())->indexX(x);
    if (jj >= 0) j = jj;

    if (i >= 0 && i < m_wsmatrix->numRows() && j >=0 && j < m_wsmatrix->numCols())
	    return m_wsmatrix->cell(i,j);
    else
	    return 0.0;
}
