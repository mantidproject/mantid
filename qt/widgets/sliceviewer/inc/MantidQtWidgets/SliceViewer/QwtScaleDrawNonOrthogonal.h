#ifndef QWT_SCALE_DRAW_NON_ORTHOGONAL_H
#define QWT_SCALE_DRAW_NON_ORTHOGONAL_H

#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/VMD.h"
#include "MantidQtWidgets/SliceViewer/NonOrthogonalOverlay.h"
#include "qwt_plot.h"
#include "qwt_scale_draw.h"
#include <functional>

class QwtScaleDrawNonOrthogonal : public QwtScaleDraw {
public:
  enum class ScreenDimension { X, Y };

  QwtScaleDrawNonOrthogonal(
      QwtPlot *plot, ScreenDimension screenDimension,
      Mantid::API::IMDWorkspace_sptr workspace, size_t dimX, size_t dimY,
      Mantid::Kernel::VMD slicePoint,
      MantidQt::SliceViewer::NonOrthogonalOverlay *gridPlot);

  void draw(QPainter *painter, const QPalette &palette) const override;

  void drawLabelNonOrthogonal(QPainter *painter, double labelValue,
                              double labelPos) const;

  void updateSlicePoint(Mantid::Kernel::VMD newSlicepoint);

private:
  void setTransformationMatrices(Mantid::API::IMDWorkspace_sptr workspace);
  qreal getScreenBottomInXyz() const;
  qreal getScreenLeftInXyz() const;

  QPoint fromXyzToScreen(QPointF xyz) const;
  QPointF fromScreenToXyz(QPoint screen) const;
  QPointF fromMixedCoordinatesToHkl(double x, double y) const;
  double fromXtickInHklToXyz(double tick) const;
  double fromYtickInHklToXyz(double tick) const;

  Mantid::Kernel::VMD fromHklToXyz(const Mantid::Kernel::VMD &hkl) const;

  void applyGridLinesX(const QwtValueList &majorTicksXyz) const;
  void applyGridLinesY(const QwtValueList &majorTicksXyz) const;

  std::array<Mantid::coord_t, 9> m_fromHklToXyz;
  std::array<Mantid::coord_t, 9> m_fromXyzToHkl;

  // Non-owning pointer to the QwtPlot
  QwtPlot *m_plot;

  ScreenDimension m_screenDimension;

  // Non-orthogoanal information
  size_t m_dimX;
  size_t m_dimY;
  size_t m_missingDimension;
  Mantid::Kernel::VMD m_slicePoint;
  double m_angleX;
  double m_angleY;
  MantidQt::SliceViewer::NonOrthogonalOverlay *m_gridPlot;
};

#endif
