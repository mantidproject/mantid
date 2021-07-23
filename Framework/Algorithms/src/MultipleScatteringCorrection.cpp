// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/MultipleScatteringCorrection.h"
#include "MantidAPI/HistoWorkspace.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/MultipleScatteringCorrectionDistGraber.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"

namespace Mantid {
namespace Algorithms {

using namespace API;
using namespace Kernel;
using namespace Mantid::DataObjects;

DECLARE_ALGORITHM(MultipleScatteringCorrection)

/**
 * @brief interface initialisation method
 *
 */
void MultipleScatteringCorrection::init() {
  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<InstrumentValidator>();

  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, wsValidator),
                  "The X values for the input workspace must be in units of wavelength");

  auto positiveInt = std::make_shared<BoundedValidator<int64_t>>();
  positiveInt->setLower(1);
  declareProperty("NumberOfWavelengthPoints", static_cast<int64_t>(EMPTY_INT()), positiveInt,
                  "The number of wavelength points for which the numerical integral is\n"
                  "calculated (default: all points)");

  auto moreThanZero = std::make_shared<BoundedValidator<double>>();
  moreThanZero->setLower(0.001);
  declareProperty("ElementSize", 1.0, moreThanZero, "The size of one side of an integration element cube in mm");

  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>("OutputWorkspace", "", Direction::Output),
                  "Output workspace name. "
                  "A Workspace2D containing the correction matrix that can be directly applied to the corresponding "
                  "Event workspace for multipalce scattering correction.");
}

/**
 * @brief validate the inputs
 *
 * @return std::map<std::string, std::string>
 */
std::map<std::string, std::string> MultipleScatteringCorrection::validateInputs() {
  std::map<std::string, std::string> result;

  // check 0: input workspace must have a valida sample
  m_inputWS = getProperty("InputWorkspace");
  Sample &sample = m_inputWS->mutableSample();
  auto sampleShape = sample.getShape();
  if (!sampleShape.hasValidShape()) {
    result["InputWorkspace"] = "The input workspace must have a valid sample shape";
  }

  // check 1: input workspace must have a valid sample environment
  // TODO: this check should be implemented once we start considering the multiple
  // scattering correction between container and sample (heterogenous scattering media)
  // NOTE: use PaalmanPingAbsorptionCorrection.cpp check as starting point

  // others?

  return result;
}

/**
 * @brief execute the algorithm
 *
 */
void MultipleScatteringCorrection::exec() {
  // parse input properties and assign corresponding values to the member
  // variables
  parseInputs();

  // prepare the cached distances
  MultipleScatteringCorrectionDistGraber distGraber(m_inputWS->sample().getShape(), m_elementSize);
  distGraber.cacheLS1(m_beamDirection);
  // NOTE: the following data is now cached in the distGraber object
  // std::vector<double> distGraber.m_LS1 : Cached L1 distances
  // std::vector<double> distGraber.m_elementVolumes : Cached element volumes
  // std::vector<Kernel::V3D> distGraber.m_elementPositions : Cached element positions
  // size_t distGraber.m_numVolumeElements : The number of volume elements

  // perform integration

  // compute the correction matrix

  // set the output workspace
}

/**
 * @brief parse and assign corresponding values from input properties
 *
 */
void MultipleScatteringCorrection::parseInputs() {
  // Get input workspace
  m_inputWS = getProperty("InputWorkspace");

  // Get the beam direction
  m_beamDirection = m_inputWS->getInstrument()->getBeamDirection();

  // Get the total number of wavelength points, default to use all if not specified
  m_num_lambda = isEmpty(getProperty("NumberOfWavelengthPoints"))
                     ? static_cast<int64_t>(m_inputWS->getNumberHistograms())
                     : getProperty("NumberOfWavelengthPoints");
  // -- while we're here, compute the step in bin number between two adjacent points
  const auto specSize = static_cast<int64_t>(m_inputWS->blocksize());
  m_xStep = std::max(int64_t(1), specSize / m_num_lambda); // Bin step between points to calculate
  // -- notify the user of the bin step
  std::ostringstream msg;
  msg << "Numerical integration performed every " << m_xStep << " wavelength points";
  g_log.information(msg.str());

  // Get the element size
  m_elementSize = getProperty("ElementSize"); // in mm
  m_elementSize = m_elementSize * 1e-3;       // convert to m

  // Get the material
  const auto &sample = m_inputWS->sample();
  // -- process the sample
  m_material = sample.getShape().material();
  // -- process the sample environment (container)
  // TODO:

  // Get total scattering cross-section
  // NOTE: the angstrom^-2 to barns and the angstrom^-1 to cm^-1
  // will cancel for mu to give units: cm^-1
  // -- sample
  m_sampleLinearCoefTotScatt = -m_material.totalScatterXSection() * m_material.numberDensityEffective() * 100;
  // -- container
  // TODO:

  // Create output workspace
  // NOTE: this output workspace is just a Workspace2D of factor that can be applied to the
  // corresponding EventWorkspace.
  // Therefore, it is inherently unitless.
  m_outputWS = create<HistoWorkspace>(*m_inputWS);
  m_outputWS->setYUnit("");          // Need to explicitly set YUnit to nothing
  m_outputWS->setDistribution(true); // The output of this is a distribution
  m_outputWS->setYUnitLabel("Multiple Scattering Correction factor");
  // TODO:
  // We will have to prepare additional output workspace holder for interaction between
  // the sample and the sample environment.
  //
}

} // namespace Algorithms
} // namespace Mantid
