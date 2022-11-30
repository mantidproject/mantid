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
#include "MantidKernel/ListValidator.h"

namespace Mantid::Algorithms {

using namespace API;
using namespace Geometry;
using namespace Kernel;
using namespace Mantid::DataObjects;

using HistogramData::interpolateLinearInplace;

DECLARE_ALGORITHM(MultipleScatteringCorrection)

namespace {
// the maximum number of elements to combine at once in the pairwise summation
constexpr int64_t MAX_INTEGRATION_LENGTH{1000};

static constexpr double RAD2DEG = 180.0 / M_PI; // save some flops??

inline int64_t findMiddle(const int64_t start, const int64_t stop) {
  auto half = static_cast<int64_t>(floor(.5 * (static_cast<double>(stop - start))));
  return start + half;
}

inline size_t calcLinearIdxFromUpperTriangular(const size_t N, const size_t row_idx, const size_t col_idx) {
  // calculate the linear index from the upper triangular matrix from a (N x N) matrix
  // row_idx < col_idx due to upper triangular matrix
  assert(row_idx < col_idx); // only relevant during Debug build
  return N * (N - 1) / 2 - (N - row_idx) * (N - row_idx - 1) / 2 + col_idx - row_idx - 1;
}

// being added to make it clearer that we are creating a unit vector
inline const V3D getDirection(const V3D &posInitial, const V3D &posFinal) { return normalize(posFinal - posInitial); }

// make code slightly clearer
inline double getDistanceInsideObject(const IObject &shape, Track &track) {
  if (shape.interceptSurface(track) > 0) {
    return track.totalDistInsideObject();
  } else {
    return 0.0;
  }
}

inline double checkzero(const double x) { return std::abs(x) < std::numeric_limits<float>::min() ? 0.0 : x; }
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

  declareProperty("ContainerElementSize", EMPTY_DBL(),
                  "The size of one side of an integration element cube in mm for container."
                  "Default to be the same as ElementSize.");

  std::vector<std::string> methodOptions{"SampleOnly", "SampleAndContainer"};
  declareProperty("Method", "SampleOnly", std::make_shared<StringListValidator>(methodOptions),
                  "Correction method, use either SampleOnly or SampleAndContainer.");

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
  std::string method = getProperty("Method");
  if (method == "SampleAndContainer") {
    const auto &containerShape = m_inputWS->sample().getEnvironment().getContainer();
    if (!containerShape.hasValidShape()) {
      result["Method"] = "SampleAndContainer requires a valid container shape.";
    }
  }

