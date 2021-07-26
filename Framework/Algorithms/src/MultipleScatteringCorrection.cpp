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
#include "MantidAPI/SampleValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/MultipleScatteringCorrectionDistGraber.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidHistogramData/Interpolate.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"

namespace Mantid {

namespace Algorithms {

using namespace API;
using namespace Geometry;
using namespace Kernel;
using namespace Mantid::DataObjects;

using HistogramData::interpolateLinearInplace;

DECLARE_ALGORITHM(MultipleScatteringCorrection)

namespace {
// the maximum number of elements to combine at once in the pairwise summation
constexpr size_t MAX_INTEGRATION_LENGTH{1000};

inline size_t findMiddle(const size_t start, const size_t stop) {
  auto half = static_cast<size_t>(floor(.5 * (static_cast<double>(stop - start))));
  return start + half;
}
} // namespace

/**
 * @brief interface initialisation method
 *
 */
void MultipleScatteringCorrection::init() {
  // 1- The input workspace must have an instrument and units of wavelength
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<InstrumentValidator>();
  // 2- The input workspace must have a sample defined (shape and material)
  wsValidator->add<SampleValidator, unsigned int>((SampleValidator::Shape | SampleValidator::Material));

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
                  "Event workspace for multiple scattering correction.");
}

/**
 * @brief validate the inputs
 *
 * @return std::map<std::string, std::string>
 */
std::map<std::string, std::string> MultipleScatteringCorrection::validateInputs() {
  std::map<std::string, std::string> result;

  // check 0: input workspace must have a valida sample
  // NOTE: technically the workspace validator should be able to catch the undefined sample
  //       error.  Keeping this check here in case the validator is changed in the future.
  m_inputWS = getProperty("InputWorkspace");
  const auto &sample = m_inputWS->sample();
  if (!sample.getShape().hasValidShape()) {
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
  size_t numVolumeElements = distGraber.m_numVolumeElements;

  // L2D needs to be calculated w.r.t the detector
  // L12 is independent from the detector, therefore can be cached outside
  // - L12 is a upper off-diagonal matrix
  size_t len_l12 = numVolumeElements * (numVolumeElements - 1) / 2;
  std::vector<double> sample_L12s(len_l12, 0.0);
  for (size_t i = 0; i < numVolumeElements; ++i) {
    for (size_t j = i + 1; j < numVolumeElements; ++j) {
      const V3D dist = distGraber.m_elementPositions[i] - distGraber.m_elementPositions[j];
      sample_L12s[i * numVolumeElements + j] = dist.norm();
    }
  }

  // perform integration
  const auto &spectrumInfo = m_inputWS->spectrumInfo();
  const auto numHists = static_cast<int64_t>(m_inputWS->getNumberHistograms());
  const auto specSize = static_cast<int64_t>(m_inputWS->blocksize());
  Progress prog(this, 0.0, 1.0, numHists);
  // -- loop over the spectra/detectors
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWS, *m_outputWS))
  for (int64_t i = 0; i < numHists; ++i) {
    PARALLEL_START_INTERUPT_REGION

    // copy bins from input workspace to output workspace
    m_outputWS->setSharedX(i, m_inputWS->sharedX(i));

    // locate the spectrum and its detector
    if (!spectrumInfo.hasDetectors(i)) {
      g_log.information() << "Spectrum " << i << " does not have a detector defined for it\n";
      continue;
    }
    const auto &det = spectrumInfo.detector(i);

    // compute L2D
    std::vector<double> sample_L2Ds(numVolumeElements, 0.0);
    calculateL2Ds(distGraber, det, sample_L2Ds);

    const auto wavelengths = m_inputWS->points(i);
    // these need to have the minus sign applied still
    const auto sampleLinearCoefAbs = m_material.linearAbsorpCoef(wavelengths.cbegin(), wavelengths.cend());
    // TODO:
    // - implement the multiple scattering correction for heterogenous media (container)

    auto &output = m_outputWS->mutableY(i);
    // -- loop over the wavelength points every m_xStep
    for (int64_t j = 0; j < specSize; j += m_xStep) {
      double A1 = 0.0;
      double A2 = 0.0;
      pairWiseSum(A1, A2, -sampleLinearCoefAbs[j], distGraber, sample_L2Ds, sample_L12s, 0, numVolumeElements);
      // compute the correction factor
      const double rho = m_material.numberDensityEffective();
      const double sigma_s = m_material.totalScatterXSection();
      const double V = distGraber.m_totalVolume;
      output[j] = rho * V * sigma_s * A2 / (4 * M_PI * A1);
      // Make certain that last point is calculated
      if (m_xStep > 1 && j + m_xStep >= specSize && j + 1 != specSize) {
        j = specSize - m_xStep - 1;
      }
    }

    // Interpolate linearly between points separated by m_xStep,
    // last point required
    if (m_xStep > 1) {
      auto histNew = m_outputWS->histogram(i);
      interpolateLinearInplace(histNew, m_xStep);
      m_outputWS->setHistogram(i, histNew);
    }

    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // set the output workspace
  setProperty("OutputWorkspace", m_outputWS);
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
  m_num_lambda = isDefault("NumberOfWavelengthPoints") ? static_cast<int64_t>(m_inputWS->getNumberHistograms())
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

/**
 * @brief calculate L2D for all detectors
 *
 * @param detector
 * @param sample_L2Ds
 */
void MultipleScatteringCorrection::calculateL2Ds(const MultipleScatteringCorrectionDistGraber &distGraber,
                                                 const IDetector &detector, std::vector<double> &sample_L2Ds) const {
  V3D detectorPos(detector.getPos());
  if (detector.nDets() > 1) {
    // We need to make sure this is right for grouped detectors - should use
    // average theta & phi
    detectorPos.spherical(detectorPos.norm(), detector.getTwoTheta(V3D(), V3D(0, 0, 1)) * 180.0 / M_PI,
                          detector.getPhi() * 180.0 / M_PI);
  }

  // calculate the distance between the detector and the sample
  const auto &sample = m_inputWS->sample();
  const auto &sampleShape = sample.getShape();
  for (size_t i = 0; i < distGraber.m_elementPositions.size(); ++i) {
    const auto &elementPos = distGraber.m_elementPositions[i];
    const V3D direction = normalize(detectorPos - elementPos);
    Track TwoToDetector(elementPos, direction);

    // find distance in sample
    sampleShape.interceptSurface(TwoToDetector);
    sample_L2Ds[i] = TwoToDetector.totalDistInsideObject();
    TwoToDetector.clearIntersectionResults();

    // find distance in container
    // TODO:
  }
}

void MultipleScatteringCorrection::pairWiseSum(double &A1, double &A2, const double linearCoefAbs,
                                               const MultipleScatteringCorrectionDistGraber &distGraber,
                                               const std::vector<double> &L2Ds, const std::vector<double> &L12s,
                                               const size_t startIndex, const size_t endIndex) const {
  if (endIndex - startIndex > MAX_INTEGRATION_LENGTH) {
    size_t middle = findMiddle(startIndex, endIndex);

    // recursive to process upper and lower part
    pairWiseSum(A1, A2, linearCoefAbs, distGraber, L2Ds, L12s, startIndex, middle);
    pairWiseSum(A1, A2, linearCoefAbs, distGraber, L2Ds, L12s, middle, endIndex);
  } else {
    // perform the integration
    const auto &Ls1s = distGraber.m_LS1;
    const auto &elementVolumes = distGraber.m_elementVolumes;
    const auto nElements = distGraber.m_numVolumeElements;
    for (size_t i = startIndex; i < endIndex; ++i) {
      // compute A1
      double exponent = (Ls1s[i] + L2Ds[i]) * linearCoefAbs;
      A1 += exp(exponent) * elementVolumes[i];
      // compute A2
      double a2 = 0.0;
      for (size_t j = 0; j < nElements; ++j) {
        if (i == j) {
          // skip self (second order scattering must happen in a different element)
          continue;
        }
        // L12 is a pre-computed vector, therefore
        // - case1: source -> 2 -> 1 -> det
        //   we don't have 2->1 cached in L12, but we do have L12 cached, which is the same.
        //   therefore, we will use L12[1*nElements + 2] to query the L12 for this case
        size_t idx_l12 = i < j ? i * nElements + j : j * nElements + i;
        exponent = (Ls1s[j] + L12s[idx_l12] + L2Ds[i]) * linearCoefAbs;
        a2 += exp(exponent) * elementVolumes[j] / (L12s[idx_l12] * L12s[idx_l12]);
      }
      A2 += a2 * elementVolumes[i];
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
