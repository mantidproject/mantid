// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/CalculatePlaczek.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SampleValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/Atom.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Unit.h"

#include <utility>

namespace Mantid::Algorithms {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;
using PhysicalConstants::BoltzmannConstant;
using PhysicalConstants::E_mev_toNeutronWavenumberSq; // in [meV*Angstrom^2]

namespace { // anonymous namespace

// calculate summation term w/ neutron mass over molecular mass ratio
// NOTE:
//  - this is directly borrowed from the original CalculatePlaczekSelfScattering
std::pair<double, double> calculateSummationTerm(const Kernel::Material &material) {
  std::pair<double, double> s;
  // add together the weighted sum
  const auto &formula = material.chemicalFormula();
  auto sumLambda_first = [](double sum, const auto &formula_unit) {
    return sum + formula_unit.multiplicity * formula_unit.atom->neutron.tot_scatt_xs / formula_unit.atom->mass;
  };
  auto sumLambda_second = [](double sum, const auto &formula_unit) {
    const double mass_sq = formula_unit.atom->mass * formula_unit.atom->mass;
    return sum + formula_unit.multiplicity * formula_unit.atom->neutron.tot_scatt_xs / mass_sq;
  };
  const double unnormalizedTermFirst = std::accumulate(formula.begin(), formula.end(), 0.0, sumLambda_first);
  const double unnormalizedTermSecond = std::accumulate(formula.begin(), formula.end(), 0.0, sumLambda_second);

  // neutron mass converted to atomic mass comes out of the sum
  constexpr double neutronMass = PhysicalConstants::NeutronMass / PhysicalConstants::AtomicMassUnit;
  constexpr double neutronMassSq = neutronMass * neutronMass;
  // normalizing by totalStoich (number of atoms) comes out of the sum
  const double totalStoich = material.totalAtoms();
  // converting scattering cross section to scattering length square comes out of the sum
  s.first = neutronMass * unnormalizedTermFirst / (4. * M_PI * totalStoich);
  s.second = neutronMassSq * unnormalizedTermSecond / (4. * M_PI * totalStoich);

  return s;
}

} // anonymous namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculatePlaczek)

//----------------------------------------------------------------------------------------------
// NOTE: the template seems to prefer having the implemenation in the cpp file

/// Algorithms name for identification. @see Algorithm::name
const std::string CalculatePlaczek::name() const { return "CalculatePlaczek"; }

/// Algorithm's version for identification. @see Algorithm::version
int CalculatePlaczek::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CalculatePlaczek::category() const { return "CorrectionFunctions"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CalculatePlaczek::summary() const {
  return "Calculate 1st or 2nd order Placzek correction factors using given workspace and incident spectrums.";
}

/// Algorithm's see also for use in the GUI and help. @see Algorithm::seeAlso
const std::vector<std::string> CalculatePlaczek::seeAlso() const {
  return {"CalculatePlaczekSelfScattering", "He3TubeEfficiency"};
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CalculatePlaczek::init() {
  // Mandatory properties
  // 1. Input workspace should have
  //    - a valid instrument
  //    - a sample with chemical formula
  auto wsValidator = std::make_shared<Mantid::Kernel::CompositeValidator>();
  wsValidator->add<Mantid::API::InstrumentValidator>();
  wsValidator->add<Mantid::API::SampleValidator, unsigned int>(Mantid::API::SampleValidator::Material);
  declareProperty(std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>("InputWorkspace", "",
                                                                                 Kernel::Direction::Input, wsValidator),
                  "Raw diffraction data workspace for associated correction to be "
                  "calculated for. Workspace must have instrument and sample data.");
  // 2. Incident spectra should have a unit of wavelength
  auto inspValidator = std::make_shared<Mantid::Kernel::CompositeValidator>();
  inspValidator->add<Mantid::API::WorkspaceUnitValidator>("Wavelength");
  declareProperty(std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
                      "IncidentSpectra", "", Kernel::Direction::Input, inspValidator),
                  "Workspace of fitted incident spectrum with its derivatives (1st &| 2nd).");

  // Optional properties
  declareProperty(std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
                      "EfficiencySpectra", "", Kernel::Direction::Input, API::PropertyMode::Optional),
                  "Workspace of efficiency spectrum with its derivatives (1st &| 2nd)."
                  "Default (not specified) will use LambdaD to calculate the efficiency spectrum.");
  auto lambdadValidator = std::make_shared<Kernel::BoundedValidator<double>>();
  lambdadValidator->setExclusive(true);
  lambdadValidator->setLower(0.0);
  declareProperty(
      "LambdaD", 1.44, lambdadValidator,
      "Reference wavelength in Angstrom, related to detector efficient coefficient alpha."
      "The coefficient used to generate a generic detector efficiency curve,"
      "eps = 1 - exp(1 - alpha*lambda), where alpha is 1/LambdaD."
      "Default is set to 1.44 for ISIS 3He detectors and 1/0.83 for ISIS:LAD circa 1990 scintillator detectors.");
  declareProperty("CrystalDensity", EMPTY_DBL(), "The crystalographic density of the sample material.");
  auto orderValidator = std::make_shared<Kernel::BoundedValidator<int>>(1, 2);
  declareProperty("Order", 1, orderValidator, "Placzek correction order (1 or 2), default to 1 (self scattering).");
  declareProperty("SampleTemperature", EMPTY_DBL(),
                  "Sample temperature in Kelvin."
                  "The input property is prioritized over the temperature recorded in the sample log."
                  "The temperature is necessary for computing second order correction.");
  declareProperty("ScaleByPackingFraction", true, "Scale the correction value by packing fraction.");

  // Output property
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>("OutputWorkspace", "", Kernel::Direction::Output),
      "Workspace with the Placzek scattering correction factors.");
}

