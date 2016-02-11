#ifndef MANTID_SLICEVIEWER_PEAK_PRIMITIVES_H
#define MANTID_SLICEVIEWER_PEAK_PRIMITIVES_H

#include "MantidKernel/V3D.h"

namespace MantidQt
{
namespace SliceViewer
{

struct PeakPrimitives {
    PeakPrimitives(Mantid::Kernel::V3D peakOrigin, double peakOpacityAtDistance)
        : peakOrigin(peakOrigin), peakOpacityAtDistance(peakOpacityAtDistance)
    {
    }
    Mantid::Kernel::V3D peakOrigin;
    double peakOpacityAtDistance;
};

struct PeakPrimitivesCross : public PeakPrimitives {
    PeakPrimitivesCross(Mantid::Kernel::V3D peakOrigin,
                        double peakOpacityAtDistance, int peakHalfCrossWidth,
                        int peakHalfCrossHeight, int peakLineWidth)
        : PeakPrimitives(peakOrigin, peakOpacityAtDistance),
          peakHalfCrossWidth(peakHalfCrossWidth),
          peakHalfCrossHeight(peakHalfCrossHeight), peakLineWidth(peakLineWidth)
    {
    }
    int peakHalfCrossWidth;
    int peakHalfCrossHeight;
    int peakLineWidth;
};

struct PeakPrimitivesSphere : public PeakPrimitives {
    PeakPrimitivesSphere(Mantid::Kernel::V3D peakOrigin,
                        double peakOpacityAtDistance, double peakInnerRadiusX,
                        double peakInnerRadiusY, double backgroundOuterRadiusX,
                        double backgroundOuterRadiusY,
                        double backgroundInnerRadiusX,
                        double backgroundInnerRadiusY)
        : PeakPrimitives(peakOrigin, peakOpacityAtDistance),
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
