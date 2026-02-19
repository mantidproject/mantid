// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/AnvredCorrection.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Fast_Exponential.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Unit.h"

/*  Following A.J.Schultz's anvred, the weight factors should be:
 *
 *    sin^2(theta) / (lamda^4 * spec * eff * trans)
 *
 *  where theta = scattering_angle/2
 *        lamda = wavelength (in angstroms?)
 *        spec  = incident spectrum correction
 *        eff   = pixel efficiency
 *        trans = absorption correction
 *
 *  The quantity:
 *
 *    sin^2(theta) / eff
 *
 *  depends only on the pixel and can be pre-calculated
 *  for each pixel.  It could be saved in array pix_weight[].
 *  For now, pix_weight[] is calculated by the method:
 *  BuildPixWeights() and just holds the sin^2(theta) values.
 *
 *  The wavelength dependent portion of the correction is saved in
 *  the array lamda_weight[].
 *  The time-of-flight is converted to wave length by multiplying
 *  by tof_to_lamda[id], then (int)STEPS_PER_ANGSTROM * lamda
 *  gives an index into the table lamda_weight[].
 *
 *  The lamda_weight[] array contains values like:
 *
 *      1/(lamda^power * spec(lamda))
 *
 *  which are pre-calculated for each lamda.  These values are
 *  saved in the array lamda_weight[].  The optimal value to use
 *  for the power should be determined when a good incident spectrum
 *  has been determined.  Currently, power=3 when used with an
 *  incident spectrum and power=2.4 when used without an incident
 *  spectrum.
 *
 *  The pixel efficiency and incident spectrum correction are NOT CURRENTLY
 *USED.
 *  The absorption correction, trans, depends on both lamda and the pixel,
 *  Which is a fairly expensive calulation when done for each event.
 */

namespace Mantid::Crystal {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(AnvredCorrection)

using namespace Kernel;
using namespace Geometry;
using namespace API;
using namespace DataObjects;
using namespace Mantid::PhysicalConstants;

AnvredCorrection::AnvredCorrection()
    : API::Algorithm(), m_smu(0.), m_amu(0.), m_radius(0.), m_power_th(0.), m_lamda_weight(),
      m_onlySphericalAbsorption(false), m_returnTransmissionOnly(false), m_useScaleFactors(false) {}

void AnvredCorrection::init() {

  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = std::make_shared<InstrumentValidator>();

  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input, wsValidator),
      "The X values for the input workspace must be in units of wavelength or TOF");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "Output workspace name");

  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("LinearScatteringCoef", EMPTY_DBL(), mustBePositive,
                  "Linear scattering coefficient in 1/cm. "
                  "If not provided this will be calculated from the "
                  "material cross-section if present (set with SetSampleMaterial)");
  declareProperty("LinearAbsorptionCoef", EMPTY_DBL(), mustBePositive,
                  "Linear absorption coefficient at 1.8 Angstroms in 1/cm. "
                  "If not provided this will be calculated from the "
                  "material cross-section if present (set with SetSampleMaterial)");
  declareProperty("Radius", EMPTY_DBL(), mustBePositive,
                  "Radius of the sample in centimeters. f not provided the "
                  "radius will be taken from the sample shape if it is a sphere "
                  "(set with SetSample).");
  declareProperty("PreserveEvents", true,
                  "Keep the output workspace as an EventWorkspace, if the "
                  "input has events (default).\n"
                  "If false, then the workspace gets converted to a "
                  "Workspace2D histogram.");
  declareProperty("OnlySphericalAbsorption", false,
                  "All corrections done if false (default).\n"
                  "If true, only the spherical absorption correction.");
  declareProperty("ReturnTransmissionOnly", false,
                  "Corrections applied to data if false (default).\n"
                  "If true, only return the transmission coefficient.");
  declareProperty("PowerLambda", 4.0, "Power of lamda ");
  declareProperty("DetectorBankScaleFactors", false,
                  "No scale factors if false (default).\n"
                  "If true, use scale factors from instrument parameter map.");

  defineProperties();
}