  return result;
}

/**
 * @brief execute the algorithm
 *
 */
void MultipleScatteringCorrection::exec() {
  // Parse input properties and assign corresponding values to the member
  // variables
  parseInputs();

  std::string method = getProperty("Method");
  if (method == "SampleOnly") {
    //-- Setup output workspace
    // set the OutputWorkspace to a workspace group with one workspace:
    //                 ${OutputWorkspace}_sampleOnly
    API::MatrixWorkspace_sptr ws_sampleOnly = create<HistoWorkspace>(*m_inputWS);
    ws_sampleOnly->setYUnit("");          // Need to explicitly set YUnit to nothing
    ws_sampleOnly->setDistribution(true); // The output of this is a distribution
    ws_sampleOnly->setYUnitLabel("Multiple Scattering Correction factor");
    //-- Fill the workspace with sample only correction factors
    const auto &sampleShape = m_inputWS->sample().getShape();
    calculateSingleComponent(ws_sampleOnly, sampleShape, m_sampleElementSize);
    //-- Package output to workspace group
    const std::string outWSName = getProperty("OutputWorkspace");
    std::vector<std::string> names;
    names.emplace_back(outWSName + "_sampleOnly");
    API::AnalysisDataService::Instance().addOrReplace(names.back(), ws_sampleOnly);
    // group
    auto group = createChildAlgorithm("GroupWorkspaces");
    group->initialize();
    group->setProperty("InputWorkspaces", names);
    group->setProperty("OutputWorkspace", outWSName);
    group->execute();
    API::WorkspaceGroup_sptr outWS = group->getProperty("OutputWorkspace");
    // NOTE:
    //   The output here is a workspace group of one, and it is an intended design as
    //   the MantidTotalScattering would like to have consistent output type regardless
    //   of the correction method.
    setProperty("OutputWorkspace", outWS);
  } else if (method == "SampleAndContainer") {
    //-- Setup output workspace
    // set the OutputWorkspace to a workspace group with two workspaces:
    //          ${OutputWorkspace}_containerOnly
    //          ${OutputWorkspace}_sampleAndContainer
    // 1. container only
    API::MatrixWorkspace_sptr ws_containerOnly = create<HistoWorkspace>(*m_inputWS);
    ws_containerOnly->setYUnit("");          // Need to explicitly set YUnit to nothing
    ws_containerOnly->setDistribution(true); // The output of this is a distribution
    ws_containerOnly->setYUnitLabel("Multiple Scattering Correction factor");
    const auto &containerShape = m_inputWS->sample().getEnvironment().getContainer();
    calculateSingleComponent(ws_containerOnly, containerShape, m_containerElementSize);
    // 2. sample and container
    API::MatrixWorkspace_sptr ws_sampleAndContainer = create<HistoWorkspace>(*m_inputWS);
    ws_sampleAndContainer->setYUnit("");          // Need to explicitly set YUnit to nothing
    ws_sampleAndContainer->setDistribution(true); // The output of this is a distribution
    ws_sampleAndContainer->setYUnitLabel("Multiple Scattering Correction factor");
    calculateSampleAndContainer(ws_sampleAndContainer);
    //-- Package output to workspace group
    const std::string outWSName = getProperty("OutputWorkspace");
    std::vector<std::string> names;
    names.emplace_back(outWSName + "_containerOnly");
    API::AnalysisDataService::Instance().addOrReplace(names.back(), ws_containerOnly);
    names.emplace_back(outWSName + "_sampleAndContainer");
    API::AnalysisDataService::Instance().addOrReplace(names.back(), ws_sampleAndContainer);
    // group
    auto group = createChildAlgorithm("GroupWorkspaces");
    group->initialize();
    group->setProperty("InputWorkspaces", names);
    group->setProperty("OutputWorkspace", outWSName);
    group->execute();
    API::WorkspaceGroup_sptr outWS = group->getProperty("OutputWorkspace");
    //
    setProperty("OutputWorkspace", outWS);
  } else {
    // With validator guarding the gate, this should never happen. However, just incase it
    // does, we should throw an exception.
    throw std::invalid_argument("Invalid method: " + method);
  }
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
  m_num_lambda = isDefault("NumberOfWavelengthPoints") ? static_cast<int64_t>(m_inputWS->blocksize())
                                                       : getProperty("NumberOfWavelengthPoints");
  // -- while we're here, compute the step in bin number between two adjacent points
  const auto specSize = static_cast<int64_t>(m_inputWS->blocksize());
  m_xStep = std::max(int64_t(1), specSize / m_num_lambda); // Bin step between points to calculate
  // -- notify the user of the bin step
  std::ostringstream msg;
  msg << "Numerical integration performed every " << m_xStep << " wavelength points";
  g_log.information(msg.str());
  g_log.information() << msg.str();

  // Get the element size
  m_sampleElementSize = getProperty("ElementSize"); // in mm
  m_sampleElementSize = m_sampleElementSize * 1e-3; // convert to m
  m_containerElementSize = getProperty("ContainerElementSize");
  m_containerElementSize = isDefault("ContainerElementSize") ? m_sampleElementSize : m_containerElementSize * 1e-3;
}

/**
 * @brief calculate the correction factor per detector for sample only case
 *
 * @param outws
 * @param shape
 * @param elementSize length of cube size used to rasterize given shape
 */
void MultipleScatteringCorrection::calculateSingleComponent(const API::MatrixWorkspace_sptr &outws,
                                                            const Geometry::IObject &shape, const double elementSize) {
  const auto material = shape.material();
  // Cache distances
  // NOTE: cannot use IObject_sprt for sample shape as the getShape() method dereferenced
  //       the shared pointer upon returning.
  MultipleScatteringCorrectionDistGraber distGraber(shape, elementSize);
  distGraber.cacheLS1(m_beamDirection);

  const int64_t numVolumeElements = distGraber.m_numVolumeElements;

  // calculate distance within material from source to scattering point
  std::vector<double> LS1s(numVolumeElements, 0.0);
  calculateLS1s(distGraber, LS1s, shape);

  // L12 is independent from the detector, therefore can be cached outside
  // - L12 is a upper off-diagonal matrix
  // NOTE: if the sample size/volume is too large, we might need to use openMP
  //       to parallelize the calculation
  const int64_t len_l12 = numVolumeElements * (numVolumeElements - 1) / 2;
  std::vector<double> L12s(len_l12, 0.0);
  calculateL12s(distGraber, L12s, shape);

  // L2D needs to be calculated w.r.t the detector

  // compute the prefactor for multiple scattering correction factor Delta
  // Delta = totScatterCoeff * A2/A1
  //  NOTE: Unit is important
  // rho in 1/A^3, and sigma_s in 1/barns (1e-8 A^(-2))
  // so rho * sigma_s = 1e-8 A^(-1) = 100 meters
  // A2/A1 gives length in meters
  const double rho = material.numberDensityEffective();
  const double sigma_s = material.totalScatterXSection();
  const double unit_scaling = 1e2;
  const double totScatterCoeff = rho * sigma_s * unit_scaling;

  // Calculate one detector at a time
  const auto &spectrumInfo = m_inputWS->spectrumInfo();
  const auto numHists = static_cast<int64_t>(m_inputWS->getNumberHistograms());
  const auto specSize = static_cast<int64_t>(m_inputWS->blocksize());
  Progress prog(this, 0.0, 1.0, numHists);
  // -- loop over the spectra/detectors
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWS, *outws))
  for (int64_t workspaceIndex = 0; workspaceIndex < numHists; ++workspaceIndex) {
    PARALLEL_START_INTERRUPT_REGION
    // locate the spectrum and its detector
    if (!spectrumInfo.hasDetectors(workspaceIndex)) {
      g_log.information() << "Spectrum " << workspaceIndex << " does not have a detector defined for it\n";
      continue;
    }
    const auto &det = spectrumInfo.detector(workspaceIndex);

    // compute L2D
    std::vector<double> L2Ds(numVolumeElements, 0.0);
    calculateL2Ds(distGraber, det, L2Ds, shape);

    const auto wavelengths = m_inputWS->points(workspaceIndex);
    // these need to have the minus sign applied still

    const auto LinearCoefAbs = material.linearAbsorpCoef(wavelengths.cbegin(), wavelengths.cend());

    auto &output = outws->mutableY(workspaceIndex);
    // -- loop over the wavelength points every m_xStep
    for (int64_t wvBinsIndex = 0; wvBinsIndex < specSize; wvBinsIndex += m_xStep) {
      double A1 = 0.0;
      double A2 = 0.0;

      pairWiseSum(A1, A2, -LinearCoefAbs[wvBinsIndex], distGraber, LS1s, L12s, L2Ds, 0, numVolumeElements);

      // compute the correction factor
      // NOTE: prefactor, totScatterCoeff, is pre-calculated outside the loop (see above)
      output[wvBinsIndex] = totScatterCoeff / (4 * M_PI) * (A2 / A1);

      // debug output
#ifndef NDEBUG
      std::ostringstream msg_debug;
      msg_debug << "Det_" << workspaceIndex << "@spectrum_" << wvBinsIndex << '\n'
                << "\trho = " << rho << ", sigma_s = " << sigma_s << '\n'
                << "\tA1 = " << A1 << '\n'
                << "\tA2 = " << A2 << '\n'
                << "\tms_factor = " << output[wvBinsIndex] << '\n';
      g_log.notice(msg_debug.str());
#endif

      // Make certain that last point is calculated
      if (m_xStep > 1 && wvBinsIndex + m_xStep >= specSize && wvBinsIndex + 1 != specSize) {
        wvBinsIndex = specSize - m_xStep - 1;
      }
    }

    // Interpolate linearly between points separated by m_xStep,
    // last point required
    if (m_xStep > 1) {
      auto histNew = outws->histogram(workspaceIndex);
      interpolateLinearInplace(histNew, m_xStep);
      outws->setHistogram(workspaceIndex, histNew);
    }

    prog.report();

    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  g_log.notice() << "finished integration.\n";
}

