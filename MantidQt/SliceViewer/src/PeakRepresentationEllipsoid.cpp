#include "MantidQtSliceViewer/PeakRepresentationEllipsoid.h"
#include "MantidQtSliceViewer/PeakBoundingBox.h"

#include <QPainter>

namespace MantidQt
{
namespace SliceViewer
{


void PeakRepresentationEllipsoid::draw(QPainter &painter) {}

void PeakRepresentationEllipsoid::setSlicePoint(const double &) {}

void PeakRepresentationEllipsoid::movePosition(Mantid::Geometry::PeakTransform_sptr peakTransform) {}

PeakBoundingBox PeakRepresentationEllipsoid::getBoundingBox() const { return PeakBoundingBox();}

void PeakRepresentationEllipsoid::setOccupancyInView(const double fraction) {}

void PeakRepresentationEllipsoid::setOccupancyIntoView(const double fraction) {}

double PeakRepresentationEllipsoid::getEffectiveRadius() const {}

double PeakRepresentationEllipsoid::getOccupancyInView() const {}

double PeakRepresentationEllipsoid::getOccupancyIntoView() const {}

}
}
