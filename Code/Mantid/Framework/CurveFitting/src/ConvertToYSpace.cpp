#include "MantidCurveFitting/ConvertToYSpace.h"

#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidKernel/BoundedValidator.h"

#include <boost/make_shared.hpp>

namespace Mantid {
namespace CurveFitting {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToYSpace)

using namespace API;
using namespace Kernel;

namespace {
/// Conversion constant
const double MASS_TO_MEV =
    0.5 * PhysicalConstants::NeutronMass / PhysicalConstants::meV;
}

//----------------------------------------------------------------------------------------------
/** Constructor
*/
ConvertToYSpace::ConvertToYSpace()
    : Algorithm(), m_inputWS(), m_mass(0.0), m_l1(0.0), m_samplePos(),
      m_outputWS() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string ConvertToYSpace::name() const { return "ConvertToYSpace"; }

/// Algorithm's version for identification. @see Algorithm::version
int ConvertToYSpace::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ConvertToYSpace::category() const {
  return "Transforms\\Units";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/**
* @param ws The workspace with attached instrument
* @param index Index of the spectrum
* @return DetectorParams structure containing the relevant parameters
*/
DetectorParams ConvertToYSpace::getDetectorParameters(
    const API::MatrixWorkspace_const_sptr &ws, const size_t index) {
  auto inst = ws->getInstrument();
  auto sample = inst->getSample();
  auto source = inst->getSource();
  if (!sample || !source) {
    throw std::invalid_argument(
        "ConvertToYSpace - Workspace has no source/sample.");
  }
  Geometry::IDetector_const_sptr det;
  try {
    det = ws->getDetector(index);
  } catch (Kernel::Exception::NotFoundError &) {
    throw std::invalid_argument("ConvertToYSpace - Workspace has no detector "
                                "attached to histogram at index " +
                                boost::lexical_cast<std::string>(index));
  }

  DetectorParams detpar;
  const auto &pmap = ws->constInstrumentParameters();
  detpar.l1 = sample->getDistance(*source);
  detpar.l2 = det->getDistance(*sample);
  detpar.pos = det->getPos();
  detpar.theta = ws->detectorTwoTheta(det);
  detpar.t0 = ConvertToYSpace::getComponentParameter(det, pmap, "t0") *
              1e-6; // Convert to seconds
  detpar.efixed = ConvertToYSpace::getComponentParameter(det, pmap, "efixed");
  return detpar;
}

/**
* If a DetectorGroup is encountered then the parameters are averaged over the
* group
* @param comp A pointer to the component that should contain the parameter
* @param pmap A reference to the ParameterMap that stores the parameters
* @param name The name of the parameter
* @returns The value of the parameter if it exists
* @throws A std::invalid_argument error if the parameter does not exist
*/
double ConvertToYSpace::getComponentParameter(
    const Geometry::IComponent_const_sptr &comp,
    const Geometry::ParameterMap &pmap, const std::string &name) {
  if (!comp)
    throw std::invalid_argument(
        "ComptonProfile - Cannot retrieve parameter from NULL component");

  double result(0.0);
  if (const auto group =
          boost::dynamic_pointer_cast<const Geometry::DetectorGroup>(comp)) {
    const auto dets = group->getDetectors();
    double avg(0.0);
    for (auto it = dets.begin(); it != dets.end(); ++it) {
      auto param = pmap.getRecursive((*it)->getComponentID(), name);
      if (param)
        avg += param->value<double>();
      else
        throw std::invalid_argument("ComptonProfile - Unable to find "
                                    "DetectorGroup component parameter \"" +
                                    name + "\".");
    }
    result = avg / static_cast<double>(group->nDets());
  } else {
    auto param = pmap.getRecursive(comp->getComponentID(), name);
    if (param) {
      result = param->value<double>();
    } else {
      throw std::invalid_argument(
          "ComptonProfile - Unable to find component parameter \"" + name +
          "\".");
    }
  }
  return result;
}

//----------------------------------------------------------------------------------------------

/**
* @param yspace Output yspace value
* @param qspace Output qspace value
* @param ei Output incident energy value
* @param mass Mass value for the transformation
* @param tsec Time-of-flight in seconds
* @param k1 Modulus of wavevector for final energy (sqrt(efixed/massToMeV)),
* avoids repeated calculation
* @param v1 Velocity of neutron for final energy (sqrt(efixed/massToMeV)),
* avoids repeated calculation
* @param detpar Struct defining Detector parameters @see ComptonProfile
*/
void ConvertToYSpace::calculateY(double &yspace, double &qspace, double &ei,
                                 const double mass, const double tsec,
                                 const double k1, const double v1,
                                 const DetectorParams &detpar) {
  const double v0 = detpar.l1 / (tsec - detpar.t0 - (detpar.l2 / v1));
  ei = MASS_TO_MEV * v0 * v0;
  const double w = ei - detpar.efixed;
  const double k0 =
      std::sqrt(ei / PhysicalConstants::E_mev_toNeutronWavenumberSq);
  qspace =
      std::sqrt(k0 * k0 + k1 * k1 - 2.0 * k0 * k1 * std::cos(detpar.theta));
  const double wreduced =
      PhysicalConstants::E_mev_toNeutronWavenumberSq * qspace * qspace / mass;
  yspace = 0.2393 * (mass / qspace) * (w - wreduced);
}

//----------------------------------------------------------------------------------------------

/** Initialize the algorithm's properties.
*/
void ConvertToYSpace::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<HistogramValidator>(false); // point data
  wsValidator->add<InstrumentValidator>();
  wsValidator->add<WorkspaceUnitValidator>("TOF");
  declareProperty(new WorkspaceProperty<>("InputWorkspace", "",
                                          Direction::Input, wsValidator),
                  "An input workspace.");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  mustBePositive->setLowerExclusive(true); // strictly greater than 0.0
  declareProperty("Mass", -1.0, mustBePositive,
                  "The mass defining the recoil peak in AMU");

  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
*/
void ConvertToYSpace::exec() {
  retrieveInputs();
  createOutputWorkspace();

  const int64_t nhist = static_cast<int64_t>(m_inputWS->getNumberHistograms());
  const int64_t nreports = nhist;
  auto progress = boost::make_shared<Progress>(this, 0.0, 1.0, nreports);

  PARALLEL_FOR2(m_inputWS, m_outputWS)
  for (int64_t i = 0; i < nhist; ++i) {
    PARALLEL_START_INTERUPT_REGION

    if (!convert(i)) {

      g_log.warning("No detector defined for index=" +
                    boost::lexical_cast<std::string>(i) +
                    ". Zeroing spectrum.");
      m_outputWS->maskWorkspaceIndex(i);
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputWorkspace", m_outputWS);
}

/**
* Convert the spectrum at the given index on the input workspace
* and place the output in the pre-allocated output workspace
* @param index Index on the input & output workspaces giving the spectrum to
* convert
*/
bool ConvertToYSpace::convert(const size_t index) {
  try {
    DetectorParams detPar = getDetectorParameters(m_inputWS, index);
    const double v1 = std::sqrt(detPar.efixed / MASS_TO_MEV);
    const double k1 = std::sqrt(detPar.efixed /
                                PhysicalConstants::E_mev_toNeutronWavenumberSq);

    auto &outX = m_outputWS->dataX(index);
    auto &outY = m_outputWS->dataY(index);
    auto &outE = m_outputWS->dataE(index);
    const auto &inX = m_inputWS->readX(index);
    const auto &inY = m_inputWS->readY(index);
    const auto &inE = m_inputWS->readE(index);

    // The t->y mapping flips the order of the axis so we need to reverse it to
    // have a monotonically
    // increasing axis
    const size_t npts = inY.size();
    for (size_t j = 0; j < npts; ++j) {
      double ys(0.0), qs(0.0), ei(0.0);
      calculateY(ys, qs, ei, m_mass, inX[j] * 1e-06, k1, v1, detPar);
      const size_t outIndex = (npts - j - 1);
      outX[outIndex] = ys;
      const double prefactor = qs / pow(ei, 0.1);
      outY[outIndex] = prefactor * inY[j];
      outE[outIndex] = prefactor * inE[j];
    }
    return true;
  } catch (Exception::NotFoundError &) {
    return false;
  }
}

/**
* Caches input details for the peak information
*/
void ConvertToYSpace::retrieveInputs() {
  m_inputWS = getProperty("InputWorkspace");
  m_mass = getProperty("Mass");
  cacheInstrumentGeometry();
}

/**
* Create & cache output workspaces
*/
void ConvertToYSpace::createOutputWorkspace() {
  m_outputWS = WorkspaceFactory::Instance().create(m_inputWS);
  // Units
  auto xLabel = boost::make_shared<Units::Label>("Momentum", "A^-1");
  m_outputWS->getAxis(0)->unit() = xLabel;
  m_outputWS->setYUnit("");
  m_outputWS->setYUnitLabel("");
}

/**
*/
void ConvertToYSpace::cacheInstrumentGeometry() {
  auto inst = m_inputWS->getInstrument();
  auto source = inst->getSource();
  auto sample = inst->getSample();
  m_l1 = sample->getDistance(*source);
  m_samplePos = sample->getPos();
}

} // namespace CurveFitting
} // namespace Mantid
