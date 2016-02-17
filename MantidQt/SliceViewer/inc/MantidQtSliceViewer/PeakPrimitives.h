#ifndef MANTID_SLICEVIEWER_PEAK_PRIMITIVES_H
#define MANTID_SLICEVIEWER_PEAK_PRIMITIVES_H

#include "MantidKernel/V3D.h"

namespace MantidQt
{
namespace SliceViewer
{

struct PeakPrimitives {
    PeakPrimitives(Mantid::Kernel::V3D peakOrigin, double peakOpacityAtDistance,
                   int peakLineWidth)
        : peakOrigin(peakOrigin),
          peakOpacityAtDistance(peakOpacityAtDistance),
          peakLineWidth(peakLineWidth)
    {
    }
    Mantid::Kernel::V3D peakOrigin;
    double peakOpacityAtDistance;
    int peakLineWidth;
};

struct PeakPrimitivesCross : public PeakPrimitives {
    PeakPrimitivesCross(Mantid::Kernel::V3D peakOrigin,
                        double peakOpacityAtDistance, int peakLineWidth,
                        int peakHalfCrossWidth, int peakHalfCrossHeight)
        : PeakPrimitives(peakOrigin, peakOpacityAtDistance, peakLineWidth),
          peakHalfCrossWidth(peakHalfCrossWidth),
          peakHalfCrossHeight(peakHalfCrossHeight)
    {
    }
    int peakHalfCrossWidth;
    int peakHalfCrossHeight;
};

struct PeakPrimitivesSphere : public PeakPrimitives {
    PeakPrimitivesSphere(Mantid::Kernel::V3D peakOrigin,
                         double peakOpacityAtDistance, int peakLineWidth,
                         double peakInnerRadiusX, double peakInnerRadiusY,
                         double backgroundOuterRadiusX,
                         double backgroundOuterRadiusY,
                         double backgroundInnerRadiusX,
                         double backgroundInnerRadiusY)
        : PeakPrimitives(peakOrigin, peakOpacityAtDistance, peakLineWidth),
          peakInnerRadiusX(peakInnerRadiusX),
          peakInnerRadiusY(peakInnerRadiusY),
          backgroundOuterRadiusX(backgroundOuterRadiusX),
          backgroundOuterRadiusY(backgroundOuterRadiusY),
          backgroundInnerRadiusX(backgroundInnerRadiusX),
          backgroundInnerRadiusY(backgroundInnerRadiusY)
    {
    }
    double peakInnerRadiusX;
    double peakInnerRadiusY;
    double backgroundOuterRadiusX;
    double backgroundOuterRadiusY;
    double backgroundInnerRadiusX;
    double backgroundInnerRadiusY;
};
}
}

#endif
