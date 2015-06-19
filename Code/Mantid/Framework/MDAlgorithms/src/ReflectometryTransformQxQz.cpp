#include "MantidMDAlgorithms/ReflectometryTransformQxQz.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidDataObjects/Workspace2D.h"
#include <stdexcept>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

typedef std::map<Mantid::specid_t, Mantid::Kernel::V3D> SpectraDistanceMap;
typedef Mantid::Geometry::IDetector_const_sptr DetConstPtr;

namespace Mantid {
namespace MDAlgorithms {

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ReflectometryTransformQxQz::~ReflectometryTransformQxQz() {}

/*
 Constructor
 @param qxMin: min qx value (extent)
 @param qxMax: max qx value (extent)
 @param qzMin: min qz value (extent)
 @param qzMax; max qz value (extent)
 @param incidentTheta: Predetermined incident theta value
 @param numberOfBinsQx : Number of bins along the qx axis
 @param numberOfBinsQz : Number of bins along the qz axis
 */
ReflectometryTransformQxQz::ReflectometryTransformQxQz(
    double qxMin, double qxMax, double qzMin, double qzMax,
    double incidentTheta, int numberOfBinsQx, int numberOfBinsQz)
    : ReflectometryTransform(numberOfBinsQx, numberOfBinsQz), m_qxMin(qxMin),
      m_qxMax(qxMax), m_qzMin(qzMin), m_qzMax(qzMax),
      m_QxCalculation(incidentTheta), m_QzCalculation(incidentTheta) {
  if (qxMin >= qxMax) {
    throw std::invalid_argument("min qx bounds must be < max qx bounds");
  }
  if (qzMin >= qzMax) {
    throw std::invalid_argument("min qz bounds must be < max qz bounds");
  }
  if (incidentTheta < 0 || incidentTheta > 90) {
    throw std::out_of_range("incident theta angle must be > 0 and < 90");
  }
}

/*
 Execute the transformation. Generates an output IMDEventWorkspace.
 @return the constructed IMDEventWorkspace following the transformation.
 @param ws: Input MatrixWorkspace, with a vertical axis of signed-theta, and an
 x-axis of wavelength
 @param boxController: Box controller to apply to output workspace
 */
IMDEventWorkspace_sptr
ReflectometryTransformQxQz::executeMD(MatrixWorkspace_const_sptr inputWs,
                                      BoxController_sptr boxController) const {

  MDHistoDimension_sptr qxDim = MDHistoDimension_sptr(new MDHistoDimension(
      "Qx", "qx", "(Ang^-1)", static_cast<Mantid::coord_t>(m_qxMin),
      static_cast<Mantid::coord_t>(m_qxMax), m_nbinsx));
  MDHistoDimension_sptr qzDim = MDHistoDimension_sptr(new MDHistoDimension(
      "Qz", "qz", "(Ang^-1)", static_cast<Mantid::coord_t>(m_qzMin),
      static_cast<Mantid::coord_t>(m_qzMax), m_nbinsz));

  auto ws = createMDWorkspace(qxDim, qzDim, boxController);

  auto spectraAxis = inputWs->getAxis(1);
  for (size_t index = 0; index < inputWs->getNumberHistograms(); ++index) {
    auto counts = inputWs->readY(index);
    auto wavelengths = inputWs->readX(index);
    auto errors = inputWs->readE(index);
    const size_t nInputBins = wavelengths.size() - 1;
    const double theta_final = spectraAxis->getValue(index);
    m_QxCalculation.setThetaFinal(theta_final);
    m_QzCalculation.setThetaFinal(theta_final);
    // Loop over all bins in spectra
    for (size_t binIndex = 0; binIndex < nInputBins; ++binIndex) {
      const double wavelength =
          0.5 * (wavelengths[binIndex] + wavelengths[binIndex + 1]);
      double _qx = m_QxCalculation.execute(wavelength);
      double _qz = m_QzCalculation.execute(wavelength);
      double centers[2] = {_qx, _qz};

      ws->addEvent(MDLeanEvent<2>(float(counts[binIndex]),
                                  float(errors[binIndex] * errors[binIndex]),
                                  centers));
    }
  }
  ws->splitAllIfNeeded(NULL);
  ws->refreshCache();
  return ws;
}

/**
 * Execute the transformation. Generates an output Matrix workspace.
 * @param inputWs : Input workspace with a vertical axis of signed-theta and an
 * x-axis of wavelength
 * @return : A 2D workspace with qz on the vertical axis and qx on the
 * horizontal axis.
 */
MatrixWorkspace_sptr
ReflectometryTransformQxQz::execute(MatrixWorkspace_const_sptr inputWs) const {
  auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();

  ws->initialize(m_nbinsz, m_nbinsx,
                 m_nbinsx); // Create the output workspace as a distribution

  // Mapping so that qx and qz values calculated can be added to the matrix
  // workspace at the correct index.
  const double gradQx = double(m_nbinsx) / (m_qxMax - m_qxMin); // The x - axis
  const double gradQz =
      double(m_nbinsz) / (m_qzMax - m_qzMin); // Actually the y-axis
  const double cxToIndex = -gradQx * m_qxMin;
  const double czToIndex = -gradQz * m_qzMin;
  const double cxToQ = m_qxMin - (1 / gradQx);
  const double czToQ = m_qzMin - (1 / gradQz);

  // Create an X - Axis.
  MantidVec xAxisVec =
      createXAxis(ws.get(), gradQx, cxToQ, m_nbinsx, "qx", "1/Angstroms");
  // Create a Y (vertical) Axis
  createVerticalAxis(ws.get(), xAxisVec, gradQz, czToQ, m_nbinsz, "qz",
                     "1/Angstroms");

  // Loop over all entries in the input workspace and calculate qx and qz for
  // each.
  auto spectraAxis = inputWs->getAxis(1);
  for (size_t index = 0; index < inputWs->getNumberHistograms(); ++index) {
    auto counts = inputWs->readY(index);
    auto wavelengths = inputWs->readX(index);
    auto errors = inputWs->readE(index);
    const size_t nInputBins = wavelengths.size() - 1;
    const double theta_final = spectraAxis->getValue(index);
    m_QxCalculation.setThetaFinal(theta_final);
    m_QzCalculation.setThetaFinal(theta_final);
    // Loop over all bins in spectra
    for (size_t binIndex = 0; binIndex < nInputBins; ++binIndex) {
      const double wavelength =
          0.5 * (wavelengths[binIndex] + wavelengths[binIndex + 1]);
      const double _qx = m_QxCalculation.execute(wavelength);
      const double _qz = m_QzCalculation.execute(wavelength);

      if (_qx >= m_qxMin && _qx <= m_qxMax && _qz >= m_qzMin &&
          _qz <= m_qzMax) // Check that the calculated qx and qz are in range
      {
        const int outIndexX = int((gradQx * _qx) + cxToIndex);
        const int outIndexZ = int((gradQz * _qz) + czToIndex);

        ws->dataY(outIndexZ)[outIndexX] += counts[binIndex];
        ws->dataE(outIndexZ)[outIndexX] += errors[binIndex];
      }
    }
  }
  return ws;
}

/**
 * A map detector ID and Q ranges
 * This method looks unnecessary as it could be calculated on the fly but
 * the parallelization means that lazy instantation slows it down due to the
 * necessary CRITICAL sections required to update the cache. The Q range
 * values are required very frequently so the total time is more than
 * offset by this precaching step
 */
void ReflectometryTransformQxQz::initAngularCaches(
    const API::MatrixWorkspace_const_sptr &workspace) {
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
    const double theta = workspace->detectorTwoTheta(det);
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

} // namespace Mantid
} // namespace MDAlgorithms
