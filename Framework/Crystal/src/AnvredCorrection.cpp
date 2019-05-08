// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
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

namespace Mantid {
namespace Crystal {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(AnvredCorrection)

using namespace Kernel;
using namespace Geometry;
using namespace API;
using namespace DataObjects;
using namespace Mantid::PhysicalConstants;

AnvredCorrection::AnvredCorrection()
    : API::Algorithm(), m_smu(0.), m_amu(0.), m_radius(0.), m_power_th(0.),
      m_lamda_weight(), m_onlySphericalAbsorption(false),
      m_returnTransmissionOnly(false), m_useScaleFactors(false) {}

void AnvredCorrection::init() {

  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = boost::make_shared<InstrumentValidator>();

  declareProperty(make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "The X values for the input workspace must be in units of "
                  "wavelength or TOF");
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Output workspace name");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("LinearScatteringCoef", EMPTY_DBL(), mustBePositive,
                  "Linear scattering coefficient in 1/cm if not set with "
                  "SetSampleMaterial");
  declareProperty("LinearAbsorptionCoef", EMPTY_DBL(), mustBePositive,
                  "Linear absorption coefficient at 1.8 Angstroms in 1/cm if "
                  "not set with SetSampleMaterial");
  declareProperty("Radius", EMPTY_DBL(), mustBePositive,
                  "Radius of the sample in centimeters");
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

  BuildLamdaWeights();

  eventW = boost::dynamic_pointer_cast<EventWorkspace>(m_inputWS);
  if (eventW)
    eventW->sortAll(TOF_SORT, nullptr);
  if ((getProperty("PreserveEvents")) && (eventW != nullptr) &&
      !m_returnTransmissionOnly) {
    // Input workspace is an event workspace. Use the other exec method
    this->execEvent();
    this->cleanup();
    return;
  }

  MatrixWorkspace_sptr correctionFactors =
      WorkspaceFactory::Instance().create(m_inputWS);

  // needs to be a signed because OpenMP gives an error otherwise
  const int64_t numHists =
      static_cast<int64_t>(m_inputWS->getNumberHistograms());
  const int64_t specSize = static_cast<int64_t>(m_inputWS->blocksize());
  if (specSize < 3)
    throw std::runtime_error("Problem in AnvredCorrection::events not binned");

  // If sample not at origin, shift cached positions.
  const auto &spectrumInfo = m_inputWS->spectrumInfo();
  double L1 = spectrumInfo.l1();