std::map<std::string, std::string> AnvredCorrection::validateInputs() {
  std::map<std::string, std::string> result;

  const double radius = getProperty("Radius");
  if (radius == EMPTY_DBL()) {
    // check that if radius isn't supplied that the radius can be accessed from the sample
    Mantid::API::MatrixWorkspace_sptr inputWorkspace = this->getProperty("InputWorkspace");
    if (!inputWorkspace) {
      result["InputWorkspace"] = "The InputWorkspace must be a MatrixWorkspace.";
      return result;
    }

    const auto &sampleShape = inputWorkspace->sample().getShape();
    if (!sampleShape.hasValidShape() ||
        sampleShape.shapeInfo().shape() != Geometry::detail::ShapeInfo::GeometryShape::SPHERE) {
      result["Radius"] = "Please supply a radius or provide a workspace with a spherical sample set.";
    }
  }
  return result;
}

void AnvredCorrection::exec() {
  // Retrieve the input workspace
  m_inputWS = getProperty("InputWorkspace");
  m_onlySphericalAbsorption = getProperty("OnlySphericalAbsorption");
  m_returnTransmissionOnly = getProperty("ReturnTransmissionOnly");
  m_useScaleFactors = getProperty("DetectorBankScaleFactors");
  if (!m_onlySphericalAbsorption) {
    const API::Run &run = m_inputWS->run();
    if (run.hasProperty("LorentzCorrection")) {
      auto lorentzDone = run.getPropertyValueAsType<bool>("LorentzCorrection");
      if (lorentzDone) {
        m_onlySphericalAbsorption = true;
        g_log.warning() << "Lorentz Correction was already done for this "
                           "workspace.  OnlySphericalAbsorption was changed to "
                           "true.\n";
      }
    }
  }

  const std::string &unitStr = m_inputWS->getAxis(0)->unit()->unitID();

  // Get the input parameters
  retrieveBaseProperties();

  if (!m_onlySphericalAbsorption && !m_returnTransmissionOnly)
    BuildLamdaWeights();

  eventW = std::dynamic_pointer_cast<EventWorkspace>(m_inputWS);
  if (eventW)
    eventW->sortAll(TOF_SORT, nullptr);
  if ((getProperty("PreserveEvents")) && (eventW != nullptr) && !m_returnTransmissionOnly) {
    // Input workspace is an event workspace. Use the other exec method
    this->execEvent();
    this->cleanup();
    return;
  }

  MatrixWorkspace_sptr correctionFactors = WorkspaceFactory::Instance().create(m_inputWS);

  // needs to be a signed because OpenMP gives an error otherwise
  const auto numHists = static_cast<int64_t>(m_inputWS->getNumberHistograms());
  const auto specSize = static_cast<int64_t>(m_inputWS->blocksize());
  if (specSize < 3)
    throw std::runtime_error("Problem in AnvredCorrection::events not binned");

  // If sample not at origin, shift cached positions.
  const auto &spectrumInfo = m_inputWS->spectrumInfo();
  const auto &componentInfo = m_inputWS->componentInfo();
  double L1 = spectrumInfo.l1();

  Progress prog(this, 0.0, 1.0, numHists);
  // Loop over the spectra
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWS, *correctionFactors))
  for (int64_t i = 0; i < int64_t(numHists); ++i) {
    PARALLEL_START_INTERRUPT_REGION

    // If no detector is found, skip onto the next spectrum
    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i))
      continue;

    Instrument_const_sptr inst = m_inputWS->getInstrument();
    UnitParametersMap pmap{};
    Mantid::Kernel::Units::Wavelength wl;
    Mantid::Kernel::Units::TOF tof;
    spectrumInfo.getDetectorValues(tof, wl, Kernel::DeltaEMode::Elastic, false, i, pmap);
    double L2 = pmap.at(UnitParams::l2);
    double scattering = pmap.at(UnitParams::twoTheta);

    double depth = 0.2;

    double pathlength = 0.0;

    std::string bankName;
    if (m_useScaleFactors) {
      const auto &det = spectrumInfo.detector(i);
      bankName = det.getParent()->getParent()->getName();
      scale_init(inst, componentInfo, L2, depth, pathlength, bankName);
    }

    auto points = m_inputWS->points(i);

    // share bin boundaries
    const auto &inSpec = m_inputWS->getSpectrum(i);
    correctionFactors->setSharedX(i, inSpec.sharedX());

    // get references to input data for calculations
    const auto &Yin = inSpec.y();
    const auto &Ein = inSpec.x();

    // Get a reference to the Y's in the output WS for storing the factors
    auto &Y = correctionFactors->mutableY(i);
    auto &E = correctionFactors->mutableE(i);
    // Loop through the bins in the current spectrum
    bool muRTooLarge = false;
    for (int64_t j = 0; j < specSize; j++) {

      double lambda = (unitStr == "TOF") ? wl.convertSingleFromTOF(points[j], L1, 0, pmap) : points[j];

      if (m_returnTransmissionOnly) {
        Y[j] = 1.0 / this->getEventWeight(lambda, scattering, muRTooLarge);
      } else {
        const auto eventWeight = this->getEventWeight(lambda, scattering, muRTooLarge);
        double scaleFactor(eventWeight);
        if (m_useScaleFactors)
          scaleFactor = scale_exec(bankName, lambda, depth, inst, pathlength, eventWeight);

        Y[j] = Yin[j] * scaleFactor;
        E[j] = Ein[j] * scaleFactor;
      }
    }

    if (muRTooLarge) {
      g_log.warning("Absorption correction not accurate for muR > 8 which was exceeded in spectrum index " +
                    std::to_string(i));
    }

    prog.report();

    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  // set the absorption correction values in the run parameters
  API::Run &run = correctionFactors->mutableRun();
  run.addProperty<double>("Radius", m_radius, true);
  if (!m_onlySphericalAbsorption && !m_returnTransmissionOnly)
    run.addProperty<bool>("LorentzCorrection", true, true);
  setProperty("OutputWorkspace", correctionFactors);
}