//----------------------------------------------------------------------------------------------
/**
 * @brief validate inputs
 *
 * @return std::map<std::string, std::string>
 */
std::map<std::string, std::string> CalculatePlaczek::validateInputs() {
  std::map<std::string, std::string> issues;
  const API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  const API::SpectrumInfo specInfo = inWS->spectrumInfo();
  const int order = getProperty("Order");

  // Case0:missing detector info
  if (specInfo.size() == 0) {
    issues["InputWorkspace"] = "Input workspace does not have detector information";
  }

  // Case1: cannot locate sample temperature
  if (isDefault("SampleTemperature") && (order == 2)) {
    const auto run = inWS->run();
    const Mantid::Kernel::Property *sampleTempLog = NULL;
    // ORNL logs use SampleTemp
    if (run.hasProperty("SampleTemp")) {
      sampleTempLog = run.getLogData("SampleTemp");
    }
    // ISIS logs use sample_temp
    if (run.hasProperty("sample_temp")) {
      sampleTempLog = run.getLogData("sample_temp");
    }
    if (!sampleTempLog) {
      issues["SampleTemperature"] = "Cannot locate sample temperature in the run.";
    }
  }

  // Case2: check number of spectra in flux workspace match required order
  const API::MatrixWorkspace_sptr incidentWS = getProperty("IncidentSpectra");
  const int64_t numHist = incidentWS->spectrumInfo().size();
  if (order == 2) {
    // we need three spectra here
    if (numHist < 3) {
      issues["IncidentSpectra"] = "Need three spectra here for second order calculation.";
    } else {
      // if there are the correct number of spectra make sure all are not empty
      if (incidentWS->readY(0).empty()) {
        issues["IncidentSpectra"] = "Flux is empty";
      }
      if (incidentWS->readY(1).empty()) {
        issues["IncidentSpectra"] = "First order derivate of the incident spectrum is empty";
      }
      if (incidentWS->readY(2).empty()) {
        issues["IncidentSpectra"] = "Second order derivate of the incident spectrum is empty";
      }
    }
  } else {
    // we are at first order here
    if (numHist < 2) {
      issues["IncidentSpectra"] = "Need two spectra here for first order calculation.";
    } else {
      // if there are the correct number of spectra make sure all are not empty
      if (incidentWS->readY(0).empty()) {
        issues["IncidentSpectra"] = "Flux is empty";
      }
      if (incidentWS->readY(1).empty()) {
        issues["IncidentSpectra"] = "First order derivate of the incident spectrum is empty";
      }
    }
  }

  // Case3: check number of spectra in efficiency workspace match required order IF provided
  const API::MatrixWorkspace_sptr efficiencyWS = getProperty("EfficiencySpectra");
  if (efficiencyWS) {
    const int64_t numHistEff = efficiencyWS->spectrumInfo().size();
    if (order == 2) {
      // we need three spectra here
      if (numHistEff < 3) {
        issues["EfficiencySpectra"] = "Need three spectra here for second order calculation.";
      } else {
        // if there are the correct number of spectra make sure all are not empty
        if (efficiencyWS->readY(0).empty()) {
          issues["EfficiencySpectra"] = "Detector efficiency is empty";
        }
        if (efficiencyWS->readY(1).empty()) {
          issues["EfficiencySpectra"] = "First order derivate of the efficiency spectrum is empty";
        }
        if (efficiencyWS->readY(2).empty()) {
          issues["EfficiencySpectra"] = "Second order derivate of the efficiency spectrum is empty";
        }
      }
    } else {
      // we are at first order here
      if (numHistEff < 2) {
        issues["EfficiencySpectra"] = "Need two spectra here for first order calculation.";
      } else {
        // if there are the correct number of spectra make sure all are not empty
        if (efficiencyWS->readY(0).empty()) {
          issues["EfficiencySpectra"] = "Detector efficiency is empty";
        }
        if (efficiencyWS->readY(1).empty()) {
          issues["EfficiencySpectra"] = "First order derivate of the efficiency spectrum is empty";
        }
      }
    }
  }

  // NOTE: order range check is enforced with validator
  return issues;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalculatePlaczek::exec() {
  // prep input
  const API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  const API::MatrixWorkspace_sptr incidentWS = getProperty("IncidentSpectra");
  const int order = getProperty("Order");
  const bool scaleByPackingFraction = getProperty("ScaleByPackingFraction");

  // prep output
  // build the output workspace
  // - use instrument information from InputWorkspace
  // - use the bin Edges from the incident flux
  API::MatrixWorkspace_sptr outputWS =
      DataObjects::create<API::HistoWorkspace>(*inWS, incidentWS->getSpectrum(0).binEdges());
  outputWS->getAxis(0)->unit() = incidentWS->getAxis(0)->unit();
  outputWS->setDistribution(true);
  outputWS->setYUnit("");
  outputWS->setYUnitLabel("Counts");

  // ---------------------------------------------------------------------------
  // calculate the Placzek correction (self scattering + optional 2nd order)
  // ---------------------------------------------------------------------------
  /* Placzek
     Original Placzek inelastic correction Ref (for constant wavelength, reactor
     source): Placzek, Phys. Rev v86, (1952), pp. 377-388 First Placzek
     correction for time-of-flight, pulsed source (also shows reactor eqs.):
     Powles, Mol. Phys., v6 (1973), pp.1325-1350
     Nomenclature and calculation for this program follows Ref:
     Howe, McGreevy, and Howells, J. Phys.: Condens. Matter v1, (1989), pp.
     3433-3451 NOTE: Powles's Equation for inelastic self-scattering is equal to
     Howe's Equation for P(theta) by adding the elastic self-scattering
  */
  const auto xLambda = incidentWS->getSpectrum(0).points();
  // pre-compute the coefficients
  // - calculate summation term w/ neutron mass over molecular mass ratio
  const std::pair<double, double> summationTerms = calculateSummationTerm(inWS->sample().getMaterial());
  const double packingFraction = getPackingFraction(inWS);
  // NOTE:
  // - when order==1, we don't care what's inside sampleTemperature.
  // - when order==2, the value here will be a valid one in K.
  const double sampleTemperature = getSampleTemperature();

  // NOTE:
  // The following coefficients are defined in the appendix 1 of
  // Ref: Howe, McGreevy, and Howells, J. Phys.: Condens. Matter v1, (1989), pp.
  //      doi: 10.1088/0953-8984/1/22/005
  // The associated analytical forms are given on the second page of
  // Ref: Howells, W.S., Nuclear Instruments and Methods in Physics Research 223, no. 1 (June 1984): 141–46.
  //      doi: 10.1016/0167-5087(84)90256-4
  // - 1st order related coefficients
  const std::vector<double> phi1 = getFluxCoefficient1();
  const std::vector<double> eps1 = getEfficiencyCoefficient1();
  // - 2nd order related coefficients
  const std::vector<double> phi2 = (order == 2) ? getFluxCoefficient2() : std::vector<double>();
  const std::vector<double> eps2 = (order == 2) ? getEfficiencyCoefficient2() : std::vector<double>();

  // loop over all spectra
  const API::SpectrumInfo specInfo = inWS->spectrumInfo();
  const int64_t numHist = specInfo.size();
  PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS))
  for (int64_t specIndex = 0; specIndex < numHist; specIndex++) {
    PARALLEL_START_INTERRUPT_REGION
    auto &y = outputWS->mutableY(specIndex); // x-axis is directly copied from incident flux
    // only perform calculation for components that
    // - is monitor
    // - at (0,0,0)
    if (!specInfo.isMonitor(specIndex) && !(specInfo.l2(specIndex) == 0.0)) {
      Kernel::Units::Wavelength wavelength;
      Kernel::Units::TOF tof;
      Kernel::UnitParametersMap pmap{};
      double l1 = specInfo.l1();
      specInfo.getDetectorValues(wavelength, tof, Kernel::DeltaEMode::Elastic, false, specIndex, pmap);
      double l2 = 0., twoTheta = 0.;
      if (pmap.find(Kernel::UnitParams::l2) != pmap.end()) {
        l2 = pmap[Kernel::UnitParams::l2];
      }
      if (pmap.find(Kernel::UnitParams::twoTheta) != pmap.end()) {
        twoTheta = pmap[Kernel::UnitParams::twoTheta];
      }
      // first order (self scattering) is mandatory, second order is optional
      // - pre-compute constants that can be cached outside loop
      const double sinThetaBy2 = sin(twoTheta / 2.0);
      const double f = l1 / (l1 + l2);
      wavelength.initialize(specInfo.l1(), 0, pmap);
      const double kBT = BoltzmannConstant * sampleTemperature; // BoltzmannConstant in meV / K, T in K -> kBT in meV
      // - convenience variables
      const double sinHalfAngleSq = sinThetaBy2 * sinThetaBy2;
      // - loop over all lambda
      for (size_t xIndex = 0; xIndex < xLambda.size(); xIndex++) {
        // -- calculate first order correction
        const double term1 = (f - 1.0) * phi1[xIndex];
        const double term2 = f * (1.0 - eps1[xIndex]);
        double inelasticPlaczekCorrection = 2.0 * (term1 + term2 - 3) * sinHalfAngleSq * summationTerms.first;
        // -- calculate second order correction
        if (order == 2) {
          const double k = 2 * M_PI / xLambda[xIndex];                 // wave vector in 1/angstrom
          const double energy = E_mev_toNeutronWavenumberSq * (k * k); // in meV
          const double kBToverE = kBT / energy;                        // unitless
          // NOTE: see the equation A1.15 in Howe et al. The analysis of liquid structure, 1989
          const double bracket_1 = (8 * f - 9) * (f - 1) * phi1[xIndex]            //
                                   - 3 * f * (2 * f - 3) * eps1[xIndex]            //
                                   + 2 * f * (1 - f) * phi1[xIndex] * eps1[xIndex] //
                                   + (1 - f) * (1 - f) * phi2[xIndex]              //
                                   + f * f * eps2[xIndex]                          //
                                   + 3 * (4 * f - 5) * (f - 1);
          const double P2_part1 = summationTerms.first * (kBToverE / 2.0 + kBToverE * sinHalfAngleSq * bracket_1);
          const double bracket_2 = (4 * f - 7) * (f - 1) * phi1[xIndex]            //
                                   + f * (7 - 2 * f) * eps1[xIndex]                //
                                   + 2 * f * (1 - f) * phi1[xIndex] * eps1[xIndex] //
                                   + (1 - f) * (1 - f) * phi2[xIndex]              //
                                   + f * f * eps2[xIndex]                          //
                                   + (2 * f * f - 7 * f + 8);
          const double P2_part2 = 2 * sinHalfAngleSq * summationTerms.second * (1 + sinHalfAngleSq * bracket_2);
          // added to the factor
          inelasticPlaczekCorrection += P2_part1 + P2_part2;
        }
        // -- consolidate
        y[xIndex] =
            scaleByPackingFraction ? (1 + inelasticPlaczekCorrection) * packingFraction : inelasticPlaczekCorrection;
      }
    } else {
      for (size_t xIndex = 0; xIndex < xLambda.size(); xIndex++) {
        y[xIndex] = 0;
      }
    }
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  // set output
  setProperty("OutputWorkspace", outputWS);
}

