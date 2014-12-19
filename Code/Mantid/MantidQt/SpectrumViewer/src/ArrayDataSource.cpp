#include <iostream>

#include <math.h>

#include "MantidQtSpectrumViewer/ArrayDataSource.h"
#include "MantidQtSpectrumViewer/SVUtils.h"

namespace MantidQt
{
namespace SpectrumView
{

/**
 * Construct a DataSource object to to display data from the specified
 * array.  This class takes ownership of the data and will delete it
 * when this class is deleted.  The calling code should not change the
 * values in the array or delete it!
 *
 * @param m_totalXMin   The x-coordinate at the left edge of the first column
 *                     of data.
 * @param m_totalXMax   The x-coordinate at the right edge of the last column
 *                     of data.
 * @param m_totalYMin   The y-coordinate at the bottom edge of bottom row of
 *                     the data region.
 * @param m_totalYMax   The y-coordinate at the top edge of the top row of
 *                     the data region
 * @param m_totalRows   The number of rows the data values are divided into.
 * @param m_totalCols   The number of columns the test values are divided
 *                     into.
 * @param data         The list of floats holding the data to be displayed,
 *                     stored in row major order.
 */
ArrayDataSource::ArrayDataSource( double m_totalXMin, double m_totalXMax,
                                  double m_totalYMin, double m_totalYMax,
                                  size_t m_totalRows, size_t m_totalCols,
                                  std::vector<float> data ) :
  SpectrumDataSource( m_totalXMin, m_totalXMax,
                      m_totalYMin, m_totalYMax,
                      m_totalRows, m_totalCols ),
  m_data(data)
{
}


ArrayDataSource::~ArrayDataSource()
{
}

bool ArrayDataSource::hasData(const std::string& wsName,
                              const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  UNUSED_ARG(wsName);
  UNUSED_ARG(ws);
  return false;
}

/**
 * Get a data array covering the specified range of data, at the specified
 * resolution.  NOTE: The calling code is responsible for deleting the
 * DataArray that is returned, when it is no longer needed.
 *
 * @param xMin      Left edge of region to be covered.
 * @param xMax      Right edge of region to be covered.
 * @param yMin      Bottom edge of region to be covered.
 * @param yMax      Top edge of region to be covered.
 * @param nRows    Number of rows to return. If the number of rows is less
 *                  than the actual number of data rows in [yMin,yMax], the
 *                  data will be subsampled, and only the specified number
 *                  of rows will be returned.
 * @param nCols    The event data will be rebinned using the specified
 *                  number of colums.
 * @param isLogX  Flag indicating whether or not the data should be
 *                  binned logarithmically in the X-direction.  This
 *                  DataSource does not support rebinning to a log axis, so
 *                  the DataArray is always returned with isLogX = false.
 */
DataArray_const_sptr ArrayDataSource::getDataArray( double xMin,   double  xMax,
                                                    double yMin,   double  yMax,
                                                    size_t nRows,  size_t  nCols,
                                                    bool   isLogX )
{
  size_t firstCol;
  SVUtils::CalculateInterval( m_totalXMin, m_totalXMax, m_totalCols,
                              firstCol, xMin, xMax, nCols );

  size_t firstRow;
  SVUtils::CalculateInterval( m_totalYMin, m_totalYMax, m_totalRows,
                              firstRow, yMin, yMax, nRows );

  std::vector<float> newData(nRows * nCols);

  double xStep = (xMax - xMin) / (double)nCols;
  double yStep = (yMax - yMin) / (double)nRows;

  double xIndex;
  double yIndex;
  size_t index = 0;

  /* Get data for middle of */
  /* each destination position */
  for ( size_t row = 0; row < nRows; row++ )
  {
    double midY = yMin + ((double)row + 0.5) * yStep;
    SVUtils::Interpolate( m_totalYMin, m_totalYMax,         midY,
                          0.0,         (double)m_totalRows, yIndex );

    size_t sourceRow = (size_t)yIndex;
    for ( size_t col = 0; col < nCols; col++ )
    {
      double midX = xMin + ((double)col + 0.5) * xStep;
      SVUtils::Interpolate( m_totalXMin, m_totalXMax,         midX,
                            0.0,         (double)m_totalCols, xIndex );

      size_t sourceCol = (size_t)xIndex;

      newData[index] = m_data[sourceRow * m_totalCols + sourceCol];
      index++;
    }
  }

  // The calling code is responsible for deleting the DataArray
  isLogX = false;
  DataArray_const_sptr newDataArray( new DataArray( xMin, xMax, yMin, yMax,
                                           isLogX, nRows, nCols, newData) );
  return newDataArray;
}


/**
 * Get a data array covering the full range of data.
 * NOTE: The calling code is responsible for deleting the DataArray that is
 * returned, when it is no longer needed.
 *
 * @param isLogX  Flag indicating whether or not the data should be
 *                binned logarithmically.  This DataSource does not
 *                support rebinning to a log axis, so the DataArray is
 *                always returned with isLogX = false.
 */
DataArray_const_sptr ArrayDataSource::getDataArray( bool isLogX )
{
  isLogX = false;
  return getDataArray( m_totalXMin, m_totalXMax, m_totalYMin, m_totalYMax,
                       m_totalRows, m_totalCols, isLogX );
}


/**
 * Clear the vector of strings and then add pairs of strings giving information
 * about the specified point, x, y.  The first string in a pair should
 * generally be a string describing the value being presented and the second
 * string should contain the value.
 *
 * @param x    The x-coordinate of the point of interest in the data.
 * @param y    The y-coordinate of the point of interest in the data.
 * @param list Vector that will be filled out with the information strings.
 */
void ArrayDataSource::getInfoList( double x,
                                   double y,
                                   std::vector<std::string> &list )
{
  list.clear();

  SVUtils::PushNameValue( "X", 8, 3, x, list );
  SVUtils::PushNameValue( "Y", 8, 3, y, list );
}

} // namespace SpectrumView
} // namespace MantidQt