/**
 * @brief calculate the multiple scattering factor (0, 1) for sample and container case
 *
 * @param outws pointer for workspace containing the multiple scattering correction factor
 *              for each detector.
 */
void MultipleScatteringCorrection::calculateSampleAndContainer(const API::MatrixWorkspace_sptr &outws) {
  // retrieve container related properties as they are relevant now
  const auto &sample = m_inputWS->sample();
  const auto &sampleMaterial = sample.getShape().material();
  const auto &containerMaterial = sample.getEnvironment().getContainer().material();

  // get the sample and container shapes
  const auto &sampleShape = sample.getShape();
  const auto &containerShape = sample.getEnvironment().getContainer();

  MultipleScatteringCorrectionDistGraber distGraberSample(sampleShape, m_sampleElementSize);
  distGraberSample.cacheLS1(m_beamDirection);
  MultipleScatteringCorrectionDistGraber distGraberContainer(containerShape, m_containerElementSize);
  distGraberContainer.cacheLS1(m_beamDirection);

  // useful info to have
  const int64_t numVolumeElementsSample = distGraberSample.m_numVolumeElements;
  const int64_t numVolumeElementsContainer = distGraberContainer.m_numVolumeElements;
  const int64_t numVolumeElements = numVolumeElementsSample + numVolumeElementsContainer;
  g_log.information() << "numVolumeElementsSample=" << numVolumeElementsSample
                      << ", numVolumeElementsContainer=" << numVolumeElementsContainer << "\n";

  // Total elements = [container_elements] + [sample_elements]
  // Schematic for scattering element i (*)
  //   |                       \                                        /                       |
  //   |      container         \               sample                 /  container             |
  //   |                         \                                    /                         |
  //   | ---LS1_container[i] ---  \  LS1_sample[i] * L2D_sample[i]   / ---L2D_container[i]  --- |
  //   |                           \                                /                           |

  // LS1 can be cached here, but L2D must be calculated within the loop of spectra
  std::vector<double> LS1_container(numVolumeElements, 0.0);
  std::vector<double> LS1_sample(numVolumeElements, 0.0);
  calculateLS1s(distGraberContainer, distGraberSample, LS1_container, LS1_sample, containerShape, sampleShape);

  // cache L12 for both sample and container
  // L12 is a upper off-diagonal matrix from the hybrid of container and sample.
  // e.g. container: i, j, k
  //      sample: l, m, n
  //      hybrid: i, j, k, l, m, n
  //      L12:
  //               i    j      k      l        m       n
  //      -------------------------------------------------
  //      i     |  x   L12[0] L12[1] L12[2]  L12[3]  L12[4]
  //      j     |  x    x     L12[5] L12[6]  L12[7]  L12[8]
  //      k     |  x    x      x     L12[9]  L12[10] L12[11]
  //      l     |  x    x      x      x      L12[12] L12[13]
  //      m     |  x    x      x      x       x      L12[14]
  //      n     |  x    x      x      x       x       x
  const int64_t len_l12 = numVolumeElements * (numVolumeElements - 1) / 2;
  std::vector<double> L12_container(len_l12, 0.0);
  std::vector<double> L12_sample(len_l12, 0.0);
  calculateL12s(distGraberContainer, distGraberSample, L12_container, L12_sample, containerShape, sampleShape);
#ifndef NDEBUG
  for (size_t i = 0; i < size_t(numVolumeElements); ++i) {
    for (size_t j = i + 1; j < size_t(numVolumeElements); ++j) {
      const auto idx = calcLinearIdxFromUpperTriangular(numVolumeElements, i, j);
      const auto l12 = L12_container[idx] + L12_sample[idx];
      if (l12 < 1e-9) {
        g_log.notice() << "L12_container(" << i << "," << j << ")=" << L12_container[idx] << '\n'
                       << "L12_sample(" << i << "," << j << ")=" << L12_sample[idx] << '\n';
      }
    }
  }
#endif

  // cache the elementsVolumes
  std::vector<double> elementVolumes(distGraberContainer.m_elementVolumes.begin(),
                                     distGraberContainer.m_elementVolumes.end());
  elementVolumes.insert(elementVolumes.end(), distGraberSample.m_elementVolumes.begin(),
                        distGraberSample.m_elementVolumes.end());
#ifndef NDEBUG
  for (size_t i = 0; i < elementVolumes.size(); ++i) {
    if (elementVolumes[i] < 1e-16) {
      g_log.notice() << "Element_" << i << " has near zero volume: " << elementVolumes[i] << '\n';
    }
  }
  g_log.notice() << "V_container = "
                 << std::accumulate(distGraberContainer.m_elementVolumes.begin(),
                                    distGraberContainer.m_elementVolumes.end(), 0.0)
                 << '\n'
                 << "V_sample = "
                 << std::accumulate(distGraberSample.m_elementVolumes.begin(), distGraberSample.m_elementVolumes.end(),
                                    0.0)
                 << '\n';
#endif

  // NOTE: Unit is important
  // rho in 1/A^3, and sigma_s in 1/barns (1e-8 A^(-2))
  // so rho * sigma_s = 1e-8 A^(-1) = 100 meters
  // A2/A1 gives length in meters
  const double unit_scaling = 1e2;
  const double rho_sample = sampleMaterial.numberDensityEffective();
  const double sigma_s_sample = sampleMaterial.totalScatterXSection();
  const double rho_container = containerMaterial.numberDensityEffective();
  const double sigma_s_container = containerMaterial.totalScatterXSection();
  const double totScatterCoef_container = rho_container * sigma_s_container * unit_scaling;
  const double totScatterCoef_sample = rho_sample * sigma_s_sample * unit_scaling;

  // Compute the multiple scattering factor: one detector at a time
  const auto &spectrumInfo = m_inputWS->spectrumInfo();
  const auto numHists = static_cast<int64_t>(m_inputWS->getNumberHistograms());
  const auto specSize = static_cast<int64_t>(m_inputWS->blocksize());
  Progress prog(this, 0.0, 1.0, numHists);
  // -- loop over the spectra/detectors
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWS, *outws))
  for (int64_t workspaceIndex = 0; workspaceIndex < numHists; ++workspaceIndex) {
    PARALLEL_START_INTERRUPT_REGION
    // locate the spectrum and its detector
    if (!spectrumInfo.hasDetectors(workspaceIndex)) {
      g_log.information() << "Spectrum " << workspaceIndex << " does not have a detector defined for it\n";
      continue;
    }
    const auto &det = spectrumInfo.detector(workspaceIndex);

    // calculate L2D (2_element -> detector)
    // 1. container
    std::vector<double> L2D_container(numVolumeElements, 0.0);
    std::vector<double> L2D_sample(numVolumeElements, 0.0);
    calculateL2Ds(distGraberContainer, distGraberSample, det, L2D_container, L2D_sample, containerShape, sampleShape);

    // prepare material wise linear coefficient
    const auto wavelengths = m_inputWS->points(workspaceIndex);
    const auto sampleLinearCoefAbs = sampleMaterial.linearAbsorpCoef(wavelengths.cbegin(), wavelengths.cend());
    const auto containerLinearCoefAbs = containerMaterial.linearAbsorpCoef(wavelengths.cbegin(), wavelengths.cend());

    auto &output = outws->mutableY(workspaceIndex);
    for (int64_t wvBinsIndex = 0; wvBinsIndex < specSize; wvBinsIndex += m_xStep) {
      double A1 = 0.0;
      double A2 = 0.0;

      // compute the multiple scattering correction factor Delta
      pairWiseSum(A1, A2,                                                                  //  output values
                  -containerLinearCoefAbs[wvBinsIndex], -sampleLinearCoefAbs[wvBinsIndex], //  absorption coefficient
                  numVolumeElementsContainer, numVolumeElements,   //  number of elements for checking type
                  totScatterCoef_container, totScatterCoef_sample, //  volumes
                  elementVolumes,                                  //  source -> 1st scattering element
                  LS1_container, LS1_sample,                       //  1st -> 2nd scattering element
                  L12_container, L12_sample,                       //  2nd scattering element -> detector
                  L2D_container, L2D_sample,                       //  starting element idx, ending element idx
                  0, numVolumeElements);

      output[wvBinsIndex] = (A2 / A1) / (4.0 * M_PI);

      // debug output
#ifndef NDEBUG
      std::ostringstream msg_debug;
      msg_debug << "Det_" << workspaceIndex << "@spectrum_" << wvBinsIndex << '\n'
                << "-containerLinearCoefAbs[wvBinsIndex] = " << -containerLinearCoefAbs[wvBinsIndex] << '\n'
                << "-sampleLinearCoefAbs[wvBinsIndex] = " << -sampleLinearCoefAbs[wvBinsIndex] << '\n'
                << "numVolumeElementsContainer = " << numVolumeElementsContainer << '\n'
                << "numVolumeElements = " << numVolumeElements << '\n'
                << "totScatterCoef_container = " << totScatterCoef_container << '\n'
                << "totScatterCoef_sample = " << totScatterCoef_sample << '\n'
                << "\tA1 = " << A1 << '\n'
                << "\tA2 = " << A2 << '\n'
                << "\tms_factor = " << output[wvBinsIndex] << '\n';
      g_log.notice(msg_debug.str());
#endif

      // Make certain that last point is calculated
      if (m_xStep > 1 && wvBinsIndex + m_xStep >= specSize && wvBinsIndex + 1 != specSize) {
        wvBinsIndex = specSize - m_xStep - 1;
      }
    }
    // Interpolate linearly between points separated by m_xStep,
    // last point required
    if (m_xStep > 1) {
      auto histNew = outws->histogram(workspaceIndex);
      interpolateLinearInplace(histNew, m_xStep);
      outws->setHistogram(workspaceIndex, histNew);
    }
    prog.report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
  g_log.notice() << "finished integration.\n";
}