//----------------------------------------------------------------------------------------------
/**
 * @brief compute the packing fraction with given crystal density
 *
 * @param ws
 * @return double
 */
double CalculatePlaczek::getPackingFraction(const API::MatrixWorkspace_const_sptr &ws) {
  // get a handle to the material
  const auto &material = ws->sample().getMaterial();

  // default value is packing fraction
  double packingFraction = material.packingFraction();

  // see if the user thinks the material wasn't setup right
  const double crystalDensity = getProperty("CrystalDensity");
  if (crystalDensity > 0.) {
    // assume that the number density set in the Material is the effective number density
    packingFraction = material.numberDensity() / crystalDensity;
  }

  return packingFraction;
}

/**
 * @brief query the sample temperature from input property or sample log
 *
 * @return double
 */
double CalculatePlaczek::getSampleTemperature() {
  double sampleTemperature = getProperty("SampleTemperature");
  const int order = getProperty("Order");

  // get the sample temperature from sample log if not provided by the user
  // NOTE:
  // we only need to go the extra mile when we really need a valid sample temperature,
  // i.e. when calculating the second order correction
  if (isDefault("SampleTemperature") && (order == 2)) {
    // get the sample temperature from sample log
    const API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
    const auto run = inWS->run();
    // SampleTemp only valid for ORNL runs
    if (run.hasProperty("SampleTemp")) {
      sampleTemperature = run.getPropertyAsSingleValue("SampleTemp");
      const std::string sampleTempUnit = run.getProperty("SampleTemp")->units();
      if (sampleTempUnit == "C") {
        sampleTemperature = sampleTemperature + 273.15; // convert to K
      }
      // sample_temp only valid for ISIS runs
    } else if (run.hasProperty("sample_temp")) {
      sampleTemperature = run.getPropertyAsSingleValue("sample_temp");
      const std::string sampleTempUnit = run.getProperty("sample_temp")->units();
      if (sampleTempUnit == "C") {
        sampleTemperature = sampleTemperature + 273.15; // convert to K
      }
    } else {
      // the validator should already catch this early on
      throw std::runtime_error("Sample temperature is not found in the log.");
    }
  }
  return sampleTemperature;
}