void AnvredCorrection::cleanup() {
  // Clear vectors to free up memory.
  m_lamda_weight.clear();
}

void AnvredCorrection::execEvent() {

  const auto numHists = static_cast<int64_t>(m_inputWS->getNumberHistograms());
  std::string unitStr = m_inputWS->getAxis(0)->unit()->unitID();
  auto correctionFactors = create<EventWorkspace>(*m_inputWS);
  correctionFactors->sortAll(TOF_SORT, nullptr);
  bool inPlace = (this->getPropertyValue("InputWorkspace") == this->getPropertyValue("OutputWorkspace"));
  if (inPlace)
    g_log.debug("Correcting EventWorkspace in-place.");

  // If sample not at origin, shift cached positions.
  Instrument_const_sptr inst = m_inputWS->getInstrument();

  const auto &spectrumInfo = eventW->spectrumInfo();
  const auto &componentInfo = eventW->componentInfo();
  double L1 = spectrumInfo.l1();

  Progress prog(this, 0.0, 1.0, numHists);
  // Loop over the spectra
  PARALLEL_FOR_IF(Kernel::threadSafe(*eventW, *correctionFactors))
  for (int64_t i = 0; i < int64_t(numHists); ++i) {
    PARALLEL_START_INTERRUPT_REGION

    // share bin boundaries, and leave Y and E nullptr
    correctionFactors->setHistogram(i, eventW->binEdges(i));

    // If no detector is found, skip onto the next spectrum
    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i))
      continue;

    UnitParametersMap pmap{};
    Mantid::Kernel::Units::Wavelength wl;
    Mantid::Kernel::Units::TOF tof;
    spectrumInfo.getDetectorValues(tof, wl, Kernel::DeltaEMode::Elastic, false, i, pmap);
    double L2 = pmap.at(UnitParams::l2);
    double scattering = pmap.at(UnitParams::twoTheta);

    EventList el = eventW->getSpectrum(i);
    el.switchTo(WEIGHTED_NOTIME);
    std::vector<WeightedEventNoTime> events = el.getWeightedEventsNoTime();

    double depth = 0.2;
    double pathlength = 0.0;
    std::string bankName;
    if (m_useScaleFactors) {
      const auto &det = spectrumInfo.detector(i);
      bankName = det.getParent()->getParent()->getName();
      scale_init(inst, componentInfo, L2, depth, pathlength, bankName);
    }

    // multiplying an event list by a scalar value
    bool muRTooLarge = false;

    for (auto &ev : events) {
      // get the event's TOF
      double lambda = ev.tof();

      if ("TOF" == unitStr)
        lambda = wl.convertSingleFromTOF(lambda, L1, 0, pmap);

      double value = this->getEventWeight(lambda, scattering, muRTooLarge);

      if (m_useScaleFactors)
        scale_exec(bankName, lambda, depth, inst, pathlength, value);

      ev.m_errorSquared = static_cast<float>(ev.m_errorSquared * value * value);
      ev.m_weight *= static_cast<float>(value);
    }

    if (muRTooLarge) {
      g_log.warning("Absorption correction not accurate for muR > 9 cm^-1 which was exceeded in spectrum index " +
                    std::to_string(i));
    }

    correctionFactors->getSpectrum(i) += events;

    // When focussing in place, you can clear out old memory from the input one!
    if (inPlace)
      eventW->getSpectrum(i).clear();

    prog.report();

    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  // set the absorption correction values in the run parameters
  API::Run &run = correctionFactors->mutableRun();
  run.addProperty<double>("Radius", m_radius, true);
  if (!m_onlySphericalAbsorption && !m_returnTransmissionOnly)
    run.addProperty<bool>("LorentzCorrection", true, true);
  setProperty("OutputWorkspace", std::move(correctionFactors));

  // Now do some cleaning-up since destructor may not be called immediately
  this->cleanup();
}

