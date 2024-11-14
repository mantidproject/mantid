// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CalculateCarpenterSampleCorrection.h"
#include "MantidAPI/HistoWorkspace.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/FloatingPointComparison.h"
#include "MantidKernel/Material.h"

#include <stdexcept>

namespace Mantid::Algorithms {
DECLARE_ALGORITHM(CalculateCarpenterSampleCorrection) // Register the class
                                                      // into the algorithm
                                                      // factory
using namespace DataObjects;
using namespace Kernel;
using namespace API;
using Mantid::DataObjects::EventWorkspace;
using Mantid::DataObjects::EventWorkspace_sptr;
using Mantid::HistogramData::HistogramY;
using Mantid::HistogramData::Points;
using std::vector;
using namespace Mantid::PhysicalConstants;
using namespace Geometry;

// Constants required internally only, so make them static. These are
// Chebyshev expansion coefficients copied directly from Carpenter 1969 Table 1
namespace { // anonymous
static const double CHEBYSHEV[] = {
    // l= 0       1         2          3         4          5       // (m,n)
    0.730284, -0.249987, 0.019448, -0.000006, 0.000249,  -0.000004, // (1,1)
    0.848859, -0.452690, 0.056557, -0.000009, 0.000000,  -0.000006, // (1,2)
    1.133129, -0.749962, 0.118245, -0.000018, -0.001345, -0.000012, // (1,3)
    1.641112, -1.241639, 0.226247, -0.000045, -0.004821, -0.000030, // (1,4)
    0.848859, -0.452690, 0.056557, -0.000009, 0.000000,  -0.000006, // (2,1)
    1.000006, -0.821100, 0.166645, -0.012096, 0.000008,  -0.000126, // (2,2)
    1.358113, -1.358076, 0.348199, -0.038817, 0.000022,  -0.000021, // (2,3)
    0.0,      0.0,       0.0,      0.0,       0.0,       0.0,       // (2,4)
    1.133129, -0.749962, 0.118245, -0.000018, -0.001345, -0.000012, // (3,1)
    1.358113, -1.358076, 0.348199, -0.038817, 0.000022,  -0.000021, // (3,2)
    0.0,      0.0,       0.0,      0.0,       0.0,       0.0,       // (3,3)
    0.0,      0.0,       0.0,      0.0,       0.0,       0.0,       // (3,4)
    1.641112, -1.241639, 0.226247, -0.000045, -0.004821, -0.000030, // (4,1)
    0.0,      0.0,       0.0,      0.0,       0.0,       0.0,       // (4,2)
    0.0,      0.0,       0.0,      0.0,       0.0,       0.0,       // (4,3)
    0.0,      0.0,       0.0,      0.0,       0.0,       0.0        // (4,4)
};

static const int Z_size = 36; // Caution, this must be updated if the
                              // algorithm is changed to use a different
                              // size Z array.
static const double Z_initial[] = {
    1.0, 0.8488263632, 1.0, 1.358122181, 2.0, 3.104279270, 0.8488263632, 0.0, 0.0, 0.0, 0.0, 0.0,
    1.0, 0.0,          0.0, 0.0,         0.0, 0.0,         1.358122181,  0.0, 0.0, 0.0, 0.0, 0.0,
    2.0, 0.0,          0.0, 0.0,         0.0, 0.0,         3.104279270,  0.0, 0.0, 0.0, 0.0, 0.0};

static const double LAMBDA_REF = 1.81; ///< Wavelength that the calculations are based on
// Badly named constants, no explanation of the origin of these
// values. They appear to be used when calculating the multiple
// scattering correction factor.
static const double COEFF4 = 1.1967;
static const double COEFF5 = -0.8667;
} // namespace

const std::string CalculateCarpenterSampleCorrection::name() const { return "CalculateCarpenterSampleCorrection"; }

int CalculateCarpenterSampleCorrection::version() const { return 1; }

const std::string CalculateCarpenterSampleCorrection::category() const {
  return "CorrectionFunctions\\AbsorptionCorrections";
}

/**
 * Initialize the properties to default values
 */
void CalculateCarpenterSampleCorrection::init() {
  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<InstrumentValidator>();

  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input, wsValidator),
      "The name of the input workspace.");
  declareProperty(
      std::make_unique<WorkspaceProperty<API::WorkspaceGroup>>("OutputWorkspaceBaseName", "", Direction::Output),
      "Basename of the output workspace group for corrections."
      "Absorption suffix = '_abs'. "
      "Multiple Scattering suffix = '_ms'. ");
  declareProperty("AttenuationXSection", 2.8,
                  "Coefficient 1, absorption cross "
                  "section / 1.81 if not set with "
                  "SetSampleMaterial");
  declareProperty("ScatteringXSection", 5.1,
                  "Coefficient 3, total scattering "
                  "cross section if not set with "
                  "SetSampleMaterial");
  declareProperty("SampleNumberDensity", 0.0721, "Coefficient 2, density if not set with SetSampleMaterial");
  declareProperty("CylinderSampleRadius", 0.3175, "Sample radius, in cm");
  declareProperty("Absorption", true, "If True then calculates the absorption correction.", Direction::Input);
  declareProperty("MultipleScattering", true, "If True then calculates the  multiple scattering correction.",
                  Direction::Input);
}

