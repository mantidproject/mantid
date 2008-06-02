#ifndef WORKSPACEMATRIX_H
#define WORKSPACEMATRIX_H

#include "../Matrix.h"
#include "../Graph.h"
#include "WorkspaceMatrixModel.h"
#include "MantidAPI/Workspace.h"

class WorkspaceMatrix : public Matrix
{
	Q_OBJECT
	
public:

    WorkspaceMatrix(Mantid::API::Workspace_sptr ws,
                    ScriptingEnv *env, 
                    const QString& label, 
                    ApplicationWindow* parent, 
                    const QString& name, 
                    Qt::WFlags f=0,
                    int start=-1, int end=-1, bool filter=false, double maxv=0.);
	~WorkspaceMatrix();
    void setGraph(Graph* g){if (d_matrix_model) static_cast<WorkspaceMatrixModel*>(d_matrix_model)->setGraph(g);}
    void getSelectedRows(int& i0,int& i1);
    double dataX(int row, int col){return static_cast<WorkspaceMatrixModel*>(d_matrix_model)->dataX(row, col);}
    double dataE(int row, int col){return static_cast<WorkspaceMatrixModel*>(d_matrix_model)->dataE(row, col);}

};

#endif /* WORKSPACEMATRIX_H */
