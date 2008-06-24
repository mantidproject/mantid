#ifndef WORKSPACEMATRIX_H
#define WORKSPACEMATRIX_H

#include "../Matrix.h"
#include "../Graph.h"
#include "../Graph3D.h"
#include "../Table.h"
#include "WorkspaceMatrixModel.h"
#include "MantidAPI/Workspace.h"

class WorkspaceMatrix;

class WorkspaceMatrixFunction: public UserHelperFunction
{
public:
    WorkspaceMatrixFunction(WorkspaceMatrix* wsm):m_wsmatrix(wsm){}
    double operator()(double x, double y);
    void init();
private:
    WorkspaceMatrix* m_wsmatrix;
    double m_dx,m_dy;
};

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
    void setGraph2D(Graph* g){if (d_matrix_model) static_cast<WorkspaceMatrixModel*>(d_matrix_model)->setGraph2D(g);}
    void setGraph1D(Graph* g){if (d_matrix_model) static_cast<WorkspaceMatrixModel*>(d_matrix_model)->setGraph1D(g);}
    void getSelectedRows(int& i0,int& i1);
    double dataX(int row, int col){return static_cast<WorkspaceMatrixModel*>(d_matrix_model)->dataX(row, col);}
    double dataE(int row, int col){return static_cast<WorkspaceMatrixModel*>(d_matrix_model)->dataE(row, col);}
    Table* createTableFromSelectedRows(bool vis = true, bool errs = true);
    void createGraphFromSelectedRows(bool vis = true, bool errs = true);
    Graph3D * WorkspaceMatrix::plotGraph3D(int style);
    WorkspaceMatrixModel* wsModel(){return static_cast<WorkspaceMatrixModel*>(d_matrix_model);}
private:
     WorkspaceMatrixFunction m_funct;
};

#endif /* WORKSPACEMATRIX_H */