/**
 * Execute the algorithm
 */
void CalculateCarpenterSampleCorrection::exec() {
  // common information
  MatrixWorkspace_sptr inputWksp = getProperty("InputWorkspace");
  double radius = getProperty("CylinderSampleRadius");
  double coeff1 = getProperty("AttenuationXSection");
  double coeff2 = getProperty("SampleNumberDensity");
  double coeff3 = getProperty("ScatteringXSection");
  const bool absOn = getProperty("Absorption");
  const bool msOn = getProperty("MultipleScattering");
  const Material &sampleMaterial = inputWksp->sample().getMaterial();
  if (sampleMaterial.totalScatterXSection() != 0.0) {
    g_log.information() << "Using material \"" << sampleMaterial.name() << "\" from workspace\n";
    if (Kernel::equals(coeff1, 2.8))
      coeff1 = sampleMaterial.absorbXSection(LAMBDA_REF) / LAMBDA_REF;
    if (Kernel::equals(coeff2, 0.0721) && !isEmpty(sampleMaterial.numberDensity()))
      coeff2 = sampleMaterial.numberDensity();
    if (Kernel::equals(coeff3, 5.1))
      coeff3 = sampleMaterial.totalScatterXSection();
  } else // Save input in Sample with wrong atomic number and name
  {
    NeutronAtom neutron(0, 0, 0.0, 0.0, coeff3, 0.0, coeff3, coeff1);
    auto shape = std::shared_ptr<IObject>(
        inputWksp->sample().getShape().cloneWithMaterial(Material("SetInMultipleScattering", neutron, coeff2)));
    inputWksp->mutableSample().setShape(shape);
  }
  g_log.debug() << "radius=" << radius << " coeff1=" << coeff1 << " coeff2=" << coeff2 << " coeff3=" << coeff3 << "\n";

  // geometry stuff
  const auto NUM_HIST = static_cast<int64_t>(inputWksp->getNumberHistograms());
  Instrument_const_sptr instrument = inputWksp->getInstrument();
  if (instrument == nullptr)
    throw std::runtime_error("Failed to find instrument attached to InputWorkspace");
  IComponent_const_sptr source = instrument->getSource();
  IComponent_const_sptr sample = instrument->getSample();
  if (source == nullptr)
    throw std::runtime_error("Failed to find source in the instrument for InputWorkspace");
  if (sample == nullptr)
    throw std::runtime_error("Failed to find sample in the instrument for InputWorkspace");

  // Initialize progress reporting.
  Progress prog(this, 0.0, 1.0, NUM_HIST);

  // Create the new correction workspaces
  MatrixWorkspace_sptr absWksp = createOutputWorkspace(inputWksp, "Attenuation factor");
  MatrixWorkspace_sptr msWksp = createOutputWorkspace(inputWksp, "Multiple scattering factor");

  // now do the correction
  const auto &spectrumInfo = inputWksp->spectrumInfo();
  PARALLEL_FOR_IF(Kernel::threadSafe(*absWksp, *msWksp))
  for (int64_t index = 0; index < NUM_HIST; ++index) {
    PARALLEL_START_INTERRUPT_REGION
    if (!spectrumInfo.hasDetectors(index))
      throw std::runtime_error("Failed to find detector");
    if (spectrumInfo.isMasked(index))
      continue;
    const double tth_rad = spectrumInfo.twoTheta(index);

    // absorption
    if (absOn) {
      absWksp->setSharedX(index, inputWksp->sharedX(index));
      const auto lambdas = inputWksp->points(index);
      auto &y = absWksp->mutableY(index);
      calculate_abs_correction(tth_rad, radius, coeff1, coeff2, coeff3, lambdas, y);
    }

    // multiple scattering
    if (msOn) {
      msWksp->setSharedX(index, inputWksp->sharedX(index));
      const auto lambdas = inputWksp->points(index);
      auto &y = msWksp->mutableY(index);
      calculate_ms_correction(tth_rad, radius, coeff1, coeff2, coeff3, lambdas, y);
    }

    prog.report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  absWksp->setDistribution(true);
  absWksp->setYUnit("");
  absWksp->setYUnitLabel("Attenuation factor");

  msWksp->setDistribution(true);
  msWksp->setYUnit("");
  msWksp->setYUnitLabel("Multiple scattering factor");

  // Group and output workspaces we calculated
  const std::string group_prefix = getPropertyValue("OutputWorkspaceBaseName");
  auto outputGroup = std::make_shared<API::WorkspaceGroup>();
  if (absOn) {
    absWksp = setUncertainties(absWksp);
    std::string ws_name = group_prefix + std::string("_abs");
    AnalysisDataService::Instance().addOrReplace(ws_name, absWksp);
    outputGroup->addWorkspace(absWksp);
  } else {
    deleteWorkspace(absWksp);
  }

  if (msOn) {
    msWksp = setUncertainties(msWksp);
    std::string ws_name = group_prefix + std::string("_ms");
    AnalysisDataService::Instance().addOrReplace(ws_name, msWksp);
    outputGroup->addWorkspace(msWksp);
  } else {
    deleteWorkspace(msWksp);
  }

  setProperty("OutputWorkspaceBaseName", outputGroup);
}

namespace { // anonymous namespace
// Set up the Z table for the specified two theta angle (in degrees).
vector<double> createZ(const double angle_rad) {
  vector<double> Z(Z_initial, Z_initial + Z_size);

  const double theta_rad = angle_rad * .5;
  int l, J;
  double sum;

  for (int i = 1; i <= 4; i++) {
    for (int j = 1; j <= 4; j++) {
      int iplusj = i + j;
      if (iplusj <= 5) {
        l = 0;
        J = 1 + l + 6 * (i - 1) + 6 * 4 * (j - 1);
        sum = CHEBYSHEV[J - 1];

        for (l = 1; l <= 5; l++) {
          J = 1 + l + 6 * (i - 1) + 6 * 4 * (j - 1);
          sum = sum + CHEBYSHEV[J - 1] * cos(l * theta_rad);
        }
        J = 1 + i + 6 * j;
        Z[J - 1] = sum;
      }
    }
  }
  return Z;
}

/**
 * Evaluate the AttFac function for a given sigir and sigsr.
 */
double AttFac(const double sigir, const double sigsr, const vector<double> &Z) {
  double facti = 1.0;
  double att = 0.0;

  for (size_t i = 0; i <= 5; i++) {
    double facts = 1.0;
    for (size_t j = 0; j <= 5; j++) {
      if (i + j <= 5) {
        size_t J = 1 + i + 6 * j; // TODO J defined in terms of j?
        att = att + Z[J - 1] * facts * facti;
        facts = -facts * sigsr / static_cast<double>(j + 1);
      }
    }
    facti = -facti * sigir / static_cast<double>(i + 1);
  }
  return att;
}

double calculate_abs_factor(const double radius, const double Q2, const double sigsct, const vector<double> &Z,
                            const double wavelength) {

  const double sigabs = Q2 * wavelength;
  const double sigir = (sigabs + sigsct) * radius;
  /**
   * By setting the incident and scattered cross sections to be equal
   * we implicitly assume elastic scattering because in general these will
   * vary with neutron energy.
   **/
  const double sigsr = sigir;

  return AttFac(sigir, sigsr, Z);
}

double calculate_ms_factor(const double radius, const double Q2, const double sigsct, const vector<double> &Z,
                           const double wavelength) {

  const double sigabs = Q2 * wavelength;
  const double sigir = (sigabs + sigsct) * radius;
  /**
   * By setting the incident and scattered cross sections to be equal
   * we implicitly assume elastic scattering because in general these will
   * vary with neutron energy.
   **/
  const double sigsr = sigir;

  const double delta = COEFF4 * sigir + COEFF5 * sigir * sigir;
  const double deltp = (delta * sigsct) / (sigsct + sigabs);

  double temp = AttFac(sigir, sigsr, Z);
  return (deltp / temp);
}

} // namespace

/**
 *  This method will change the values in the y_val array to correct for
 *  multiple scattering absorption. Parameter total_path is in meters, and
 *  the sample radius is in cm.
 *
 *  @param angle_deg ::   The scattering angle (two theta) in degrees
 *  @param radius ::      The sample rod radius in cm
 *  @param coeff1 ::      The absorption cross section / 1.81
 *  @param coeff2 ::      The density
 *  @param coeff3 ::      The total scattering cross section
 *  @param wavelength ::          Array of wavelengths at bin boundaries
 *                     (or bin centers) for the spectrum, in Angstroms
 *  @param y_val ::       The spectrum values
 */
void CalculateCarpenterSampleCorrection::calculate_abs_correction(const double angle_deg, const double radius,
                                                                  const double coeff1, const double coeff2,
                                                                  const double coeff3, const Points &wavelength,
                                                                  HistogramY &y_val) {

  const size_t NUM_Y = y_val.size();
  bool is_histogram = false;
  if (wavelength.size() == NUM_Y + 1)
    is_histogram = true;
  else if (wavelength.size() == NUM_Y)
    is_histogram = false;
  else
    throw std::runtime_error("Data is neither historgram or density");

  // initialize Z array for this angle
  vector<double> Z = createZ(angle_deg);

  const double Q2 = coeff1 * coeff2;
  const double sigsct = coeff2 * coeff3;

  for (size_t j = 0; j < NUM_Y; j++) {
    double wl_val = wavelength[j];
    if (is_histogram) // average with next value
      wl_val = .5 * (wl_val + wavelength[j + 1]);

    y_val[j] = calculate_abs_factor(radius, Q2, sigsct, Z, wl_val);
  }
}

void CalculateCarpenterSampleCorrection::calculate_ms_correction(const double angle_deg, const double radius,
                                                                 const double coeff1, const double coeff2,
                                                                 const double coeff3, const Points &wavelength,
                                                                 HistogramY &y_val) {

  const size_t NUM_Y = y_val.size();
  bool is_histogram = false;
  if (wavelength.size() == NUM_Y + 1)
    is_histogram = true;
  else if (wavelength.size() == NUM_Y)
    is_histogram = false;
  else
    throw std::runtime_error("Data is neither historgram or density");

  // initialize Z array for this angle
  vector<double> Z = createZ(angle_deg);

  const double Q2 = coeff1 * coeff2;
  const double sigsct = coeff2 * coeff3;

  for (size_t j = 0; j < NUM_Y; j++) {
    double wl_val = wavelength[j];
    if (is_histogram) // average with next value
      wl_val = .5 * (wl_val + wavelength[j + 1]);

    y_val[j] = calculate_ms_factor(radius, Q2, sigsct, Z, wl_val);
  }
}

MatrixWorkspace_sptr CalculateCarpenterSampleCorrection::createOutputWorkspace(const MatrixWorkspace_sptr &inputWksp,
                                                                               const std::string &ylabel) const {
  MatrixWorkspace_sptr outputWS = create<HistoWorkspace>(*inputWksp);
  // The algorithm computes the signal values at bin centres so they should
  // be treated as a distribution
  outputWS->setDistribution(true);
  outputWS->setYUnit("");
  outputWS->setYUnitLabel(ylabel);
  return outputWS;
}

MatrixWorkspace_sptr CalculateCarpenterSampleCorrection::setUncertainties(const MatrixWorkspace_sptr &workspace) {
  auto alg = this->createChildAlgorithm("SetUncertainties");
  alg->initialize();
  alg->setProperty("InputWorkspace", workspace);
  alg->execute();
  return alg->getProperty("OutputWorkspace");
}

void CalculateCarpenterSampleCorrection::deleteWorkspace(const MatrixWorkspace_sptr &workspace) {
  auto alg = this->createChildAlgorithm("DeleteWorkspace");
  alg->initialize();
  alg->setChild(true);
  alg->setLogging(false);
  alg->setProperty("Workspace", workspace);
  alg->execute();
}

} // namespace Mantid::Algorithms
