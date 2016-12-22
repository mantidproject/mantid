#ifndef QWT_SCALE_DRAW_NON_ORTHOGONAL_H
#define QWT_SCALE_DRAW_NON_ORTHOGONAL_H

#include "MantidQtAPI/DllOption.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/VMD.h"
#include "MantidAPI/IMDWorkspace.h"
#include "qwt_scale_draw.h"
#include "qwt_plot.h"
#include <functional>


class EXPORT_OPT_MANTIDQT_API QwtScaleDrawNonOrthogonal: public QwtScaleDraw
{
public:
  enum class ScreenDimension {X, Y};

  QwtScaleDrawNonOrthogonal(QwtPlot* plot, ScreenDimension screenDimension, Mantid::API::IMDWorkspace_sptr workspace, size_t dimX,
                            size_t dimY, Mantid::Kernel::VMD slicePoint);

  void draw(QPainter * painter, const QPalette & palette) const override;

  void drawLabelNonOrthogonal(QPainter *painter, double labelValue, double labelPos) const;

private:
  void setTransformationMatrices(Mantid::API::IMDWorkspace_sptr workspace);
  qreal getScreenBottomInXyz() const;
  qreal getScreenLeftInXyz() const;

  QPoint fromXyzToScreen(QPointF xyz) const;
  QPointF fromScreenToXyz(QPoint screen) const;
  QPointF fromMixedCoordinatesToHkl(double x, double y) const;
  QPointF fromMixedCoordinatesToXyz(double x, double y) const;

  void convertTicksToXyz(QwtValueList& majorTicksXyz, QwtValueList& minorTicksXyz,
                         const QwtValueList& majorTicksHkl, const QwtValueList& minorTicksHkl,
                         std::function<void(double)> func);


  Mantid::coord_t m_fromHklToXyz[9];
  Mantid::coord_t m_fromXyzToHkl[9];

  // Non-owning pointer to the QwtPlot
  QwtPlot* m_plot;

  ScreenDimension m_screenDimension;

  // Non-orthogoanal information
  size_t m_dimX;
  size_t m_dimY;
  size_t m_missingDimension;
  Mantid::Kernel::VMD m_slicePoint;

};

#endif