/**
 * @brief compute LS1s within given shape for single component case
 *
 * @param distGraber Pointer to distGraber instance that does the heavy lifting for discretization
 * @param LS1s Vector to store source -> 1st scattering element distance within material
 * @param shape Object shape that defines the boundary of material
 */
void MultipleScatteringCorrection::calculateLS1s(const MultipleScatteringCorrectionDistGraber &distGraber, //
                                                 std::vector<double> &LS1s,                                //
                                                 const Geometry::IObject &shape) const {
  const auto &sourcePos = m_inputWS->getInstrument()->getSource()->getPos();
  const int64_t numVolumeElements = distGraber.m_numVolumeElements;
  Track trackerLS1(V3D{0, 0, 1}, V3D{0, 0, 1});

  for (int64_t idx = 0; idx < numVolumeElements; ++idx) {
    const auto pos = distGraber.m_elementPositions[idx];
    const auto vec = getDirection(pos, sourcePos);
    //
    trackerLS1.reset(pos, vec);
    trackerLS1.clearIntersectionResults();
    LS1s[idx] = getDistanceInsideObject(shape, trackerLS1);
  }
}

/**
 * @brief compute LS1s within given shape for sample and container case
 *
 * @param distGraberContainer Pointer of distGraber that helps discretize container
 * @param distGraberSample Pointer of distGraber that helps discretize sample
 * @param LS1sContainer Vector to store source -> 1st scattering element distance within container
 * @param LS1sSample Vector to store source -> 1st scattering element distance within sample
 * @param shapeContainer Pointer of shape object defines the container
 * @param shapeSample Pointer of shape object defines the sample
 */
