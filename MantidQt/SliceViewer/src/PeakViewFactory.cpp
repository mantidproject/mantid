#include "MantidQtSliceViewer/PeakViewFactory.h"
#include "MantidQtSliceViewer/PeakRepresentation.h"
#include "MantidQtSliceViewer/PeakRepresentationCross.h"
#include "MantidQtSliceViewer/PeakView.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Crystal/PeakTransform.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"

namespace
{

struct ZMinAndMax {
    double zMax;
    double zMin;
};

ZMinAndMax getZMinAndMax(Mantid::API::IMDWorkspace_sptr workspace,
                         Mantid::Geometry::PeakTransform_const_sptr transform)
{
    double zMax = 0.0;
    double zMin = 0.0;
    const auto numberOfDimensions = workspace->getNumDims();
    for (size_t dimIndex = 0; dimIndex < numberOfDimensions; ++dimIndex) {
        Mantid::Geometry::IMDDimension_const_sptr dimensionMappedToZ
            = workspace->getDimension(dimIndex);
        if (boost::regex_match(dimensionMappedToZ->getName(),
                               transform->getFreePeakAxisRegex())) {
            zMax = dimensionMappedToZ->getMaximum();
            zMin = dimensionMappedToZ->getMinimum();
            break;
        }
    }

    ZMinAndMax zMinAndMax;
    zMinAndMax.zMax = zMax;
    zMinAndMax.zMin = zMin;

    return zMinAndMax;
}
}

namespace MantidQt
{
namespace SliceViewer
{

PeakViewFactory::PeakViewFactory(Mantid::API::IMDWorkspace_sptr mdWS,
                                 Mantid::API::IPeaksWorkspace_sptr peaksWS,
                                 QwtPlot *plot, QWidget *parent,
                                 const int plotXIndex, const int plotYIndex,
                                 const size_t colorNumber)
    : PeakOverlayViewFactoryBase(plot, parent, plotXIndex, plotYIndex,
                                 colorNumber),
      m_mdWS(mdWS), m_peaksWS(peaksWS)
{
    setForegroundAndBackgroundColors(colorNumber);
}

PeakViewFactory::~PeakViewFactory() {}

boost::shared_ptr<PeakOverlayView> PeakViewFactory::createView(
    PeaksPresenter *const presenter,
    Mantid::Geometry::PeakTransform_const_sptr transform) const
{
    VecPeakRepresentation peakRepresentations(m_peaksWS->rowCount());
    int index = 0;
    for (auto &peakRepresentation : peakRepresentations) {
        const Mantid::Geometry::IPeak &peak = m_peaksWS->getPeak(index);
        auto position = transform->transformPeak(peak);
        peakRepresentation
            = createSinglePeakRepresentation(peak, position, transform);
        ++index;
    }

    return boost::make_shared<PeakView>(
        presenter, m_plot, m_parent, peakRepresentations, m_plotXIndex,
        m_plotYIndex, m_foregroundColor, m_backgroundColor);
}

PeakRepresentation_sptr PeakViewFactory::createSinglePeakRepresentation(
    const Mantid::Geometry::IPeak &peak, Mantid::Kernel::V3D position,
    Mantid::Geometry::PeakTransform_const_sptr transform) const
{
    // Available representations for this peaks: Cross, Sphere, Ellipsoid
    const auto &peakShape = peak.getPeakShape();
    const auto shapeName = peakShape.shapeName();

    // Create the correct peak representation for the peak shape
    PeakRepresentation_sptr peakRepresentation;
    if (shapeName
        == Mantid::DataObjects::PeakShapeSpherical::sphereShapeName()) {
        peakRepresentation = createPeakRepresentationSphere(position, peak);
    } else if (shapeName == Mantid::DataObjects::PeakShapeEllipsoid::
                                ellipsoidShapeName()) {
        peakRepresentation = createPeakRepresentationEllipsoid(position, peak);
    } else {
        peakRepresentation = createPeakRepresentationCross(position, transform);
    }
    return peakRepresentation;
}

PeakRepresentation_sptr PeakViewFactory::createPeakRepresentationCross(
    Mantid::Kernel::V3D position,
    Mantid::Geometry::PeakTransform_const_sptr transform) const
{
    const auto zMinAndMax = getZMinAndMax(m_mdWS, transform);
    return std::make_shared<PeakRepresentationCross>(position, zMinAndMax.zMax,
                                                     zMinAndMax.zMin);
}

PeakRepresentation_sptr PeakViewFactory::createPeakRepresentationSphere(
    Mantid::Kernel::V3D position, const Mantid::Geometry::IPeak &) const
{
    // TODO Replace with correct implementation
    return std::make_shared<PeakRepresentationCross>(position, -1.0, 1.0);
}

PeakRepresentation_sptr PeakViewFactory::createPeakRepresentationEllipsoid(
    Mantid::Kernel::V3D position, const Mantid::Geometry::IPeak &) const
{
    // TODO Replace with correct implementation
    return std::make_shared<PeakRepresentationCross>(position, -1.0, 1.0);
}

void PeakViewFactory::swapPeaksWorkspace(
    Mantid::API::IPeaksWorkspace_sptr &peaksWS)
{
    m_peaksWS = peaksWS;
}

// TODO REMOVE< wont be needed anymore
int PeakViewFactory::FOM() const { return 100; }

void PeakViewFactory::setForegroundAndBackgroundColors(
    const size_t colourNumber)
{
    PeakPalette<PeakViewColor> defaultPalette;
    const auto peakColourEnum = defaultPalette.foregroundIndexToColour(
        static_cast<int>(colourNumber));
    const auto backColourEnum = defaultPalette.backgroundIndexToColour(
        static_cast<int>(colourNumber));
    m_foregroundColor = peakColourEnum;
    m_backgroundColor = backColourEnum;
}
}
}
