#include "WorkspaceMatrixModel.h"
#include "WorkspaceMatrix.h"
#include "../plot2D/ScaleEngine.h"
#include "MantidAPI/Axis.h"
#include "MantidKernel/Exception.h"

#include <gsl/gsl_math.h>
#include <fstream>

WorkspaceMatrixModel::WorkspaceMatrixModel(Mantid::API::Workspace_sptr& ws, QObject *parent, int start, int end, bool filter, double maxv):
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

    m_start = (start<0 || start>=ws->getNumberHistograms())?0:start;
    m_end   = (end<0 || end>=ws->getNumberHistograms() || end < start)?ws->getNumberHistograms()-1:end;
    d_rows = m_end - m_start + 1;
	d_cols = ws->blocksize(); 
    m_filter = filter;
    m_maxValue = maxv;

}

double WorkspaceMatrixModel::cell(int row, int col) const
{
    if (!m_workspace || row >= rowCount() || col >= columnCount()) return 0.;
    double res = m_workspace->dataY(row + startRow())[col];

    if (m_filter)
    {
        if (res > m_maxValue) res =  m_maxValue;
        if (res < 0.) res =  0.;
    }
    
    return res;
}

double WorkspaceMatrixModel::dataX(int row, int col) const
{
    if (!m_workspace || row >= rowCount() || col >= columnCount()) return 0.;
    double res = m_workspace->dataX(row + startRow())[col];
    return res;

}

double WorkspaceMatrixModel::dataE(int row, int col) const
{
    if (!m_workspace || row >= rowCount() || col >= columnCount()) return 0.;
    double res = m_workspace->dataE(row + startRow())[col];
    if (res == 0.) res = 1.;//  quick fix of the fitting problem
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

void WorkspaceMatrixModel::setGraph2D(Graph* g)
{
    g->setTitle(tr("Workspace ")+d_matrix->name());
    Mantid::API::Axis* ax;
    try
    {
        ax = m_workspace->getAxis(0);
        std::string s;
        if (ax->unit().get()) s = ax->unit()->caption() + " / " + ax->unit()->label();
        else
            s = "X axis";
        g->setXAxisTitle(tr(s.c_str()));
        ax = m_workspace->getAxis(1);
        if (ax->isNumeric()) 
        {
            if (ax->unit().get()) s = ax->unit()->caption() + " / " + ax->unit()->label();
            else
                s = "Y axis";
            g->setYAxisTitle(tr(s.c_str())); 
        }
        else
            g->setYAxisTitle(tr("Spectrum")); 
    }
    catch(Mantid::Kernel::Exception::IndexError& e)
    {
        QMessageBox::critical(0,"WorkspaceMatrixModel error",e.what());
        g->setXAxisTitle(tr("X axis"));
        g->setYAxisTitle(tr("Y axis"));
    }
}

void WorkspaceMatrixModel::setGraph1D(Graph* g)
{
    g->setTitle(tr("Workspace ")+d_matrix->name());
    Mantid::API::Axis* ax;
    try
    {
        ax = m_workspace->getAxis(0);
        std::string s;
        if (ax->unit().get()) s = ax->unit()->caption() + " / " + ax->unit()->label();
        else
            s = "X axis";
        g->setXAxisTitle(tr(s.c_str()));
    }
    catch(Mantid::Kernel::Exception::IndexError& e)
    {
        QMessageBox::critical(0,"WorkspaceMatrixModel error",e.what());
        g->setXAxisTitle(tr("X axis"));
    }
    g->setYAxisTitle(tr("Counts")); 
}

int WorkspaceMatrixModel::indexX(double s)const
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