void MultipleScatteringCorrection::calculateLS1s(const MultipleScatteringCorrectionDistGraber &distGraberContainer, //
                                                 const MultipleScatteringCorrectionDistGraber &distGraberSample,    //
                                                 std::vector<double> &LS1sContainer,                                //
                                                 std::vector<double> &LS1sSample,                                   //
                                                 const Geometry::IObject &shapeContainer,                           //
                                                 const Geometry::IObject &shapeSample) const {
  // Total elements = [container_elements] + [sample_elements]
  // Schematic for scattering element i (*)
  //   |                       \                                        /                       |
  //   |      container         \               sample                 /  container             |
  //   |                         \                                    /                         |
  //   | ---LS1_container[i] ---  \  LS1_sample[i] * L2D_sample[i]   / ---L2D_container[i]  --- |
  //   |                           \                                /                           |
  const int64_t numVolumeElementsSample = distGraberSample.m_numVolumeElements;
  const int64_t numVolumeElementsContainer = distGraberContainer.m_numVolumeElements;
  const int64_t numVolumeElements = numVolumeElementsSample + numVolumeElementsContainer;
  const auto sourcePos = m_inputWS->getInstrument()->getSource()->getPos();
  Track trackerLS1(V3D{0, 0, 1}, V3D{0, 0, 1}); // reusable tracker for calculating LS1
  for (int64_t idx = 0; idx < numVolumeElements; ++idx) {
    const auto pos = idx < numVolumeElementsContainer
                         ? distGraberContainer.m_elementPositions[idx]
                         : distGraberSample.m_elementPositions[idx - numVolumeElementsContainer];
    const auto vec = getDirection(pos, sourcePos);
    //
    trackerLS1.reset(pos, vec);
    trackerLS1.clearIntersectionResults();
    LS1sContainer[idx] = getDistanceInsideObject(shapeContainer, trackerLS1);
    trackerLS1.reset(pos, vec);
    trackerLS1.clearIntersectionResults();
    LS1sSample[idx] = getDistanceInsideObject(shapeSample, trackerLS1);
#ifndef NDEBUG
    // debug
    std::ostringstream msg_debug;
    msg_debug << "idx=" << idx << ", pos=" << pos << ", vec=" << vec << '\n';
    if (idx < numVolumeElementsContainer) {
      msg_debug << "Container element " << idx << '\n';
    } else {
      msg_debug << "Sample element " << idx - numVolumeElementsContainer << '\n';
    }
    msg_debug << "LS1_container=" << LS1sContainer[idx] << ", LS1_sample=" << LS1sSample[idx] << '\n';
    g_log.notice(msg_debug.str());
#endif
  }
}

