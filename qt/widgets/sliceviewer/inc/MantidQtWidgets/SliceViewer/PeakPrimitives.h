#ifndef MANTID_SLICEVIEWER_PEAK_PRIMITIVES_H
#define MANTID_SLICEVIEWER_PEAK_PRIMITIVES_H

#include "DllOption.h"
#include "MantidKernel/V3D.h"

namespace MantidQt {
namespace SliceViewer {

struct EXPORT_OPT_MANTIDQT_SLICEVIEWER PeakPrimitives {
  PeakPrimitives(Mantid::Kernel::V3D peakOrigin, double peakOpacityAtDistance,
                 int peakLineWidth)
      : peakOrigin(peakOrigin), peakOpacityAtDistance(peakOpacityAtDistance),
        peakLineWidth(peakLineWidth) {}
  Mantid::Kernel::V3D peakOrigin;
  double peakOpacityAtDistance;
  int peakLineWidth;
};

struct EXPORT_OPT_MANTIDQT_SLICEVIEWER PeakPrimitivesCross
    : public PeakPrimitives {
  PeakPrimitivesCross(Mantid::Kernel::V3D peakOrigin,
                      double peakOpacityAtDistance, int peakLineWidth,
                      int peakHalfCrossWidth, int peakHalfCrossHeight)
      : PeakPrimitives(peakOrigin, peakOpacityAtDistance, peakLineWidth),
        peakHalfCrossWidth(peakHalfCrossWidth),
        peakHalfCrossHeight(peakHalfCrossHeight) {}
  int peakHalfCrossWidth;
  int peakHalfCrossHeight;
};

struct EXPORT_OPT_MANTIDQT_SLICEVIEWER PeakPrimitiveCircle
    : public PeakPrimitives {
  PeakPrimitiveCircle(Mantid::Kernel::V3D peakOrigin,
                      double peakOpacityAtDistance, int peakLineWidth,
                      double peakInnerRadiusX, double peakInnerRadiusY,
                      double backgroundOuterRadiusX,
                      double backgroundOuterRadiusY,
                      double backgroundInnerRadiusX,
                      double backgroundInnerRadiusY)
      : PeakPrimitives(peakOrigin, peakOpacityAtDistance, peakLineWidth),
        peakInnerRadiusX(peakInnerRadiusX), peakInnerRadiusY(peakInnerRadiusY),
        backgroundOuterRadiusX(backgroundOuterRadiusX),
        backgroundOuterRadiusY(backgroundOuterRadiusY),
        backgroundInnerRadiusX(backgroundInnerRadiusX),
        backgroundInnerRadiusY(backgroundInnerRadiusY) {}
  double peakInnerRadiusX;
  double peakInnerRadiusY;
  double backgroundOuterRadiusX;
  double backgroundOuterRadiusY;
  double backgroundInnerRadiusX;
  double backgroundInnerRadiusY;
};

struct EXPORT_OPT_MANTIDQT_SLICEVIEWER PeakPrimitivesEllipse
    : public PeakPrimitives {
  PeakPrimitivesEllipse(Mantid::Kernel::V3D peakOrigin,
                        double peakOpacityAtDistance, int peakLineWidth,
                        double peakInnerRadiusMajorAxis,
                        double peakInnerRadiusMinorAxis,
                        double backgroundOuterRadiusMajorAxis,
                        double backgroundOuterRadiusMinorAxis,
                        double backgroundInnerRadiusMajorAxis,
                        double backgroundInnerRadiusMinorAxis, double angle)
      : PeakPrimitives(peakOrigin, peakOpacityAtDistance, peakLineWidth),
        peakInnerRadiusMajorAxis(peakInnerRadiusMajorAxis),
        peakInnerRadiusMinorAxis(peakInnerRadiusMinorAxis),
        backgroundOuterRadiusMajorAxis(backgroundOuterRadiusMajorAxis),
        backgroundOuterRadiusMinorAxis(backgroundOuterRadiusMinorAxis),
        backgroundInnerRadiusMajorAxis(backgroundInnerRadiusMajorAxis),
        backgroundInnerRadiusMinorAxis(backgroundInnerRadiusMinorAxis),
        angle(angle) {}
  double peakInnerRadiusMajorAxis;
  double peakInnerRadiusMinorAxis;
  double backgroundOuterRadiusMajorAxis;
  double backgroundOuterRadiusMinorAxis;
  double backgroundInnerRadiusMajorAxis;
  double backgroundInnerRadiusMinorAxis;
  double angle;
};
}
}

#endif
