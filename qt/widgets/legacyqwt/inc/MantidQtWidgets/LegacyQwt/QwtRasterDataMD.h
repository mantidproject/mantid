// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef QwtRasterDataMD_H_
#define QwtRasterDataMD_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/SpectraDetectorTypes.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidQtWidgets/LegacyQwt/DllOption.h"

#include <qwt_double_interval.h>
#include <qwt_raster_data.h>

#include <vector>

namespace MantidQt {
namespace API {

/** Implemenation of QwtRasterData that can display the data
 * from a slice of an IMDWorkspace.
 *
 * This can be used by QwtPlotSpectrogram's to plot 2D data.
 * It is used by the SliceViewer GUI.
 *
 * @author Janik Zikovsky
 * @date Sep 29, 2011
 */

class EXPORT_OPT_MANTIDQT_LEGACYQWT QwtRasterDataMD : public QwtRasterData {
public:
  QwtRasterDataMD();
  ~QwtRasterDataMD() override;
  QwtRasterDataMD *copy() const override;

  virtual void setWorkspace(Mantid::API::IMDWorkspace_const_sptr ws);
  Mantid::API::IMDWorkspace_const_sptr getWorkspace() const;

  void setOverlayWorkspace(Mantid::API::IMDWorkspace_const_sptr ws);

  QwtDoubleInterval range() const override;
  void setRange(const QwtDoubleInterval &range);

  virtual void setSliceParams(size_t dimX, size_t dimY,
                              Mantid::Geometry::IMDDimension_const_sptr X,
                              Mantid::Geometry::IMDDimension_const_sptr Y,
                              std::vector<Mantid::coord_t> &slicePoint);

  double value(double x, double y) const override;

  QSize rasterHint(const QwtDoubleRect &) const override;

  void setFastMode(bool fast);

  void setZerosAsNan(bool val);

  bool isZerosAsNan() const;

  void setNormalization(Mantid::API::MDNormalization normalization);
  Mantid::API::MDNormalization getNormalization() const;

  void transferSettingsTo(QwtRasterDataMD *dest) const;

protected:
  void copyFrom(const QwtRasterDataMD &source, QwtRasterDataMD &dest) const;

  /// Workspace being shown
  Mantid::API::IMDWorkspace_const_sptr m_ws;

  /// Workspace overlaid on top of original (optional)
  Mantid::API::IMDWorkspace_const_sptr m_overlayWS;

  /// Number of dimensions in the workspace
  size_t m_nd;

  /// Dimension index used as the X axis
  size_t m_dimX;

  /// Dimension index used as the Y axis
  size_t m_dimY;

  /// The X dimensions of the workspace (with the estimated bin resolution)
  Mantid::Geometry::IMDDimension_const_sptr m_X;

  /// The Y dimensions of the workspace (with the estimated bin resolution)
  Mantid::Geometry::IMDDimension_const_sptr m_Y;

  /// nd-sized array indicating where the slice is being done in the OTHER
  /// dimensions
  Mantid::coord_t *m_slicePoint;

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

} // namespace API
} // namespace MantidQt

#endif /* QwtRasterDataMD_H_ */
