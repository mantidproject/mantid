// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Algorithms/ConvertToYSpace.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Unit.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToYSpace)

using namespace API;
using namespace Kernel;

namespace {
/// Conversion constant
const double MASS_TO_MEV =
    0.5 * PhysicalConstants::NeutronMass / PhysicalConstants::meV;
} // namespace

/** Constructor
 */
ConvertToYSpace::ConvertToYSpace()
    : Algorithm(), m_inputWS(), m_mass(0.0), m_l1(0.0), m_samplePos(),
      m_outputWS(), m_qOutputWS() {}

/// Algorithm's name for identification. @see Algorithm::name
const std::string ConvertToYSpace::name() const { return "ConvertToYSpace"; }

/// Algorithm's version for identification. @see Algorithm::version
int ConvertToYSpace::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ConvertToYSpace::category() const {
  return "Transforms\\Units";
}

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

  const auto &spectrumInfo = ws->spectrumInfo();
  if (!spectrumInfo.hasDetectors(index))
    throw std::invalid_argument("ConvertToYSpace - Workspace has no detector "
                                "attached to histogram at index " +
                                std::to_string(index));

  DetectorParams detpar;
  const auto &pmap = ws->constInstrumentParameters();
  const auto &det = spectrumInfo.detector(index);
  detpar.l1 = spectrumInfo.l1();
  detpar.l2 = spectrumInfo.l2(index);
  detpar.pos = spectrumInfo.position(index);
  detpar.theta = spectrumInfo.twoTheta(index);
  detpar.t0 =
      getComponentParameter(det, pmap, "t0") * 1e-6; // Convert to seconds
  detpar.efixed = getComponentParameter(det, pmap, "efixed");
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
double
ConvertToYSpace::getComponentParameter(const Geometry::IComponent &comp,
                                       const Geometry::ParameterMap &pmap,
                                       const std::string &name) {
  double result(0.0);
  if (const auto &group =
          dynamic_cast<const Geometry::DetectorGroup *>(&comp)) {
    double avg(0.0);
    for (const auto &det : group->getDetectors()) {
      auto param = pmap.getRecursive(det->getComponentID(), name);
      if (param)
        avg += param->value<double>();
      else
        throw std::invalid_argument("ComptonProfile - Unable to find "
                                    "DetectorGroup component parameter \"" +
                                    name + "\".");
    }
    result = avg / static_cast<double>(group->nDets());
  } else {
    auto param = pmap.getRecursive(comp.getComponentID(), name);
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
  declareProperty(make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "The input workspace in Time of Flight");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  mustBePositive->setLowerExclusive(true); // strictly greater than 0.0
  declareProperty("Mass", -1.0, mustBePositive,
                  "The mass defining the recoil peak in AMU");

  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The output workspace in y-Space");

  declareProperty(make_unique<WorkspaceProperty<>>("QWorkspace", "",
                                                   Direction::Output,
                                                   PropertyMode::Optional),
                  "The output workspace in q-Space");
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

  auto &spectrumInfo = m_outputWS->mutableSpectrumInfo();
  SpectrumInfo *qSpectrumInfo{nullptr};
  if (m_qOutputWS)
    qSpectrumInfo = &m_qOutputWS->mutableSpectrumInfo();

  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWS, *m_outputWS))
  for (int64_t i = 0; i < nhist; ++i) {
    PARALLEL_START_INTERUPT_REGION

    if (!convert(i)) {
      g_log.warning("No detector defined for index=" + std::to_string(i) +
                    ". Zeroing spectrum.");
      m_outputWS->getSpectrum(i).clearData();
      PARALLEL_CRITICAL(setMasked) {
        spectrumInfo.setMasked(i, true);
        if (m_qOutputWS) {
          m_qOutputWS->getSpectrum(i).clearData();
          qSpectrumInfo->setMasked(i, true);
        }
      }
    }
    progress->report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputWorkspace", m_outputWS);

  if (m_qOutputWS)
    setProperty("QWorkspace", m_qOutputWS);
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

    auto &outX = m_outputWS->mutableX(index);
    auto &outY = m_outputWS->mutableY(index);
    auto &outE = m_outputWS->mutableE(index);
    const auto &inX = m_inputWS->x(index);
    const auto &inY = m_inputWS->y(index);
    const auto &inE = m_inputWS->e(index);

    // The t->y mapping flips the order of the axis so we need to reverse it to
    // have a monotonically increasing axis
    const size_t npts = inY.size();
    for (size_t j = 0; j < npts; ++j) {
      double ys(0.0), qs(0.0), ei(0.0);
      calculateY(ys, qs, ei, m_mass, inX[j] * 1e-06, k1, v1, detPar);
      const size_t outIndex = (npts - j - 1);
      outX[outIndex] = ys;
      const double prefactor = qs / pow(ei, 0.1);
      outY[outIndex] = prefactor * inY[j];
      outE[outIndex] = prefactor * inE[j];

      if (m_qOutputWS) {
        m_qOutputWS->mutableX(index)[outIndex] = ys;
        m_qOutputWS->mutableY(index)[outIndex] = qs;
      }
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
  // y-Space output workspace
  m_outputWS = WorkspaceFactory::Instance().create(m_inputWS);

  auto xLabel = boost::make_shared<Units::Label>("Momentum", "A^-1");
  m_outputWS->getAxis(0)->unit() = xLabel;
  m_outputWS->setYUnit("");
  m_outputWS->setYUnitLabel("");

  // q-Space output workspace
  if (!getPropertyValue("QWorkspace").empty()) {
    m_qOutputWS = WorkspaceFactory::Instance().create(m_inputWS);

    m_qOutputWS->getAxis(0)->unit() = xLabel;
    m_qOutputWS->setYUnit("");
    m_qOutputWS->setYUnitLabel("");
  }
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

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
