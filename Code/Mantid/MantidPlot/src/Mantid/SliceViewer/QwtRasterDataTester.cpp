#include "QwtRasterDataTester.h"
#include <math.h>

QwtRasterDataTester::QwtRasterDataTester()
: m_data(NULL)
{
}


QwtRasterDataTester::QwtRasterDataTester(size_t nx, size_t ny)
: m_nx(nx), m_ny(ny)
{
  m_data = new double[nx*ny];
}

QwtRasterDataTester::~QwtRasterDataTester()
{
  delete [] m_data;
}


double QwtRasterDataTester::value(double x, double y) const
{
  return (sin(x*30) + sin(y*30)) * 5;
}

QwtRasterData* QwtRasterDataTester::copy() const
{
  return new QwtRasterDataTester(1,1);
}

QwtDoubleInterval QwtRasterDataTester::range() const
{
  return QwtDoubleInterval(0.0, 10.0, 0);
}
