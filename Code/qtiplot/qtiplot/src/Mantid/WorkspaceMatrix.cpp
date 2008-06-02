#include "WorkspaceMatrix.h"
#include "../pixmaps.h"

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
