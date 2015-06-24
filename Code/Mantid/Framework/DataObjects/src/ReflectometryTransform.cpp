#include "MantidDataObjects/ReflectometryTransform.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidDataObjects/CalculateReflectometry.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/UnitFactory.h"

#include <boost/shared_ptr.hpp>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid {
namespace DataObjects {

ReflectometryTransform::ReflectometryTransform(
    const std::string &d0Label, const std::string &d0ID, double d0Min,
    double d0Max, const std::string &d1Label, const std::string &d1ID,
    double d1Min, double d1Max, size_t d0NumBins, size_t d1NumBins,
    CalculateReflectometry *calc)
    : m_d0NumBins(d0NumBins), m_d1NumBins(d1NumBins), m_d0Min(d0Min),
      m_d1Min(d1Min), m_d0Max(d0Max), m_d1Max(d1Max), m_d0Label(d0Label),
      m_d1Label(d1Label), m_d0ID(d0ID), m_d1ID(d1ID), m_calculator(calc) {
  if (d0Min >= d0Max || d1Min >= d1Max)
    throw std::invalid_argument(
        "The supplied minimum values must be less than the maximum values.");
}

ReflectometryTransform::~ReflectometryTransform() {}

boost::shared_ptr<MDEventWorkspace2Lean>
ReflectometryTransform::createMDWorkspace(
    Mantid::Geometry::IMDDimension_sptr a,
    Mantid::Geometry::IMDDimension_sptr b,
    BoxController_sptr boxController) const {
  auto ws = boost::make_shared<MDEventWorkspace2Lean>();

  ws->addDimension(a);
  ws->addDimension(b);

  BoxController_sptr wsbc = ws->getBoxController(); // Get the box controller
  wsbc->setSplitInto(boxController->getSplitInto(0));
  wsbc->setMaxDepth(boxController->getMaxDepth());
  wsbc->setSplitThreshold(boxController->getSplitThreshold());

  // Initialize the workspace.
  ws->initialize();

  // Start with a MDGridBox.
  ws->splitBox();
  return ws;
}

/**
 * Create a new X-Axis for the output workspace
 * @param ws : Workspace to attach the axis to
 * @param gradX : Gradient used in the linear transform from index to X-scale
 * @param cxToUnit : C-offset used in the linear transform
 * @param nBins : Number of bins along this axis
 * @param caption : Caption for the axis
 * @param units : Units label for the axis
 * @return Vector containing increments along the axis.
 */
MantidVec createXAxis(MatrixWorkspace *const ws, const double gradX,
                      const double cxToUnit, const size_t nBins,
                      const std::string &caption, const std::string &units) {
  // Create an X - Axis.
  auto *const xAxis = new BinEdgeAxis(nBins);
  ws->replaceAxis(0, xAxis);
  auto unitXBasePtr = UnitFactory::Instance().create("Label");
  boost::shared_ptr<Mantid::Kernel::Units::Label> xUnit =
      boost::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(unitXBasePtr);
  xUnit->setLabel(caption, units);
  xAxis->unit() = xUnit;
  xAxis->title() = caption;
  MantidVec xAxisVec(nBins);
  for (size_t i = 0; i < nBins; ++i) {
    double qxIncrement = ((1 / gradX) * ((double)i + 1) + cxToUnit);
    xAxis->setValue(i, qxIncrement);
    xAxisVec[i] = qxIncrement;
  }
  return xAxisVec;
}

/**
 * Create a new Y, or Vertical Axis for the output workspace
 * @param ws : Workspace to attache the vertical axis to
 * @param xAxisVec : Vector of x axis increments
 * @param gradY : Gradient used in linear transform from index to Y-scale
 * @param cyToUnit : C-offset used in the linear transform
 * @param nBins : Number of bins along the axis
 * @param caption : Caption for the axis
 * @param units : Units label for the axis
 */
void createVerticalAxis(MatrixWorkspace *const ws, const MantidVec &xAxisVec,
                        const double gradY, const double cyToUnit,
                        const size_t nBins, const std::string &caption,
                        const std::string &units) {
  // Create a Y (vertical) Axis
  auto *const verticalAxis = new BinEdgeAxis(nBins);
  ws->replaceAxis(1, verticalAxis);
  auto unitZBasePtr = UnitFactory::Instance().create("Label");
  boost::shared_ptr<Mantid::Kernel::Units::Label> verticalUnit =
      boost::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(unitZBasePtr);
  verticalAxis->unit() = verticalUnit;
  verticalUnit->setLabel(caption, units);
  verticalAxis->title() = caption;
  for (size_t i = 0; i < nBins; ++i) {
    ws->setX(i, xAxisVec);
    double qzIncrement = ((1 / gradY) * ((double)i + 1) + cyToUnit);
    verticalAxis->setValue(i, qzIncrement);
  }
}

Mantid::API::MatrixWorkspace_sptr ReflectometryTransform::executeNormPoly(
    Mantid::API::MatrixWorkspace_const_sptr inputWs) const {
  UNUSED_ARG(inputWs);
  throw std::runtime_error("executeNormPoly not implemented.");
}

/**
 * A map detector ID and Q ranges
 * This method looks unnecessary as it could be calculated on the fly but
 * the parallelization means that lazy instantation slows it down due to the
 * necessary CRITICAL sections required to update the cache. The Q range
 * values are required very frequently so the total time is more than
 * offset by this precaching step
 */
void ReflectometryTransform::initAngularCaches(
    const API::MatrixWorkspace_const_sptr &workspace) const {
  const size_t nhist = workspace->getNumberHistograms();
  m_theta = std::vector<double>(nhist);
  m_thetaWidths = std::vector<double>(nhist);

  auto inst = workspace->getInstrument();
  const auto samplePos = inst->getSample()->getPos();
  const PointingAlong upDir = inst->getReferenceFrame()->pointingUp();

  for (size_t i = 0; i < nhist; ++i) // signed for OpenMP
  {
    IDetector_const_sptr det;
    try {
      det = workspace->getDetector(i);
    } catch (Kernel::Exception::NotFoundError &) {
      // Catch if no detector. Next line tests whether this happened - test
      // placed
      // outside here because Mac Intel compiler doesn't like 'continue' in a
      // catch
      // in an openmp block.
    }
    // If no detector found, skip onto the next spectrum
    if (!det || det->isMonitor()) {
      m_theta[i] = -1.0; // Indicates a detector to skip
      m_thetaWidths[i] = -1.0;
      continue;
    }
    // We have to convert theta from radians to degrees
    const double theta = workspace->detectorTwoTheta(det) * 180.0 / M_PI;
    m_theta[i] = theta;

    /**
     * Determine width from shape geometry. A group is assumed to contain
     * detectors with the same shape & r, theta value, i.e. a ring mapped-group
     * The shape is retrieved and rotated to match the rotation of the detector.
     * The angular width is computed using the l2 distance from the sample
     */
    if (auto group = boost::dynamic_pointer_cast<const DetectorGroup>(det)) {
      // assume they all have same shape and same r,theta
      auto dets = group->getDetectors();
      det = dets[0];
    }
    const auto pos = det->getPos();
    double l2(0.0), t(0.0), p(0.0);
    pos.getSpherical(l2, t, p);
    // Get the shape
    auto shape =
        det->shape(); // Defined in its own reference frame with centre at 0,0,0
    auto rot = det->getRotation();
    BoundingBox bbox = shape->getBoundingBox();
    auto maxPoint(bbox.maxPoint());
    rot.rotate(maxPoint);
    double boxWidth = maxPoint[upDir];

    m_thetaWidths[i] = std::fabs(2.0 * std::atan(boxWidth / l2));
  }
}
}
}