/// Fetch the properties and set the appropriate member variables
void AnvredCorrection::retrieveBaseProperties() {
  m_smu = getProperty("LinearScatteringCoef"); // in 1/cm
  m_amu = getProperty("LinearAbsorptionCoef"); // in 1/cm
  m_radius = getProperty("Radius");            // in cm
  m_power_th = getProperty("PowerLambda");     // in cm
  const Material &sampleMaterial = m_inputWS->sample().getMaterial();

  const double scatterXSection = sampleMaterial.totalScatterXSection();

  if (scatterXSection != 0.0) {
    double rho = sampleMaterial.numberDensity();
    if (m_smu == EMPTY_DBL())
      m_smu = scatterXSection * rho;
    if (m_amu == EMPTY_DBL())
      m_amu = sampleMaterial.absorbXSection(NeutronAtom::ReferenceLambda) * rho;
    if (m_radius == EMPTY_DBL())
      m_radius =
          m_inputWS->sample().getShape().shapeInfo().sphereGeometry().radius * 100; // convert radius from m to cm
  } else // Save input in Sample with wrong atomic number and name
  {
    NeutronAtom neutron(0, 0, 0.0, 0.0, m_smu, 0.0, m_smu, m_amu);
    auto shape = std::shared_ptr<IObject>(
        m_inputWS->sample().getShape().cloneWithMaterial(Material("SetInAnvredCorrection", neutron, 1.0)));
    m_inputWS->mutableSample().setShape(shape);
  }
  if (m_smu != EMPTY_DBL() && m_amu != EMPTY_DBL())
    g_log.notice() << "LinearScatteringCoef = " << m_smu << " 1/cm\n"
                   << "LinearAbsorptionCoef = " << m_amu << " 1/cm\n"
                   << "Radius = " << m_radius << " cm\n"
                   << "Power Lorentz corrections = " << m_power_th << " \n";
  // Call the virtual function for any further properties
  retrieveProperties();
}

