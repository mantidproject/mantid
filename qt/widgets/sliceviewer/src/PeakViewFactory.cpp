#include "MantidQtWidgets/SliceViewer/PeakViewFactory.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Crystal/PeakTransform.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidQtWidgets/Common/NonOrthogonal.h"
#include "MantidQtWidgets/SliceViewer/NonOrthogonalAxis.h"
#include "MantidQtWidgets/SliceViewer/PeakRepresentationCross.h"
#include "MantidQtWidgets/SliceViewer/PeakRepresentationEllipsoid.h"
#include "MantidQtWidgets/SliceViewer/PeakRepresentationSphere.h"
#include "MantidQtWidgets/SliceViewer/PeakView.h"

namespace {
struct ZMinAndMax {
  double zMax;
  double zMin;
};

ZMinAndMax getZMinAndMax(Mantid::API::IMDWorkspace_sptr workspace,
                         Mantid::Geometry::PeakTransform_const_sptr transform) {
  double zMax = 0.0;
  double zMin = 0.0;
  const auto numberOfDimensions = workspace->getNumDims();
  for (size_t dimIndex = 0; dimIndex < numberOfDimensions; ++dimIndex) {
    Mantid::Geometry::IMDDimension_const_sptr dimensionMappedToZ =
        workspace->getDimension(dimIndex);
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

std::vector<Mantid::Kernel::V3D> getDirectionsForEllipticalPeak(
    const Mantid::Geometry::IPeak &peak,
    const Mantid::DataObjects::PeakShapeEllipsoid &ellipticalShape,
    const Mantid::Geometry::MDFrame &frame) {
  std::vector<Mantid::Kernel::V3D> directions;
  if (frame.name() == Mantid::Geometry::QSample::QSampleName) {
    Mantid::Kernel::Matrix<double> goniometerMatrix =
        peak.getGoniometerMatrix();
    if (goniometerMatrix.Invert() != 0.0) {
      directions =
          ellipticalShape.getDirectionInSpecificFrame(goniometerMatrix);
    } else {
      directions = ellipticalShape.directions();
    }
  } else {

    directions = ellipticalShape.directions();
  }
  return directions;
}
} // namespace

namespace MantidQt {
namespace SliceViewer {

PeakViewFactory::PeakViewFactory(Mantid::API::IMDWorkspace_sptr mdWS,
                                 Mantid::API::IPeaksWorkspace_sptr peaksWS,
                                 QwtPlot *plot, QWidget *parent,
                                 const int plotXIndex, const int plotYIndex,
                                 const size_t colorNumber)
    : PeakOverlayViewFactoryBase(plot, parent, plotXIndex, plotYIndex,
                                 colorNumber),
      m_mdWS(mdWS), m_peaksWS(peaksWS),
      m_calculator(std::make_shared<
                   Mantid::SliceViewer::EllipsoidPlaneSliceCalculator>()) {
  setForegroundAndBackgroundColors(colorNumber);
}

PeakViewFactory::~PeakViewFactory() {}

boost::shared_ptr<PeakOverlayView> PeakViewFactory::createView(
    PeaksPresenter *const presenter,
    Mantid::Geometry::PeakTransform_const_sptr transform) const {
  double largestEffectiveRadius = 0.0;
  VecPeakRepresentation peakRepresentations(m_peaksWS->rowCount());
  int index = 0;
  for (auto &peakRepresentation : peakRepresentations) {
    const Mantid::Geometry::IPeak &peak = m_peaksWS->getPeak(index);
    auto position = transform->transformPeak(peak);
    peakRepresentation =
        createSinglePeakRepresentation(peak, position, transform);
    // Get the largest radius of the data set
    double currentEffectiveRadius = peakRepresentation->getEffectiveRadius();
    if (currentEffectiveRadius > largestEffectiveRadius) {
      largestEffectiveRadius = currentEffectiveRadius;
    }
    ++index;
  }

  return boost::make_shared<PeakView>(
      presenter, m_plot, m_parent, peakRepresentations, m_plotXIndex,
      m_plotYIndex, m_foregroundColor, m_backgroundColor,
      largestEffectiveRadius);
}

PeakRepresentation_sptr PeakViewFactory::createSinglePeakRepresentation(
    const Mantid::Geometry::IPeak &peak, Mantid::Kernel::V3D position,
    Mantid::Geometry::PeakTransform_const_sptr transform) const {
  // Available representations for this peaks: Cross, Sphere, Ellipsoid
  const auto &peakShape = peak.getPeakShape();
  const auto shapeName = peakShape.shapeName();

  // Create the correct peak representation for the peak shape
  PeakRepresentation_sptr peakRepresentation;
  if (shapeName == Mantid::DataObjects::PeakShapeSpherical::sphereShapeName()) {
    peakRepresentation = createPeakRepresentationSphere(position, peak);
  } else if (shapeName ==
             Mantid::DataObjects::PeakShapeEllipsoid::ellipsoidShapeName()) {
    peakRepresentation = createPeakRepresentationEllipsoid(position, peak);
  } else {
    peakRepresentation = createPeakRepresentationCross(position, transform);
  }
  return peakRepresentation;
}

PeakRepresentation_sptr PeakViewFactory::createPeakRepresentationCross(
    Mantid::Kernel::V3D position,
    Mantid::Geometry::PeakTransform_const_sptr transform) const {
  const auto zMinAndMax = getZMinAndMax(m_mdWS, transform);
  return std::make_shared<PeakRepresentationCross>(position, zMinAndMax.zMax,
                                                   zMinAndMax.zMin);
}

PeakRepresentation_sptr PeakViewFactory::createPeakRepresentationSphere(
    Mantid::Kernel::V3D position, const Mantid::Geometry::IPeak &peak) const {
  const auto &shape = peak.getPeakShape();
  const auto &sphericalShape =
      dynamic_cast<const Mantid::DataObjects::PeakShapeSpherical &>(shape);

  // Get the radius
  const auto peakRadius = sphericalShape.radius().get();

  // Get the background inner radius. If it does not exist, default to radius.
  const auto backgroundInnerRadiusOptional =
      sphericalShape.backgroundInnerRadius();
  const auto backgroundInnerRadius =
      backgroundInnerRadiusOptional.is_initialized()
          ? backgroundInnerRadiusOptional.get()
          : peakRadius;

  // Get the background outer radius. If it does not exist, default to radius.
  const auto backgroundOuterRadiusOptional =
      sphericalShape.backgroundOuterRadius();
  const auto backgroundOuterRadius =
      backgroundOuterRadiusOptional.is_initialized()
          ? backgroundOuterRadiusOptional.get()
          : peakRadius;

  return std::make_shared<PeakRepresentationSphere>(
      position, peakRadius, backgroundInnerRadius, backgroundOuterRadius);
}

PeakRepresentation_sptr PeakViewFactory::createPeakRepresentationEllipsoid(
    Mantid::Kernel::V3D position, const Mantid::Geometry::IPeak &peak) const {
  const auto &shape = peak.getPeakShape();
  const auto &ellipsoidShape =
      dynamic_cast<const Mantid::DataObjects::PeakShapeEllipsoid &>(shape);

  // Ellipsoidd paramters
  const auto &abcRadii = ellipsoidShape.abcRadii();
  const auto &abcRadiiBackgroundInner =
      ellipsoidShape.abcRadiiBackgroundInner();
  const auto &abcRadiiBackgroundOuter =
      ellipsoidShape.abcRadiiBackgroundOuter();

  // Extract directions for the displayed frame
  const auto dimension0 = m_mdWS->getDimension(0);
  const auto &frame = dimension0->getMDFrame();
  auto directions = getDirectionsForEllipticalPeak(peak, ellipsoidShape, frame);

  return std::make_shared<PeakRepresentationEllipsoid>(
      position, abcRadii, abcRadiiBackgroundInner, abcRadiiBackgroundOuter,
      directions, m_calculator);
}

void PeakViewFactory::swapPeaksWorkspace(
    Mantid::API::IPeaksWorkspace_sptr &peaksWS) {
  m_peaksWS = peaksWS;
}

void PeakViewFactory::setForegroundAndBackgroundColors(
    const size_t colourNumber) {
  PeakPalette<PeakViewColor> defaultPalette;
  const auto peakColourEnum =
      defaultPalette.foregroundIndexToColour(static_cast<int>(colourNumber));
  const auto backColourEnum =
      defaultPalette.backgroundIndexToColour(static_cast<int>(colourNumber));
  m_foregroundColor = peakColourEnum;
  m_backgroundColor = backColourEnum;
}

void PeakViewFactory::getNonOrthogonalInfo(NonOrthogonalAxis &info) {
  if (API::requiresSkewMatrix(*m_mdWS)) {
    auto numberOfDimensions = m_mdWS->getNumDims();
    Mantid::Kernel::DblMatrix skewMatrixDbl(numberOfDimensions,
                                            numberOfDimensions, true);
    API::provideSkewMatrix(skewMatrixDbl, *m_mdWS);
    skewMatrixDbl.Invert();
    API::transformFromDoubleToCoordT(skewMatrixDbl, info.fromHklToXyz);
    info.dimMissing =
        API::getMissingHKLDimensionIndex(m_mdWS, info.dimX, info.dimY);
  }
}
} // namespace SliceViewer
} // namespace MantidQt
