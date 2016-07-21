//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CorrectFlightPaths.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/UnitFactory.h"

#include <cmath>

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace Geometry;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(CorrectFlightPaths)

// Constructor
CorrectFlightPaths::CorrectFlightPaths()
    : API::Algorithm(), m_inputWS(), m_outputWS(), m_instrument(), m_sample(),
      m_l2(0.), m_wavelength(0.) {}

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void CorrectFlightPaths::init() {

  // todo: add validator for TOF

  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("TOF");
  wsValidator->add<HistogramValidator>();
  declareProperty(make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "Name of the input workspace");
  declareProperty(make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace, can be the same as the input");
}

/**
 * Initialises input and output workspaces.
 *
 */
void CorrectFlightPaths::initWorkspaces() {
  // Get the workspaces
  m_inputWS = this->getProperty("InputWorkspace");
  m_outputWS = this->getProperty("OutputWorkspace");
  m_instrument = m_inputWS->getInstrument();
  m_sample = m_instrument->getSample();
  // If input and output workspaces are not the same, create a new workspace for
  // the output
  if (m_outputWS != this->m_inputWS) {
    m_outputWS = API::WorkspaceFactory::Instance().create(m_inputWS);
  }

  m_wavelength = getRunProperty("wavelength");
  g_log.debug() << "Wavelength = " << m_wavelength;
  m_l2 = getInstrumentProperty("l2");
  g_log.debug() << " L2 = " << m_l2 << '\n';
}

/**
 * Executes the algorithm
 *
 */
void CorrectFlightPaths::exec() {

  initWorkspaces();

  Geometry::ParameterMap &pmap = m_outputWS->instrumentParameters();

  const size_t numberOfChannels = this->m_inputWS->blocksize();
  // Calculate the number of spectra in this workspace
  const int numberOfSpectra =
      static_cast<int>(this->m_inputWS->size() / numberOfChannels);
  API::Progress prog(this, 0.0, 1.0, numberOfSpectra);

  int64_t numberOfSpectra_i =
      static_cast<int64_t>(numberOfSpectra); // cast to make openmp happy

  // Loop over the histograms (detector spectra)

  PARALLEL_FOR2(m_inputWS, m_outputWS)
  for (int64_t i = 0; i < numberOfSpectra_i; ++i) {
    PARALLEL_START_INTERUPT_REGION
    m_outputWS->setHistogram(i, m_outputWS->histogram(i));
    // Copy the energy transfer axis
    // TOF

    // subract the diference in l2
    IDetector_const_sptr det = m_inputWS->getDetector(i);
    double thisDetL2 = det->getDistance(*m_sample);
    double deltaL2 = std::abs(thisDetL2 - m_l2);
    double deltaTOF = calculateTOF(deltaL2);
    deltaTOF *= 1e6; // micro sec

    // position - set all detector distance to constant l2
    double r, theta, phi;
    V3D oldPos = det->getPos();
    oldPos.getSpherical(r, theta, phi);
    V3D newPos;
    newPos.spherical(m_l2, theta, phi);
    ComponentHelper::moveComponent(*det, pmap, newPos,
                                   ComponentHelper::Absolute);

    m_outputWS->mutableX(i) += -deltaTOF;

    prog.report("Aligning elastic line...");
    PARALLEL_END_INTERUPT_REGION
  } // end for i
  PARALLEL_CHECK_INTERUPT_REGION

  this->setProperty("OutputWorkspace", this->m_outputWS);
}

/*
 * Get run property as double
 * @s - input property name
 *
 */
double CorrectFlightPaths::getRunProperty(std::string s) {
  Mantid::Kernel::Property *prop = m_inputWS->run().getProperty(s);
  double val;
  if (!prop || !Strings::convert(prop->value(), val)) {
    std::string mesg = "Run property " + s + "doesn't exist!";
    g_log.error(mesg);
    throw std::runtime_error(mesg);
  }
  return val;
}
/*
 * Get instrument property as double
 * @s - input property name
 *
 */
double CorrectFlightPaths::getInstrumentProperty(std::string s) {
  std::vector<std::string> prop = m_instrument->getStringParameter(s);
  if (prop.empty()) {
    std::string mesg = "Property <" + s + "> doesn't exist!";
    g_log.error(mesg);
    throw std::runtime_error(mesg);
  }
  g_log.debug() << "prop[0] = " << prop[0] << '\n';
  return boost::lexical_cast<double>(prop[0]);
}

/*
 * Returns the neutron TOF
 * @distance - Distance in meters
 */
double CorrectFlightPaths::calculateTOF(double distance) {
  double velocity = PhysicalConstants::h / (PhysicalConstants::NeutronMass *
                                            m_wavelength * 1e-10); // m/s

  return distance / velocity;
}

} // namespace Algorithm
} // namespace Mantid
