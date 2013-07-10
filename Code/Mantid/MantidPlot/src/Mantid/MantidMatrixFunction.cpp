#include "MantidMatrixFunction.h"

#include "MantidAPI/Axis.h"

MantidMatrixFunction::MantidMatrixFunction(MantidMatrix &matrix):
    m_outside(0)
{

  init(matrix.workspace());

  double tmp;
  matrix.range( &tmp, &m_outside );
  m_outside *= 1.1;

  m_observer = new MantidMatrixFunctionWorkspaceObserver(this);
  m_observer->observeADSClear();
  m_observer->observePreDelete();
  m_observer->observeAfterReplace();
}

MantidMatrixFunction::~MantidMatrixFunction()
{
    //std::cerr << "MantidMatrixFunction deleted." << std::endl;
}

/**
 * Initialize the function from a matrix workspace.
 *
 * @param workspace :: New workspace to use.
 */
void MantidMatrixFunction::init(const Mantid::API::MatrixWorkspace_const_sptr& workspace)
{
    m_workspace = workspace;

    if (!m_workspace->getAxis(1))
    {
      throw std::runtime_error("The y-axis is not set");
    }

    setMesh(m_workspace->blocksize(),m_workspace->getNumberHistograms());

}

/**
 * Reset the underlying matrix workspace and the mesh dimensions.
 *
 * @param workspace :: New workspace to use.
 */
void MantidMatrixFunction::reset(const Mantid::API::MatrixWorkspace_const_sptr &workspace)
{
    init( workspace );
    double minz, maxz;
    findYRange( workspace, minz, maxz );
    setMinZ( minz );
    setMaxZ( maxz );
    setMesh(workspace->blocksize(),workspace->getNumberHistograms());
    double minx,maxx;
    workspace->getXMinMax( minx, maxx );
    const Mantid::API::Axis *axis = workspace->getAxis(1);
    double miny = axis->getMin();
    double maxy = axis->getMax();
    setDomain(minx,maxx,miny,maxy);
    create();
}

double MantidMatrixFunction::operator()(double x, double y)
{
  size_t i = indexY(y);
  if ( i >= rows() )
  {
    return m_outside;
  }

  size_t j = indexX(i,x);

  if ( j < columns() )
      return m_workspace->readY(i)[j];
  else
    return m_outside;
}

double MantidMatrixFunction::getMinPositiveValue()const
{
  double zmin = DBL_MAX;
  for(size_t i = 0; i < rows(); ++i)
  {
    for(size_t j = 0; j < columns(); ++j)
    {
      double tmp = value(i,j);
      if (tmp > 0 && tmp < zmin)
      {
        zmin = tmp;
      }
    }
  }
  return zmin;
}

QString MantidMatrixFunction::saveToString() const
{
    return "mantidMatrix3D\t";
}

/**
 * Connect to a viewer object to ask it to redraw when needed.
 *
 * @param viewer :: An object displaying this function. It must have slot update().
 */
void MantidMatrixFunction::connectToViewer(QObject *viewer)
{
    m_observer->connect(m_observer,SIGNAL(requestRedraw()),viewer,SLOT(update()));
    m_observer->connect(m_observer,SIGNAL(requestClose()),viewer,SLOT(close()));
}

double MantidMatrixFunction::value(size_t row,size_t col)const
{
  return m_workspace->readY(row)[col];
}

void MantidMatrixFunction::getRowYRange(size_t row,double& ymin, double& ymax)const
{
  const Mantid::API::Axis& yAxis = *(m_workspace->getAxis(1));


  size_t i = row;
  double y = yAxis(i);

  size_t imax = static_cast<int>(m_workspace->getNumberHistograms())-1;
  if (yAxis.isNumeric())
  {
    if (i < imax)
    {
      ymax = (yAxis(i+1) + y)/2;
      if (i > 0)
      {
        ymin = (yAxis(i-1) + y)/2;
      }
      else
      {
        ymin = 2*y - ymax;
      }
    }
    else
    {
      ymin = (yAxis(i-1) + y)/2;
      ymax = 2*y - ymin;
    }
  }
  else // if spectra
  {
    ymin = y - 0.5;
    ymax = y + 0.5;
  }
  
}

