#ifndef QWTRASTERDATATESTER_H_
#define QWTRASTERDATATESTER_H_

#include <qwt_raster_data.h>
#include <qwt_double_interval.h>

/** Tester class for QwtRaster data giving fake data
 *
 * @author Janik Zikovsky
 * @date Sep 29, 2011
 */

class QWT_EXPORT QwtRasterDataTester : public QwtRasterData
{
public:
  QwtRasterDataTester();
  QwtRasterDataTester(size_t nx, size_t ny);
  virtual ~QwtRasterDataTester();

  double value(double x, double y) const;

  QwtRasterData* copy() const;

  QwtDoubleInterval range() const;

protected:
  size_t m_nx;
  size_t m_ny;
  double * m_data;
};

#endif /* QWTRASTERDATATESTER_H_ */
