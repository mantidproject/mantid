// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ConvertToConstantL2.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Strings.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <boost/format.hpp>
#include <cmath>

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace Geometry;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(ConvertToConstantL2)

// Constructor
ConvertToConstantL2::ConvertToConstantL2()
    : API::Algorithm(), m_inputWS(), m_outputWS(), m_instrument(), m_l2(0.),
      m_wavelength(0.) {}

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void ConvertToConstantL2::init() {
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
void ConvertToConstantL2::initWorkspaces() {
  // Get the workspaces
  m_inputWS = this->getProperty("InputWorkspace");
  m_outputWS = this->getProperty("OutputWorkspace");
  m_instrument = m_inputWS->getInstrument();
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
void ConvertToConstantL2::exec() {

  initWorkspaces();

  // Calculate the number of spectra in this workspace
  const size_t numberOfSpectra = m_inputWS->getNumberHistograms();
  API::Progress prog(this, 0.0, 1.0, numberOfSpectra);

  int64_t numberOfSpectra_i =
      static_cast<int64_t>(numberOfSpectra); // cast to make openmp happy

  const auto &inputSpecInfo = m_inputWS->spectrumInfo();
  auto &outputDetInfo = m_outputWS->mutableDetectorInfo();

  // Loop over the histograms (detector spectra)
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWS, *m_outputWS))
  for (int64_t i = 0; i < numberOfSpectra_i; ++i) {
    PARALLEL_START_INTERUPT_REGION
    m_outputWS->setHistogram(i, m_inputWS->histogram(i));

    // Should not move the monitors
    if (inputSpecInfo.isMonitor(i))
      continue;

    // Throw if detector doesn't exist or is a group
    if (!inputSpecInfo.hasUniqueDetector(i)) {
      const auto errorMsg =
          boost::format("The detector for spectrum number %d was either not "
                        "found, or is a group.") %
          i;
      throw std::runtime_error(errorMsg.str());
    }

    // subract the diference in l2
    double thisDetL2 = inputSpecInfo.l2(i);
    double deltaL2 = std::abs(thisDetL2 - m_l2);
    double deltaTOF = calculateTOF(deltaL2);
    deltaTOF *= 1e6; // micro sec

    // position - set all detector distance to constant l2
    double r, theta, phi;
    V3D oldPos = inputSpecInfo.position(i);
    oldPos.getSpherical(r, theta, phi);
    V3D newPos;
    newPos.spherical(m_l2, theta, phi);

    const auto detIndex = inputSpecInfo.spectrumDefinition(i)[0];
    outputDetInfo.setPosition(detIndex, newPos);

    m_outputWS->mutableX(i) -= deltaTOF;

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
double ConvertToConstantL2::getRunProperty(std::string s) {
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
double ConvertToConstantL2::getInstrumentProperty(std::string s) {
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
double ConvertToConstantL2::calculateTOF(double distance) {
  double velocity = PhysicalConstants::h / (PhysicalConstants::NeutronMass *
                                            m_wavelength * 1e-10); // m/s

  return distance / velocity;
}

} // namespace Algorithms
} // namespace Mantid