void MantidMatrixFunction::getRowXRange(int row,double& xmin, double& xmax)const
{
  const Mantid::MantidVec& X = m_workspace->readX(row);
  xmin = X[0];
  xmax = X[X.size()-1];
}

const Mantid::MantidVec& MantidMatrixFunction::getMantidVec(int row)const
{
    return m_workspace->readX(row);
}

size_t MantidMatrixFunction::indexX(size_t row,double s)const
{
  size_t n = m_workspace->blocksize();

  const Mantid::MantidVec& X = m_workspace->readX(row);
  if (n == 0 || s < X[0] || s > X[n-1]) return std::numeric_limits<size_t>::max();

  size_t i = 0, j = n-1, k = n/2;
  double ss;
  for(size_t it = 0; it < n; it++)
  {
    ss = X[k];
    if (ss == s ) return k;
    if (abs(static_cast<int>(i) - static_cast<int>(j)) <2)
    {
      double ds = fabs(ss-s);
      if (fabs(X[j]-s) < ds) return j;
      return i;
    }
    if (s > ss) i = k;
    else
      j = k;
    k = i + (j - i)/2;
  }

  return i;
}

size_t MantidMatrixFunction::indexY(double s)const
{
  size_t n = rows();

  const Mantid::API::Axis& yAxis = *m_workspace->getAxis(1);

  bool isNumeric = yAxis.isNumeric();

  if (n == 0) return std::numeric_limits<size_t>::max();

  size_t i0 = 0;

  if (s < yAxis(i0))
  {
    if (isNumeric || yAxis(i0) - s > 0.5) return std::numeric_limits<size_t>::max();
    return 0;
  }
  else if (s > yAxis(n-1))
  {
    if (isNumeric || s - yAxis(n-1) > 0.5) return std::numeric_limits<size_t>::max();
    return n-1;
  }

  size_t i = i0, j = n-1, k = n/2;
  double ss;
  for(size_t it = 0; it < n; it++)
  {
    ss = yAxis(k);
    if (ss == s ) return k;
    if (abs(static_cast<int>(i) - static_cast<int>(j)) <2)
    {
      double ds = fabs(ss-s);
      double ds1 = fabs(yAxis(j)-s);
      if (ds1 < ds)
      {
        if (isNumeric || ds1 < 0.5) return j;
        return std::numeric_limits<size_t>::max();
      }
      if (isNumeric || ds < 0.5) return i;
      return std::numeric_limits<size_t>::max();
    }
    if (s > ss) i = k;
    else
      j = k;
    k = i + (j - i)/2;
  }

  return i;
}

/*--------------------------------------------------------------------------------------------*/

MantidMatrixFunctionWorkspaceObserver::MantidMatrixFunctionWorkspaceObserver(MantidMatrixFunction *fun):
    m_function(fun)
{
}

void MantidMatrixFunctionWorkspaceObserver::afterReplaceHandle(const std::string &wsName, const boost::shared_ptr<Mantid::API::Workspace> ws)
{
    if ( m_function->m_workspace && wsName == m_function->m_workspace->name() )
    {
        auto mws = boost::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(ws);
        if ( mws )
        {
            m_function->reset( mws );
            emit requestRedraw();
        }
        else
        {
            emit requestClose();
        }
    }
}

void MantidMatrixFunctionWorkspaceObserver::preDeleteHandle(const std::string &wsName, const boost::shared_ptr<Mantid::API::Workspace>)
{
    if ( m_function->m_workspace && wsName == m_function->m_workspace->name() )
    {
        emit requestClose();
    }
}

void MantidMatrixFunctionWorkspaceObserver::clearADSHandle()
{
    emit requestClose();
}
