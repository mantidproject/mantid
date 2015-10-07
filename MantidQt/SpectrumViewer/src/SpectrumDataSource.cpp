#include <iostream>

#include <math.h>

#include "MantidQtSpectrumViewer/SpectrumDataSource.h"

namespace MantidQt
{
namespace SpectrumView
{

/**
 *  Construct data source with specified total range and data size.
 *
 *  @param totalXMin  The smallest 'x' value covered by the data
 *  @param totalXMax  The largest 'x' value covered by the data
 *  @param totalYMin  The smallest 'y' value covered by the data
 *  @param totalYMax  The largest 'y' value covered by the data
 *  @param totalRows  The total number of rows the data is divided into
 *  @param totalCols  The total number of columns the data is divided into
 */
SpectrumDataSource::SpectrumDataSource( double totalXMin, double totalXMax,
                                        double totalYMin, double totalYMax,
                                        size_t totalRows, size_t totalCols ) :
  m_totalXMin(totalXMin), m_totalXMax(totalXMax),
  m_totalYMin(totalYMin), m_totalYMax(totalYMax),
  m_totalRows(totalRows), m_totalCols(totalCols)
{
}


SpectrumDataSource::~SpectrumDataSource()
{
}


/**
 * Get the smallest 'x' value covered by the data.
 */
double SpectrumDataSource::getXMin()
{
  return m_totalXMin;
}


/**
 * Get the largest 'x' value covered by the data.
 */
double SpectrumDataSource::getXMax()
{
  return m_totalXMax;
}


/**
 * Get the smallest 'y' value covered by the data.
 */
double SpectrumDataSource::getYMin()
{
  return m_totalYMin;
}


/**
 * Get the largest 'y' value covered by the data.
 */
double SpectrumDataSource::getYMax()
{
  return m_totalYMax;
}


/**
 * Get the total number of rows the data is divided into
 */
size_t SpectrumDataSource::getNRows()
{
  return m_totalRows;
}


/**
 * Get the total number of columns the data is divided into
 */
size_t SpectrumDataSource::getNCols()
{
  return m_totalCols;
}


/**
 * Clamp x to the interval of x-values covered by this DataSource
 * @param x   If x is more than xmax it will be set to xmax. If x is less
 *            than xmin, it will be set to xmin.
 */
void SpectrumDataSource::restrictX( double & x )
{
  if ( x > m_totalXMax )
    x = m_totalXMax;

  else if ( x < m_totalXMin )
    x = m_totalXMin;
}


/**
 * Clamp y to the interval of y-values covered by this DataSource.
 * @param y   If y is more than ymax it will be set to ymax. If y is less
 *            than ymin, it will be set to ymin.
 */
void SpectrumDataSource::restrictY( double & y )
{
  if ( y > m_totalYMax )
    y = m_totalYMax;

  else if ( y < m_totalYMin )
    y = m_totalYMin;
}


/**
 * Clamp row to a valid row number for this DataSource.
 * @param row  If row is more than n_rows-1, it is set to n_rows-1.  If
 *             row < 0 it is set to zero.
 */
void SpectrumDataSource::restrictRow( int & row )
{
  if ( row >= (int)m_totalRows )
    row = (int)m_totalRows - 1;

  else if ( row < 0 )
    row = 0;
}


/**
 * Clamp col to a valid column number for this DataSource.
 * @param col  If col is more than n_cols-1, it is set to n_cols-1.  If
 *             col < 0 it is set to zero.
 */
void SpectrumDataSource::restrictCol( int & col )
{
  if ( col >= (int)m_totalCols )
    col = (int)m_totalCols - 1;

  else if ( col < 0 )
    col = 0;
}


/**
 *  Convenience method to get all the data at the maximum resolution.
 */
DataArray_const_sptr SpectrumDataSource::getDataArray( bool isLogX )
{
  return getDataArray( m_totalXMin, m_totalXMax, m_totalYMin, m_totalYMax,
                       m_totalRows, m_totalCols, isLogX );
}

} // namespace SpectrumView
} // namespace MantidQt