/**
 * @brief calculate L12 for single component case
 *
 * @param distGraber
 * @param L12s
 * @param shape
 */
void MultipleScatteringCorrection::calculateL12s(const MultipleScatteringCorrectionDistGraber &distGraber, //
                                                 std::vector<double> &L12s,                                //
                                                 const Geometry::IObject &shape) {
  const int64_t numVolumeElements = distGraber.m_numVolumeElements;
  // L12 is independent from the detector, therefore can be cached outside
  // - L12 is a upper off-diagonal matrix
  // NOTE: if the sample size/volume is too large, we might need to use openMP
  //       to parallelize the calculation

  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWS))
  for (int64_t indexTo = 0; indexTo < numVolumeElements; ++indexTo) {
    PARALLEL_START_INTERRUPT_REGION

    const auto posTo = distGraber.m_elementPositions[indexTo];
    Track track(posTo, V3D{0, 0, 1}); // take object creation out of the loop
    for (int64_t indexFrom = indexTo + 1; indexFrom < numVolumeElements; ++indexFrom) {
      // where in the final result to update, e.g.
      // x 1 2 3
      // x x 4 5
      // x x x 6
      // x x x x
      const int64_t idx = calcLinearIdxFromUpperTriangular(numVolumeElements, indexTo, indexFrom);

      const auto posFrom = distGraber.m_elementPositions[indexFrom];
      const V3D unitVector = getDirection(posFrom, posTo);

      // reset information in the Track and calculate distance
      track.reset(posFrom, unitVector);
      track.clearIntersectionResults();
      const auto rayLengthOne1 = getDistanceInsideObject(shape, track);

      track.reset(posTo, unitVector);
      track.clearIntersectionResults();
      const auto rayLengthOne2 = getDistanceInsideObject(shape, track);
      // getDistanceInsideObject returns the line segment inside the shape from the given ray (defined by track)
      // therefore, the distance between the two point (element) can be found by the difference of the two line
      // segments.
      L12s[idx] = checkzero(rayLengthOne1 - rayLengthOne2);
    }

    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
}

void MultipleScatteringCorrection::calculateL12s(const MultipleScatteringCorrectionDistGraber &distGraberContainer, //
                                                 const MultipleScatteringCorrectionDistGraber &distGraberSample,    //
                                                 std::vector<double> &L12sContainer,                                //
                                                 std::vector<double> &L12sSample,                                   //
                                                 const Geometry::IObject &shapeContainer,                           //
                                                 const Geometry::IObject &shapeSample) {
  const int64_t numVolumeElementsSample = distGraberSample.m_numVolumeElements;
  const int64_t numVolumeElementsContainer = distGraberContainer.m_numVolumeElements;
  const int64_t numVolumeElements = numVolumeElementsSample + numVolumeElementsContainer;
  // L12 is a upper off-diagonal matrix from the hybrid of container and sample.
  // e.g. container: a, b, c
  //      sample: alpha, beta, gamma
  //      hybrid: a, b, c, alpha, beta, gamma
  //      L12:
  //               a    b      c     alpha   beta    gamma
  //      -------------------------------------------------
  //      a     |  x   L12[0] L12[1] L12[2]  L12[3]  L12[4]
  //      b     |  x    x     L12[5] L12[6]  L12[7]  L12[8]
  //      c     |  x    x      x     L12[9]  L12[10] L12[11]
  //      alpha |  x    x      x      x      L12[12] L12[13]
  //      beta  |  x    x      x      x       x      L12[14]
  //      gamma |  x    x      x      x       x       x

  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWS))
  for (int64_t indexTo = 0; indexTo < numVolumeElements; ++indexTo) {
    PARALLEL_START_INTERRUPT_REGION
    // find the position of first scattering element (container or sample)
    const auto posTo = indexTo < numVolumeElementsContainer
                           ? distGraberContainer.m_elementPositions[indexTo]
                           : distGraberSample.m_elementPositions[indexTo - numVolumeElementsContainer];
    // NOTE:
    // We are using Track to ensure that the distance calculated are within the material
    Track track(posTo, V3D{0, 0, 1}); // reusable track, eco-friendly
    for (int64_t indexFrom = indexTo + 1; indexFrom < numVolumeElements; ++indexFrom) {
      const int64_t idx = calcLinearIdxFromUpperTriangular(numVolumeElements, indexTo, indexFrom);
      const auto posFrom = indexFrom < numVolumeElementsContainer
                               ? distGraberContainer.m_elementPositions[indexFrom]
                               : distGraberSample.m_elementPositions[indexFrom - numVolumeElementsContainer];
      const V3D unitVector = getDirection(posFrom, posTo);

      // combined
      track.clearIntersectionResults();
      track.reset(posFrom, unitVector);
      const auto rayLen1_container = getDistanceInsideObject(shapeContainer, track);
      track.clearIntersectionResults();
      track.reset(posFrom, unitVector);
      const auto rayLen1_sample = getDistanceInsideObject(shapeSample, track);
      track.clearIntersectionResults();
      track.reset(posTo, unitVector);
      const auto rayLen2_container = getDistanceInsideObject(shapeContainer, track);
      track.clearIntersectionResults();
      track.reset(posTo, unitVector);
      const auto rayLen2_sample = getDistanceInsideObject(shapeSample, track);
      //
      L12sContainer[idx] = checkzero(rayLen1_container - rayLen2_container);
      L12sSample[idx] = checkzero(rayLen1_sample - rayLen2_sample);
    }
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
}

