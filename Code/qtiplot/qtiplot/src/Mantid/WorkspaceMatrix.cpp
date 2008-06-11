#include "WorkspaceMatrix.h"
#include "../pixmaps.h"
#include "../ApplicationWindow.h"

#include <QMessageBox>

WorkspaceMatrix::WorkspaceMatrix(Mantid::API::Workspace_sptr ws, ScriptingEnv *env, const QString& label, ApplicationWindow* parent, const QString& name, Qt::WFlags f, int start, int end, bool filter, double maxv):
Matrix(env, label, parent, name, f)
{

    d_matrix_model = static_cast<MatrixModel*> (new WorkspaceMatrixModel(ws,this,start,end,filter,maxv));
    
    initGlobals();
	d_view_type = TableView;

    m_bk_color = QColor(128, 255, 128);
    m_matrix_icon = workspace_matrix_xpm;

    initTableView();

    double xs = ws->dataX(0)[0];
    double xe = ws->dataX(0)[ws->blocksize()];
    double ys = double(static_cast<WorkspaceMatrixModel*>(d_matrix_model)->startRow());
    double ye = double(static_cast<WorkspaceMatrixModel*>(d_matrix_model)->endRow());

    setCoordinates(xs,xe,ys,ye);

/*	// resize the table
	setGeometry(50, 50, QMIN(_Matrix_initial_columns_, d_matrix_model->columnCount())*d_table_view->horizontalHeader()->sectionSize(0) + 55,
                (QMIN(_Matrix_initial_rows_,d_matrix_model->rowCount())+1)*d_table_view->verticalHeader()->sectionSize(0));
*/
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