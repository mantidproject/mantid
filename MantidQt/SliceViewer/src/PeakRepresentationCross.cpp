#include "MantidQtSliceViewer/PeakRepresentationCross.h"
#include "MantidQtSliceViewer/PeakBoundingBox.h"

#include <QPainter>

namespace MantidQt
{
namespace SliceViewer
{


void PeakRepresentationCross::draw(QPainter &painter) {}

void PeakRepresentationCross::setSlicePoint(const double &) {}

void PeakRepresentationCross::movePosition(Mantid::Geometry::PeakTransform_sptr peakTransform) {}

PeakBoundingBox PeakRepresentationCross::getBoundingBox() const {}

void PeakRepresentationCross::setOccupancyInView(const double fraction) {}

void PeakRepresentationCross::setOccupancyIntoView(const double fraction) {}

double PeakRepresentationCross::getEffectiveRadius() const {}

double PeakRepresentationCross::getOccupancyInView() const {}

double PeakRepresentationCross::getOccupancyIntoView() const {}

}
}