/**
 * @brief Calculate distance between exiting element to the detector for single component case
 *
 * @param distGraber Pointer of distGraber that helps discretize the shape
 * @param detector Pointer of the detector object (pixel)
 * @param L2Ds Vector to container the distance from 2nd scattering element to detector within the material
 * @param shape Pointer of the shape object
 */
void MultipleScatteringCorrection::calculateL2Ds(const MultipleScatteringCorrectionDistGraber &distGraber,
                                                 const IDetector &detector, std::vector<double> &L2Ds,
                                                 const Geometry::IObject &shape) const {
  V3D detectorPos(detector.getPos());
  if (detector.nDets() > 1) {
    // We need to make sure this is right for grouped detectors - should use
    // average theta & phi
    detectorPos.spherical(detectorPos.norm(), detector.getTwoTheta(V3D(), V3D(0, 0, 1)) * RAD2DEG,
                          detector.getPhi() * RAD2DEG);
  }

  // calculate the distance between the detector and the shape (sample or container)
  Track TwoToDetector(distGraber.m_elementPositions[0], V3D{1, 0, 0}); // take object creation out of the loop
  for (size_t i = 0; i < distGraber.m_elementPositions.size(); ++i) {
    const auto &elementPos = distGraber.m_elementPositions[i];
    TwoToDetector.reset(elementPos, getDirection(elementPos, detectorPos));
    TwoToDetector.clearIntersectionResults();
    // find distance in sample
    L2Ds[i] = getDistanceInsideObject(shape, TwoToDetector);
  }
}

void MultipleScatteringCorrection::calculateL2Ds(const MultipleScatteringCorrectionDistGraber &distGraberContainer, //
                                                 const MultipleScatteringCorrectionDistGraber &distGraberSample,    //
                                                 const IDetector &detector,                                         //
                                                 std::vector<double> &container_L2Ds,                               //
                                                 std::vector<double> &sample_L2Ds,                                  //
                                                 const Geometry::IObject &shapeContainer,                           //
                                                 const Geometry::IObject &shapeSample) const {
  V3D detectorPos(detector.getPos());
  if (detector.nDets() > 1) {
    // We need to make sure this is right for grouped detectors - should use
    // average theta & phi
    detectorPos.spherical(detectorPos.norm(), detector.getTwoTheta(V3D(), V3D(0, 0, 1)) * RAD2DEG,
                          detector.getPhi() * RAD2DEG);
  }

  const int64_t numVolumeElementsSample = distGraberSample.m_numVolumeElements;
  const int64_t numVolumeElementsContainer = distGraberContainer.m_numVolumeElements;
  const int64_t numVolumeElements = numVolumeElementsSample + numVolumeElementsContainer;
  // Total elements = [container_elements] + [sample_elements]
  // Schematic for scattering element i (*)
  //   |                       \                                        /                       |
  //   |      container         \               sample                 /  container             |
  //   |                         \                                    /                         |
  //   | ---LS1_container[i] ---  \  LS1_sample[i] * L2D_sample[i]   / ---L2D_container[i]  --- |
  //   |                           \                                /                           |
  // L2D must be calculated within the loop of spectra, so we cannot use OpenMP here
  Track trackerL2D(V3D{0, 0, 1}, V3D{0, 0, 1}); // reusable tracker for calculating L2D
  for (int64_t idx = 0; idx < numVolumeElements; ++idx) {
    const auto pos = idx < numVolumeElementsContainer
                         ? distGraberContainer.m_elementPositions[idx]
                         : distGraberSample.m_elementPositions[idx - numVolumeElementsContainer];
    const auto vec = getDirection(pos, detectorPos);
    //
    trackerL2D.reset(pos, vec);
    trackerL2D.clearIntersectionResults();
    container_L2Ds[idx] = getDistanceInsideObject(shapeContainer, trackerL2D);
    trackerL2D.reset(pos, vec);
    trackerL2D.clearIntersectionResults();
    sample_L2Ds[idx] = getDistanceInsideObject(shapeSample, trackerL2D);
  }
}