/**
 * @brief Compute the flux coefficient for the first order, i.e. Phi1 in the formula
 *
 * @return std::vector<double>
 */
std::vector<double> CalculatePlaczek::getFluxCoefficient1() {
  g_log.information("Compute the flux coefficient phi1.");
  std::vector<double> phi1;

  const API::MatrixWorkspace_sptr incidentWS = getProperty("IncidentSpectra");
  const auto xLambda = incidentWS->getSpectrum(0).points();
  const auto &incident = incidentWS->readY(0);
  const auto &incidentPrime = incidentWS->readY(1);
  // phi1 = lambda * phi'(lambda)/phi(lambda)
  for (size_t i = 0; i < xLambda.size(); i++) {
    phi1.emplace_back(xLambda[i] * incidentPrime[i] / incident[i]);
  }
  return phi1;
}

/**
 * @brief Compute the flux coefficient for the second order, i.e. Phi2 in the formula
 *
 * @return std::vector<double>
 */
std::vector<double> CalculatePlaczek::getFluxCoefficient2() {
  g_log.information("Compute the flux coefficient phi2.");
  std::vector<double> phi2;

  const API::MatrixWorkspace_sptr incidentWS = getProperty("IncidentSpectra");
  const auto xLambda = incidentWS->getSpectrum(0).points();
  const auto &incident = incidentWS->readY(0);
  const auto &incidentPrime2 = incidentWS->readY(2);
  // phi2 = lambda^2 * phi''(lambda)/phi(lambda)
  for (size_t i = 0; i < xLambda.size(); i++) {
    phi2.emplace_back(xLambda[i] * xLambda[i] * incidentPrime2[i] / incident[i]);
  }
  return phi2;
}

