#include "WorkspaceMatrixModel.h"
#include "WorkspaceMatrix.h"

#include <gsl/gsl_math.h>

WorkspaceMatrixModel::WorkspaceMatrixModel(Mantid::API::Workspace_sptr& ws, QObject *parent, int start, int end):
MatrixModel(parent)
{

    m_workspace = ws;

    if (!m_workspace)
    {
        QMessageBox::critical(0,"WorkspaceMatrixModel error","2D workspace expected.");
        d_rows = 0;
    	d_cols = 0; 
        m_start = 0;
        m_end = 0;
        return;
    }

    m_start = (start<0 || start>=ws->getHistogramNumber())?0:start;
    m_end   = (end<0 || end>=ws->getHistogramNumber() || end < start)?ws->getHistogramNumber()-1:end;
    d_rows = m_end - m_start + 1;
	d_cols = ws->blocksize(); 

}

double WorkspaceMatrixModel::cell(int row, int col) const
{
    if (!m_workspace || row >= rowCount() || col >= columnCount()) return 0.;
    double res = m_workspace->dataY(row + startRow())[col];

    return abs(res)>10000.?0.:res;
    //return res;

}

double WorkspaceMatrixModel::dataX(int row, int col) const
{
    if (!m_workspace || row >= rowCount() || col >= columnCount()) return 0.;
    double res = m_workspace->dataX(row + startRow())[col];
    return res;

}

QVariant WorkspaceMatrixModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
		return QVariant();

	double val = cell(index.row(),index.column());
    if (gsl_isnan (val))
        return QVariant();

	if (role == Qt::DisplayRole || role == Qt::EditRole){
		if (d_matrix)
			return QVariant(d_matrix->locale().toString(val, d_matrix->textFormat().toAscii(), d_matrix->precision()));
		else
			return QVariant(d_locale.toString(val, d_txt_format, d_num_precision));
	} else
		return QVariant();
}

void WorkspaceMatrixModel::setGraph(Graph* g)
{
    g->setTitle(tr("Workspace ")+d_matrix->name());
    g->setXAxisTitle(tr("Time of flight"));
    //g->setScale(Graph::Bottom,1,2000);
    g->setYAxisTitle(tr("Histogram"));
}