void MultipleScatteringCorrection::pairWiseSum(double &A1, double &A2,                                   //
                                               const double linearCoefAbs,                               //
                                               const MultipleScatteringCorrectionDistGraber &distGraber, //
                                               const std::vector<double> &LS1s,                          //
                                               const std::vector<double> &L12s,                          //
                                               const std::vector<double> &L2Ds,                          //
                                               const int64_t startIndex, const int64_t endIndex) const {
  if (endIndex - startIndex > MAX_INTEGRATION_LENGTH) {
    int64_t middle = findMiddle(startIndex, endIndex);

    // recursive to process upper and lower part
    pairWiseSum(A1, A2, linearCoefAbs, distGraber, LS1s, L12s, L2Ds, startIndex, middle);
    pairWiseSum(A1, A2, linearCoefAbs, distGraber, LS1s, L12s, L2Ds, middle, endIndex);
  } else {
    // perform the integration
    const auto &elementVolumes = distGraber.m_elementVolumes;
    const auto nElements = distGraber.m_numVolumeElements;
    for (int64_t i = startIndex; i < endIndex; ++i) {
      // compute A1
      double exponent = (LS1s[i] + L2Ds[i]) * linearCoefAbs;
      A1 += exp(exponent) * elementVolumes[i];
      // compute A2
      double a2 = 0.0;
      for (int64_t j = 0; j < int64_t(nElements); ++j) {
        if (i == j) {
          // skip self (second order scattering must happen in a different element)
          continue;
        }
        // L12 is a pre-computed vector, therefore we can use the index directly
        size_t idx_l12 = i < j ? calcLinearIdxFromUpperTriangular(nElements, i, j)
                               : calcLinearIdxFromUpperTriangular(nElements, j, i);
        // compute a2 component
        const auto l12 = L12s[idx_l12];
        if (l12 > 0.0) {
          exponent = (LS1s[i] + L12s[idx_l12] + L2Ds[j]) * linearCoefAbs;
          a2 += exp(exponent) * elementVolumes[j] / (L12s[idx_l12] * L12s[idx_l12]);
        }
      }
      A2 += a2 * elementVolumes[i];
    }
  }
}

void MultipleScatteringCorrection::pairWiseSum(
    double &A1, double &A2,                                                          //
    const double linearCoefAbsContainer, const double linearCoefAbsSample,           //
    const int64_t numVolumeElementsContainer, const int64_t numVolumeElementsTotal,  //
    const double totScatterCoefContainer,                                            // rho * sigma_s * unit_scaling
    const double totScatterCoefSample,                                               // unit_scaling = 100
    const std::vector<double> &elementVolumes,                                       //
    const std::vector<double> &LS1sContainer, const std::vector<double> &LS1sSample, // source    -> 1_element
    const std::vector<double> &L12sContainer, const std::vector<double> &L12sSample, // 1_element -> 2_element
    const std::vector<double> &L2DsContainer, const std::vector<double> &L2DsSample, // 2_element -> detector
    const int64_t startIndex, const int64_t endIndex) const {
  if (endIndex - startIndex > MAX_INTEGRATION_LENGTH) {
    int64_t middle = findMiddle(startIndex, endIndex);
    // recursive to process upper and lower part
    pairWiseSum(A1, A2, linearCoefAbsContainer, linearCoefAbsSample, numVolumeElementsContainer, numVolumeElementsTotal,
                totScatterCoefContainer, totScatterCoefSample, elementVolumes, LS1sContainer, LS1sSample, L12sContainer,
                L12sSample, L2DsContainer, L2DsSample, startIndex, middle);
    pairWiseSum(A1, A2, linearCoefAbsContainer, linearCoefAbsSample, numVolumeElementsContainer, numVolumeElementsTotal,
                totScatterCoefContainer, totScatterCoefSample, elementVolumes, LS1sContainer, LS1sSample, L12sContainer,
                L12sSample, L2DsContainer, L2DsSample, middle, endIndex);
  } else {
    // perform the integration
    for (int64_t i = startIndex; i < endIndex; ++i) {
      const double factor_i = i > numVolumeElementsContainer ? totScatterCoefSample : totScatterCoefContainer;
      // compute A1
      double exponent = (LS1sContainer[i] + L2DsContainer[i]) * linearCoefAbsContainer +
                        (LS1sSample[i] + L2DsSample[i]) * linearCoefAbsSample;
      A1 += exp(exponent) * factor_i * elementVolumes[i];
      // compute A2
      double a2 = 0.0;
      for (int64_t j = 0; j < numVolumeElementsTotal; ++j) {
        if (i == j) {
          // skip self (second order scattering must happen in a different element)
          continue;
        }
        const double factor_j = j > numVolumeElementsContainer ? totScatterCoefSample : totScatterCoefContainer;
        // L12 is a pre-computed vector, therefore we can use the index directly
        size_t idx_l12 = i < j ? calcLinearIdxFromUpperTriangular(numVolumeElementsTotal, i, j)
                               : calcLinearIdxFromUpperTriangular(numVolumeElementsTotal, j, i);
        const double l12 = L12sContainer[idx_l12] + L12sSample[idx_l12];
        if (l12 > 0.0) {
          exponent = (LS1sContainer[i] + L12sContainer[idx_l12] + L2DsContainer[j]) * linearCoefAbsContainer + //
                     (LS1sSample[i] + L12sSample[idx_l12] + L2DsSample[j]) * linearCoefAbsSample;
          a2 += exp(exponent) * factor_j * elementVolumes[j] / (l12 * l12);
        }
      }
      A2 += a2 * factor_i * elementVolumes[i];
    }
  }
}

} // namespace Mantid::Algorithms