/**
 * @brief Compute the detector efficiency coefficient based on either given efficiency
 *        workspace or a vector derived from an assume efficiency profile
 *
 * @return std::vector<double>
 */
std::vector<double> CalculatePlaczek::getEfficiencyCoefficient1() {
  g_log.information("Compute detector efficiency coefficient 1");
  std::vector<double> eps1;

  // NOTE: we need the xlambda here to
  // - compute the coefficient based on an assumed efficiency curve
  const API::MatrixWorkspace_sptr incidentWS = getProperty("IncidentSpectra");
  const auto xLambda = incidentWS->getSpectrum(0).points();
  const API::MatrixWorkspace_sptr efficiencyWS = getProperty("EfficiencySpectra");
  if (efficiencyWS) {
    // Use the formula
    // eps1 = k * eps'/eps, k = 2pi/lambda
    std::vector<double> eps = efficiencyWS->readY(0);
    std::vector<double> epsPrime = efficiencyWS->readY(1);
    for (size_t i = 0; i < xLambda.size(); i++) {
      double lambda = xLambda[i];
      double k = 2.0 * M_PI / lambda;
      double eps_i = (eps[i] + eps[i + 1]) / 2.0;
      double epsPrime_i = (epsPrime[i] + epsPrime[i + 1]) / 2.0;
      eps1.emplace_back(k * epsPrime_i / eps_i);
    }
  } else {
    // This is based on an assume efficiency curve from
    const double LambdaD = getProperty("LambdaD");
    for (auto x : xLambda) {
      x /= -LambdaD;
      eps1.emplace_back(x * exp(x) / (1.0 - exp(x)));
    }
  }
  return eps1;
}