/**
 *  Get the weight factor that would be used for an event occuring
 *  at the specified wavelength, with the specified two_theta value.
 *
 *  @param  lamda      The wavelength of an event.
 *  @param  two_theta  The scattering angle of the event.
 *  @param muRTooLarge bool to warn in muR limit exceeded in absorption correction
 *  @return The weight factor for the specified position and wavelength.
 */
double AnvredCorrection::getEventWeight(const double lamda, const double two_theta, bool &muRTooLarge) {
  double transinv = 1;
  if (m_radius > 0)
    transinv = absor_sphere(two_theta, lamda, muRTooLarge);
  // Only Spherical absorption correction
  if (m_onlySphericalAbsorption || m_returnTransmissionOnly)
    return transinv;

  // Resolution of the lambda table
  auto lamda_index = static_cast<size_t>(STEPS_PER_ANGSTROM * lamda);

  if (lamda_index >= m_lamda_weight.size())
    lamda_index = m_lamda_weight.size() - 1;

  double lamda_w = m_lamda_weight[lamda_index];

  double sin_theta = std::sin(two_theta / 2);
  double pix_weight = sin_theta * sin_theta;

  double event_weight = pix_weight * lamda_w * transinv;

  return event_weight;
}

/**
 *       function to calculate a spherical absorption correction
 *       and tbar. based on values in:
 *
 *       Weber, K., Acta Cryst. B, 25.6 (1969)
 *
 *       in this paper, a is the transmission and a* = 1/a is
 *       the absorption correction.
 *
 *       input are the smu (scattering) and amu (absorption at 1.8 ang.)
 *       linear absorption coefficients, the radius r of the sample
 *       the theta angle and wavelength.
 *       the absorption (absn) and tbar are returned.
 *
 *       a. j. schultz, june, 2008
 *
 *       @param twoth scattering angle
 *       @param wl scattering wavelength
 *       @param muRTooLarge bool to warn in muR limit exceeded
 *       @returns absorption
 */
double AnvredCorrection::absor_sphere(const double twoth, const double wl, bool &muRTooLarge) {
  //  For each of the 19 theta values in (theta = 0:5:90 deg)
  //  fitted ln(1/A*) = sum_{icoef=0}^{N=7} pc[7-icoef][ith]*(muR)^icoef
  //  using A* values in Weber (1969) for 0 < muR < 8.
  //  These values are given in the static array pc[][]

  double mur = (m_smu + (m_amu / 1.8f) * wl) * m_radius;
  if (mur < 0.) {
    throw std::runtime_error("muR cannot be negative");
  } else if (mur > 8.0) {
    muRTooLarge = true;
  }

  auto theta = 0.5 * twoth * radtodeg;
  if (theta < 0. || theta > 90.) {
    std::ostringstream s;
    s << theta;
    throw std::runtime_error("theta is not in allowed range :" + s.str());
  }

  // tbar = -(double)Math.log(trans)/mu;  // as defined by coppens
  // trans = exp(-mu*tbar)
  return calc_Astar(theta, mur);
}

/*
 * Helper function to calc Astar to be called from SaveHKL
 *       @param theta: half scattering angle (i.e. twotheta/2)
 *       @param mur: muR is the product of linenar attenuation and sphere radius
 *       @returns astar: 1/transmission
 */
