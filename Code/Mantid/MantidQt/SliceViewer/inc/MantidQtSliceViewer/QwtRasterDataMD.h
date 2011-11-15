#ifndef QwtRasterDataMD_H_
#define QwtRasterDataMD_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include <qwt_double_interval.h>
#include <qwt_raster_data.h>
#include <vector>

namespace MantidQt
{
namespace SliceViewer
{

/** Implemenation of QwtRasterData that can display the data
 * from a slice of an IMDWorkspace.
 *
 * @author Janik Zikovsky
 * @date Sep 29, 2011
 */

class QWT_EXPORT QwtRasterDataMD : public QwtRasterData
{
public:
  QwtRasterDataMD();
  virtual ~QwtRasterDataMD();
  QwtRasterData* copy() const;

  void setWorkspace(Mantid::API::IMDWorkspace_sptr ws);

  QwtDoubleInterval range() const;
  void setRange(const QwtDoubleInterval & range);

  void setSliceParams(size_t dimX, size_t dimY, std::vector<Mantid::coord_t> & slicePoint);

  double value(double x, double y) const;

  QSize rasterHint(const QwtDoubleRect &) const;

  mutable size_t timesRequested;

protected:
  /// Workspace being shown
  Mantid::API::IMDWorkspace_sptr m_ws;

  /// Number of dimensions
  size_t m_nd;

  /// Dimension index used as the X axis
  size_t m_dimX;

  /// Dimension index used as the Y axis
  size_t m_dimY;

  /// nd-sized array indicating where the slice is being done in the OTHER dimensions
  Mantid::coord_t * m_slicePoint;

  /// Min and Max values plotted.
  mutable double m_minVal;
  mutable double m_maxVal;

  /// Range of colors to plot
  QwtDoubleInterval m_range;

  /// Not a number
  double nan;
};

} // namespace SliceViewer
} // namespace Mantid

#endif /* QwtRasterDataMD_H_ */