/**
 * @brief compute the second order detector efficiency coefficient vector
 *
 * @return std::vector<double>
 */
std::vector<double> CalculatePlaczek::getEfficiencyCoefficient2() {
  g_log.information("Compute detector efficiency coefficient 2");
  std::vector<double> eps2;

  // NOTE: we need the xlambda here to
  // - compute the coefficient based on an assumed efficiency curve
  const API::MatrixWorkspace_sptr incidentWS = getProperty("IncidentSpectra");
  const auto xLambda = incidentWS->getSpectrum(0).points();

  const API::MatrixWorkspace_sptr efficiencyWS = getProperty("EfficiencySpectra");
  if (efficiencyWS) {
    // Use the formula
    // eps1 = k^2 * eps''/eps, k = 2pi/lambda
    std::vector<double> eps = efficiencyWS->readY(0);
    std::vector<double> epsPrime2 = efficiencyWS->readY(2);
    for (size_t i = 0; i < xLambda.size(); i++) {
      double lambda = xLambda[i];
      double k = 2.0 * M_PI / lambda;
      double eps_i = (eps[i] + eps[i + 1]) / 2.0;
      double epsPrime2_i = (epsPrime2[i] + epsPrime2[i + 1]) / 2.0;
      eps2.emplace_back(k * k * epsPrime2_i / eps_i);
    }
  } else {
    // using the analytical formula from the second page of
    // Ref: Howells, W.S., Nuclear Instruments and Methods in Physics Research 223, no. 1 (June 1984): 141–46.
    //      doi: 10.1016/0167-5087(84)90256-4
    // DEV NOTE:
    // The detector efficiency coefficient is denoted with F1 and _2F in the paper instead of the eps1 and eps2
    // used in the code.
    const double LambdaD = getProperty("LambdaD");
    for (auto x : xLambda) {
      x /= -LambdaD;
      double eps1 = x * exp(x) / (1.0 - exp(x));
      eps2.emplace_back((-x - 2.0) * eps1);
    }
  }
  return eps2;
}

} // namespace Mantid::Algorithms