double AnvredCorrection::calc_Astar(const double theta, const double mur) {
  //  interpolation better done on A = 1/A* = transmission
  auto ith = static_cast<size_t>(theta / 5.); // floor
  double lnA_1 = 0.0;
  double lnA_2 = 0.0;
  size_t ncoef = sizeof pc / sizeof pc[0]; // order of poly
  for (size_t icoef = 0; icoef < ncoef; icoef++) {
    lnA_1 = lnA_1 * mur + pc[icoef][ith];     // previous theta
    lnA_2 = lnA_2 * mur + pc[icoef][ith + 1]; // next theta
  }
  double A1 = std::exp(lnA_1);
  double sin_th1_sq = std::pow(sin((static_cast<double>(ith) * 5.0) / radtodeg), 2);
  double A2 = std::exp(lnA_2);
  double sin_th2_sq = std::pow(sin((static_cast<double>(ith + 1) * 5.0) / radtodeg), 2);
  // interpolate between theta values using
  // A(th) = L0 + L1*(sin(th)^2)
  double L1 = (A1 - A2) / (sin_th1_sq - sin_th2_sq);
  double L0 = A1 - L1 * sin_th1_sq;

  // correction to apply (A* = 1/A = 1/transmission)
  return 1 / (L0 + L1 * std::pow(sin(theta / radtodeg), 2));
}
/**
 *  Build the list of weights corresponding to different wavelengths.
 *  Although the spectrum file need not have a fixed number of
 *  points, it MUST have the spectrum recorded as a histogram with one
 *  more bin boundary than the number of bins.
 *    The entries in the table produced are:
 *
 *     1/( lamda^power * spec(lamda) )
 *
 *  Where power was chosen to give a relatively uniform intensity display
 *  in 3D.  The power is currently 3 if an incident spectrum is present
 *  and 2.4 if no incident spectrum is used.
 */
void AnvredCorrection::BuildLamdaWeights() {
  // Theoretically correct value 3.0;
  // if we have an incident spectrum
  //  double power_ns = 2.4;                   // lower power needed to find
  // peaks in ARCS data with no
  // incident spectrum

  // GetSpectrumWeights( spectrum_file_name, m_lamda_weight);

  if (m_lamda_weight.empty()) // loading spectrum failed so use
  {                           // array of 1's
    //    power = power_ns;                      // This is commented out, so we
    // don't override user specified
    // value.
    m_lamda_weight.reserve(NUM_WAVELENGTHS);
    for (int i = 0; i < NUM_WAVELENGTHS; i++) {
      double lamda = static_cast<double>(i) / STEPS_PER_ANGSTROM;
      m_lamda_weight.emplace_back(1 / std::pow(lamda, m_power_th));
    }
  }
}

void AnvredCorrection::scale_init(const Instrument_const_sptr &inst, const ComponentInfo &componentInfo,
                                  const double L2, const double depth, double &pathlength,
                                  const std::string &bankName) {
  // Distance to center of detector
  const IComponent *det0 = inst->getComponentByName(bankName).get();
  if ("CORELLI" == inst->getName()) // for Corelli with sixteenpack under bank
  {
    const size_t bankIndex = componentInfo.indexOfAny(bankName);
    const auto children = componentInfo.children(bankIndex);
    if (!children.empty()) {
      det0 = componentInfo.componentID(children[0]);
    }
  }
  IComponent_const_sptr sample = inst->getSample();
  double cosA = det0->getDistance(*sample) / L2;
  pathlength = depth / cosA;
}

double AnvredCorrection::scale_exec(std::string &bankName, const double lambda, const double depth,
                                    const Instrument_const_sptr &inst, const double pathlength, double eventWeight) {
  // correct for the slant path throught the scintillator glass
  const double mu = (9.614 * lambda) + 0.266;            // mu for GS20 glass
  const double eff_center = 1.0 - std::exp(-mu * depth); // efficiency at center of detector
  const double eff_R = 1.0 - exp(-mu * pathlength);      // efficiency at point R
  double scaleFactor(eventWeight * eff_center / eff_R);  // slant path efficiency ratio
  // Take out the "bank" part of the bank name
  bankName.erase(remove_if(bankName.begin(), bankName.end(), std::not_fn(::isdigit)), bankName.end());
  if (inst->hasParameter("detScale" + bankName))
    scaleFactor *= static_cast<double>(inst->getNumberParameter("detScale" + bankName)[0]);

  return scaleFactor;
}

} // namespace Mantid::Crystal