  Progress prog(this, 0.0, 1.0, numHists);
  // Loop over the spectra
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWS, *correctionFactors))
  for (int64_t i = 0; i < int64_t(numHists); ++i) {
    PARALLEL_START_INTERUPT_REGION

    // If no detector is found, skip onto the next spectrum
    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i))
      continue;

    // This is the scattered beam direction
    Instrument_const_sptr inst = m_inputWS->getInstrument();
    double L2 = spectrumInfo.l2(i);
    // Two-theta = polar angle = scattering angle = between +Z vector and the
    // scattered beam
    double scattering = spectrumInfo.twoTheta(i);

    double depth = 0.2;

    double pathlength = 0.0;

    std::string bankName;

    const auto &det = spectrumInfo.detector(i);
    if (m_useScaleFactors)
      scale_init(det, inst, L2, depth, pathlength, bankName);

    Mantid::Kernel::Units::Wavelength wl;
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
    for (int64_t j = 0; j < specSize; j++) {

      double lambda =
          (unitStr == "TOF")
              ? wl.convertSingleFromTOF(points[j], L1, L2, scattering, 0, 0, 0)
              : points[j];

      if (m_returnTransmissionOnly) {
        Y[j] = 1.0 / this->getEventWeight(lambda, scattering);
      } else {
        double value = this->getEventWeight(lambda, scattering);

        if (m_useScaleFactors)
          scale_exec(bankName, lambda, depth, inst, pathlength, value);

        Y[j] = Yin[j] * value;
        E[j] = Ein[j] * value;
      }
    }

    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

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

  const int64_t numHists =
      static_cast<int64_t>(m_inputWS->getNumberHistograms());
  std::string unitStr = m_inputWS->getAxis(0)->unit()->unitID();
  auto correctionFactors = create<EventWorkspace>(*m_inputWS);
  correctionFactors->sortAll(TOF_SORT, nullptr);
  bool inPlace = (this->getPropertyValue("InputWorkspace") ==
                  this->getPropertyValue("OutputWorkspace"));
  if (inPlace)
    g_log.debug("Correcting EventWorkspace in-place.");

  // If sample not at origin, shift cached positions.
  Instrument_const_sptr inst = m_inputWS->getInstrument();

  const auto &spectrumInfo = eventW->spectrumInfo();
  double L1 = spectrumInfo.l1();

  Progress prog(this, 0.0, 1.0, numHists);
  // Loop over the spectra
  PARALLEL_FOR_IF(Kernel::threadSafe(*eventW, *correctionFactors))
  for (int64_t i = 0; i < int64_t(numHists); ++i) {
    PARALLEL_START_INTERUPT_REGION

    // share bin boundaries, and leave Y and E nullptr
    correctionFactors->setHistogram(i, eventW->binEdges(i));

    // If no detector is found, skip onto the next spectrum
    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i))
      continue;

    // This is the scattered beam direction
    double L2 = spectrumInfo.l2(i);
    // Two-theta = polar angle = scattering angle = between +Z vector and the
    // scattered beam
    double scattering = spectrumInfo.twoTheta(i);

    EventList el = eventW->getSpectrum(i);
    el.switchTo(WEIGHTED_NOTIME);
    std::vector<WeightedEventNoTime> events = el.getWeightedEventsNoTime();

    Mantid::Kernel::Units::Wavelength wl;

    double depth = 0.2;
    double pathlength = 0.0;
    std::string bankName;
    const auto &det = spectrumInfo.detector(i);
    if (m_useScaleFactors)
      scale_init(det, inst, L2, depth, pathlength, bankName);

    // multiplying an event list by a scalar value

    for (auto &ev : events) {
      // get the event's TOF
      double lambda = ev.tof();

      if ("TOF" == unitStr)
        lambda = wl.convertSingleFromTOF(lambda, L1, L2, scattering, 0, 0, 0);

      double value = this->getEventWeight(lambda, scattering);

      if (m_useScaleFactors)
        scale_exec(bankName, lambda, depth, inst, pathlength, value);

      ev.m_errorSquared = static_cast<float>(ev.m_errorSquared * value * value);
      ev.m_weight *= static_cast<float>(value);
    }

    correctionFactors->getSpectrum(i) += events;

    // When focussing in place, you can clear out old memory from the input one!
    if (inPlace)
      eventW->getSpectrum(i).clear();

    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

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

  const double scatterXSection =
      sampleMaterial.totalScatterXSection(NeutronAtom::ReferenceLambda);

  if (scatterXSection != 0.0) {
    double rho = sampleMaterial.numberDensity();
    if (m_smu == EMPTY_DBL())
      m_smu = scatterXSection * rho;
    if (m_amu == EMPTY_DBL())
      m_amu = sampleMaterial.absorbXSection(NeutronAtom::ReferenceLambda) * rho;
  } else // Save input in Sample with wrong atomic number and name
  {
    NeutronAtom neutron(0, 0, 0.0, 0.0, m_smu, 0.0, m_smu, m_amu);
    auto shape = boost::shared_ptr<IObject>(
        m_inputWS->sample().getShape().cloneWithMaterial(
            Material("SetInAnvredCorrection", neutron, 1.0)));
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
 *
 *  @return The weight factor for the specified position and wavelength.
 */
double AnvredCorrection::getEventWeight(double lamda, double two_theta) {
  double transinv = 1;
  if (m_radius > 0)
    transinv = absor_sphere(two_theta, lamda);
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
 *       c. w. dwiggins, jr., acta cryst. a31, 395 (1975).
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
 *       @returns absorption
 */
double AnvredCorrection::absor_sphere(double &twoth, double &wl) {
  int i;
  double mu, mur; // mu is the linear absorption coefficient,
  // r is the radius of the spherical sample.
  double theta, astar1, astar2, frac, astar;
  //  double trans;
  //  double tbar;

  //  For each of the 19 theta values in dwiggins (theta = 0.0 to 90.0
  //  in steps of 5.0 deg.), the astar values vs.mur were fit to a third
  //  order polynomial in excel. these values are given in the static array
  //  pc[][]

  mu = m_smu + (m_amu / 1.8f) * wl;

  mur = mu * m_radius;
  if (mur < 0. || mur > 2.5) {
    std::ostringstream s;
    s << mur;
    throw std::runtime_error("muR is not in range of Dwiggins' table :" +
                             s.str());
  }

  theta = twoth * radtodeg_half;
  if (theta < 0. || theta > 90.) {
    std::ostringstream s;
    s << theta;
    throw std::runtime_error("theta is not in range of Dwiggins' table :" +
                             s.str());
  }

  //  using the polymial coefficients, calulate astar (= 1/transmission) at
  //  theta values below and above the actual theta value.

  i = static_cast<int>(theta / 5.);
  astar1 = pc[0][i] + mur * (pc[1][i] + mur * (pc[2][i] + pc[3][i] * mur));

  i = i + 1;
  astar2 = pc[0][i] + mur * (pc[1][i] + mur * (pc[2][i] + pc[3][i] * mur));

  //  do a linear interpolation between theta values.

  frac = theta -
         static_cast<double>(static_cast<int>(theta / 5.)) * 5.; // theta%5.
  frac = frac / 5.;

  astar = astar1 * (1 - frac) + astar2 * frac; // astar is the correction
  //  trans = 1.f/astar;                           // trans is the transmission
  // trans = exp(-mu*tbar)

  //  calculate tbar as defined by coppens.
  //  tbar = -(double)Math.log(trans)/mu;

  return astar;
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

  double power = m_power_th;

  // GetSpectrumWeights( spectrum_file_name, m_lamda_weight);

  if (m_lamda_weight.empty()) // loading spectrum failed so use
  {                           // array of 1's
    //    power = power_ns;                      // This is commented out, so we
    // don't override user specified
    // value.
    m_lamda_weight.reserve(NUM_WAVELENGTHS);
    for (int i = 0; i < NUM_WAVELENGTHS; i++)
      m_lamda_weight.push_back(1.);
  }

  for (size_t i = 0; i < m_lamda_weight.size(); ++i) {
    double lamda = static_cast<double>(i) / STEPS_PER_ANGSTROM;
    m_lamda_weight[i] *= (1 / std::pow(lamda, power));
  }
}

void AnvredCorrection::scale_init(const IDetector &det,
                                  Instrument_const_sptr inst, double &L2,
                                  double &depth, double &pathlength,
                                  std::string &bankName) {
  bankName = det.getParent()->getParent()->getName();
  // Distance to center of detector
  boost::shared_ptr<const IComponent> det0 = inst->getComponentByName(bankName);
  if ("CORELLI" == inst->getName()) // for Corelli with sixteenpack under bank
  {
    std::vector<Geometry::IComponent_const_sptr> children;
    auto asmb = boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(
        inst->getComponentByName(bankName));
    asmb->getChildren(children, false);
    det0 = children[0];
  }
  IComponent_const_sptr sample = inst->getSample();
  double cosA = det0->getDistance(*sample) / L2;
  pathlength = depth / cosA;
}

void AnvredCorrection::scale_exec(std::string &bankName, double &lambda,
                                  double &depth, Instrument_const_sptr inst,
                                  double &pathlength, double &value) {
  // correct for the slant path throught the scintillator glass
  double mu = (9.614 * lambda) + 0.266; // mu for GS20 glass
  double eff_center =
      1.0 - std::exp(-mu * depth); // efficiency at center of detector
  double eff_R = 1.0 - exp(-mu * pathlength); // efficiency at point R
  value *= eff_center / eff_R;                // slant path efficiency ratio
  // Take out the "bank" part of the bank name
  bankName.erase(remove_if(bankName.begin(), bankName.end(),
                           not1(std::ptr_fun(::isdigit))),
                 bankName.end());
  if (inst->hasParameter("detScale" + bankName))
    value *=
        static_cast<double>(inst->getNumberParameter("detScale" + bankName)[0]);
}

} // namespace Crystal
} // namespace Mantid
