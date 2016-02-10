#include "MantidQtSliceViewer/PeakRepresentationSphere.h"
#include "MantidQtSliceViewer/PeakBoundingBox.h"

#include <QPainter>

namespace MantidQt
{
namespace SliceViewer
{

PeakRepresentationSphere::PeakRepresentationSphere(const Mantid::Kernel::V3D& origin,
                           const double& peakRadius,
                           const double& backgroundInnerRadius,
                           const double& backgroundOuterRadius) :
                          m_originalOrigin(origin),
                          m_origin(origin),
                          m_peakRadius(peakRadius),
                          m_backgroundInnerRadius(backgroundInnerRadius),
                          m_backgroundOuterRadius(backgroundOuterRadius){

}

void PeakRepresentationSphere::draw(QPainter &painter) {}

void PeakRepresentationSphere::setSlicePoint(const double &) {}

void PeakRepresentationSphere::movePosition(Mantid::Geometry::PeakTransform_sptr peakTransform) {}

PeakBoundingBox PeakRepresentationSphere::getBoundingBox() const {}

void PeakRepresentationSphere::setOccupancyInView(const double fraction) {}

void PeakRepresentationSphere::setOccupancyIntoView(const double fraction) {}

double PeakRepresentationSphere::getEffectiveRadius() const {}

double PeakRepresentationSphere::getOccupancyInView() const {}

double PeakRepresentationSphere::getOccupancyIntoView() const {}

}
}
