/**
 * File DataArray.cpp
 */

#include <iostream>
#include <math.h>

#include "MantidQtSpectrumViewer/DataArray.h"

namespace MantidQt
{
namespace SpectrumView
{

/**
 * Construct a DataArray "wrapper" around the data and region info.  The
 * actual data must be provided in a one-dimensional array, with n_rows*n_cols
 * entries.  The data corresponds to the "real" region [xMin,xMax]X[yMin,yMax].
 * Xmin must correspond to the left edge of the first column and xmax must
 * correspond to the right edge of the last column.  Simiarly, ymin must
 * correspond to the outer edge of the first row and ymax must correspond to
 * the outer edge of the last row.
 *
 * @param xMin       Left edge of data region
 * @param xMax       Right edge of data region
 * @param yMin       Bottom edge of data region
 * @param yMax       Top edge of data region
 * @param isLogX     Flag indication whether or not the data is binned
 *                   logarithmically in the 'x' direction.
 * @param nRows      Number of rows in the data array
 * @param nCols      Number of columns in the data array
 * @param data       Pointer to start of memory block holding the actual
 *                   data as a list of floats.
 */
DataArray::DataArray( double xMin,     double xMax,
                      double yMin,     double yMax,
                      bool   isLogX,
                      size_t nRows,    size_t nCols,
                      std::vector<float> data ):
  m_xMin(xMin), m_xMax(xMax),
  m_yMin(yMin), m_yMax(yMax),
  m_isLogX(isLogX),
  m_nRows(nRows), m_nCols(nCols),
  m_dataMin(data[0]), m_dataMax(data[0]),
  m_data(data)
{
  double value;
  size_t index = 0;

  for ( size_t row = 0; row < nRows; row++ )
  {
    for ( size_t col = 0; col < nCols; col++ )
    {
      value = data[index];

      if ( value < m_dataMin )
        m_dataMin = value;

      else if ( value > m_dataMax )
        m_dataMax = value;

      index++;
    }
  }
}

DataArray::~DataArray()
{
}

/**
 * Get the value corresponding to the left edge of the array.
 */
double DataArray::getXMin() const
{
  return m_xMin;
}

/**
 * Get the value corresponding to the right edge of the array.
 */
double DataArray::getXMax() const
{
  return m_xMax;
}

/**
 * Get the value corresponding to the bottom edge of the array (outer edge
 * of first row).
 */
double DataArray::getYMin() const
{
  return m_yMin;
}

/**
 * Get the value corresponding to the top edge of the array (outer edge
 * of last row).
 */
double DataArray::getYMax() const
{
  return m_yMax;
}

/**
 * Check if the returned array is binned logarithmically in 'x'.
 */
bool DataArray::isLogX() const
{
  return m_isLogX;
}

/**
 * Get smallest value recorded in this DataArray
 */
double DataArray::getDataMin() const
{
  return m_dataMin;
}


/**
 * Get largest value recorded in this DataArray
 */
double DataArray::getDataMax() const
{
  return m_dataMax;
}


/**
 * Get the actual number of rows in this DataArray
 *
 */
size_t DataArray::getNRows() const
{
  return m_nRows;
}


/**
 * Get the actual number of columns in this DataArray
 */
size_t DataArray::getNCols() const
{
  return m_nCols;
}


/**
 * Get the list of all values, packed in a 1-D array, in row-major order
 */
std::vector<float> DataArray::getData() const
{
  return m_data;
}


/**
 * Get the value at the specified row and column.  If the row or column
 * value is outside of the array, a value from the edge of the array
 * will be returned.  That is, the row and column numbers are "clamped"
 * to always lie in the range of valid values.
 *
 * @param row Row of data to get
 * @param col Columns of data to get
 * @returns Data at [row,col]
 */
double DataArray::getValue( int row, int col ) const
{
  if ( row < 0 )
    row = 0;

  if ( row > (int)m_nRows - 1 )
    row = (int)m_nRows - 1;

  if ( col < 0 )
    col = 0;

  if ( col > (int)m_nCols - 1 )
    col = (int)m_nCols - 1;

  return m_data[ row * m_nCols + col ];
}


/**
 * Get the value from the row and column containing the specified point.
 * If the specified point (x,y) is off the edge of the array, a value
 * from the edge of the array will be returned.
 *
 * @param x X value of data to get
 * @param y Y value of data to get
 * @returns Data at [x,y]
 */
double DataArray::getValue( double x, double y ) const
{
  size_t col = columnOfX( x );
  size_t row = rowOfY( y );

  return getValue( (int)row, (int)col );
}


/**
 * Clamp x to the interval of x-values covered by this DataArray.
 *
 * @param x If x is more than xmax it will be set to xmax. If x is less
 *          than xmin, it will be set to xmin.
 */
void DataArray::restrictX( double & x ) const
{
  if ( x > m_xMax )
    x = m_xMax;
  else if ( x < m_xMin )
    x = m_xMin;
}


/**
 * Clamp y to the interval of y-values covered by this DataArray.
 * @param y   If y is more than ymax it will be set to ymax. If y is less
 *            than ymin, it will be set to ymin.
 */
void DataArray::restrictY( double & y ) const
{
  if ( y > m_yMax )
    y = m_yMax;
  else if ( y < m_yMin )
    y = m_yMin;
}


/**
 * Clamp row to a valid row number for this DataArray.
 * @param row  If row is more than n_rows-1, it is set to n_rows-1.  If
 *             row < 0 it is set to zero.
 */
void DataArray::restrictRow( int & row ) const
{
  if ( row >= (int)m_nRows )
    row = (int)m_nRows - 1;
  else if ( row < 0 )
    row = 0;
}


/**
 * Clamp col to a valid column number for this DataArray.
 * @param col  If col is more than n_cols-1, it is set to n_cols-1.  If
 *             col < 0 it is set to zero.
 */
void DataArray::restrictCol( int & col ) const
{
  if ( col >= (int)m_nCols )
    col = (int)m_nCols - 1;
  else if ( col < 0 )
    col = 0;
}


/**
 * Calculate the column number containing the specified x value.  If the
 * specified value is less than xmin, 0 is returned.  If the specified
 * value is more than or equal to xmax, n_cols-1 is returned.  This
 * method uses the is_log_x flag to determine whether to use a "log"
 * transformation to map the column to x.
 *
 * @param x  The x value to map to a column number
 *
 * @return A valid column number, containing x, if x is in range, or the
 *         first or last column number if x is out of range.
 */
size_t DataArray::columnOfX( double x ) const
{
  int col;
  if ( m_isLogX )
    col = (int)((double)m_nCols * log(x / m_xMin) / log(m_xMax / m_xMin));
  else
    col = (int)((double)m_nCols * (x - m_xMin) / (m_xMax - m_xMin));

  restrictCol( col );
  return (size_t)col;
}


/*
 * Calculate the x-value at the center of the specified column.  If the
 * column number is too large, xmax is returned.  If the column number is
 * too small, xmin is returned.  This method uses the is_log_x flag to
 * determine whether to use a "log" tranformation to map the column to x.
 *
 * @param col  The column number to map to an x-value.
 *
 * @return A corresponding x value between xmin and xmax.
 */
double DataArray::xOfColumn( size_t col ) const
{
  double xVal;
  if ( m_isLogX )
    xVal = m_xMin * exp(((double)col + 0.5)/(double)m_nCols * log(m_xMax / m_xMin));
  else
    xVal = ((double)col + 0.5)/(double)m_nCols * (m_xMax - m_xMin) + m_xMin;

  restrictX( xVal );
  return xVal;
}


/**
 * Calculate the row number containing the specified y value.  If the
 * specified value is less than ymin, 0 is returned.  If the specified
 * value is more than or equal to ymax, n_rows-1 is returned.
 *
 * @param y  The y value to map to a row number
 *
 * @return A valid row number, containing y, if y is in range, or the
 *         first or last row number if y is out of range.
 */
size_t DataArray::rowOfY( double y ) const
{
  int row = (int)((double)m_nRows * (y - m_yMin) / (m_yMax - m_yMin));

  restrictRow( row );
  return (size_t)row;
}


/*
 * Calculate the y-value at the center of the specified row.  If the
 * row number is too large, ymax is returned.  If the row number is
 * too small, ymin is returned.
 *
 * @param row  The row number to map to an y-value.
 *
 * @return A corresponding y value between ymin and ymax.
 */
double DataArray::yOfRow( size_t row ) const
{
  double yVal;
  yVal = ((double)row + 0.5)/(double)m_nRows * (m_yMax - m_yMin) + m_yMin;

  restrictY( yVal );
  return yVal;
}


} // namespace SpectrumView
} // namespace MantidQt
