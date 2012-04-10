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
 * This can be used by QwtPlotSpectrogram's to plot 2D data.
 * It is used by the SliceViewer GUI.
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

  void setOverlayWorkspace(Mantid::API::IMDWorkspace_sptr ws);

  QwtDoubleInterval range() const;
  void setRange(const QwtDoubleInterval & range);

  void setSliceParams(size_t dimX, size_t dimY, std::vector<Mantid::coord_t> & slicePoint);

  double value(double x, double y) const;

  QSize rasterHint(const QwtDoubleRect &) const;

  void setFastMode(bool fast);

  void setZerosAsNan(bool val);

  void setNormalization(Mantid::API::MDNormalization normalization);
  Mantid::API::MDNormalization getNormalization() const;

protected:
  /// Workspace being shown
  Mantid::API::IMDWorkspace_sptr m_ws;

  /// Workspace overlaid on top of original (optional)
  Mantid::API::IMDWorkspace_sptr m_overlayWS;

  /// Number of dimensions in the workspace
  size_t m_nd;

  /// Dimension index used as the X axis
  size_t m_dimX;

  /// Dimension index used as the Y axis
  size_t m_dimY;

  /// nd-sized array indicating where the slice is being done in the OTHER dimensions
  Mantid::coord_t * m_slicePoint;

  /// Range of colors to plot
  QwtDoubleInterval m_range;

  /// Edges of the overlay workspace, in the X
  double m_overlayXMin;
  double m_overlayXMax;
  double m_overlayYMin;
  double m_overlayYMax;

  /// Boolean, set to true when the overlay workspace is visible
  /// given the current slice point
  bool m_overlayInSlice;

  /// Not a number
  double nan;

  /// When true, renders the view as quickly as the workspace resolution allows
  /// when false, renders one point per pixel
  bool m_fast;

  /// Convert zeroes to NAN
  bool m_zerosAsNan;

  /// Normalization of signals
  Mantid::API::MDNormalization m_normalization;
};

} // namespace SliceViewer
} // namespace Mantid

#endif /* QwtRasterDataMD_H_ */
