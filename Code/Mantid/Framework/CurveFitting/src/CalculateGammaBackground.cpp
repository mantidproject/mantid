#include "MantidCurveFitting/CalculateGammaBackground.h"
#include "MantidCurveFitting/ComptonProfile.h"
#include "MantidCurveFitting/ConvertToYSpace.h"
#include "MantidCurveFitting/VesuvioResolution.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceValidators.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/PhysicalConstants.h"

#include "MantidGeometry/Instrument/ReferenceFrame.h"

namespace Mantid {
namespace CurveFitting {
using namespace API;
using namespace Kernel;
using namespace std;

// Subscription
DECLARE_ALGORITHM(CalculateGammaBackground);

namespace {
/// Number of elements in theta direction integration
size_t NTHETA = 5;
/// Number of elements in up direction integration
size_t NUP = 5;

/// Degrees to radians
double DEG2RAD = M_PI / 180.0;
/// Wavelength of absorption (30603e-24 * 6e19). Constant came from VMS
double ABSORB_WAVELENGTH = 1.83618;
/// Start of forward scattering spectrum numbers (inclusive)
specid_t FORWARD_SCATTER_SPECMIN = 135;
/// End of forward scattering spectrum numbers (inclusive)
specid_t FORWARD_SCATTER_SPECMAX = 198;
}

//--------------------------------------------------------------------------------------------------------
// Public members
//--------------------------------------------------------------------------------------------------------

/// Default constructor
CalculateGammaBackground::CalculateGammaBackground()
    : Algorithm(), m_inputWS(), m_indices(), m_profileFunction(), m_npeaks(0),
      m_reversed(), m_samplePos(), m_l1(0.0), m_foilRadius(0.0),
      m_foilUpMin(0.0), m_foilUpMax(0.0), m_foils0(), m_foils1(),
      m_backgroundWS(), m_correctedWS(), m_progress(NULL) {}

/// Destructor
CalculateGammaBackground::~CalculateGammaBackground() {
  delete m_progress;
  m_indices.clear();
}

//--------------------------------------------------------------------------------------------------------
// Private members
//--------------------------------------------------------------------------------------------------------

const std::string CalculateGammaBackground::name() const {
  return "CalculateGammaBackground";
}

int CalculateGammaBackground::version() const { return 1; }

const std::string CalculateGammaBackground::category() const {
  return "CorrectionFunctions";
}

void CalculateGammaBackground::init() {

  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("TOF");
  wsValidator->add<HistogramValidator>(false); // point data
  declareProperty(new WorkspaceProperty<>("InputWorkspace", "",
                                          Direction::Input, wsValidator),
                  "An input workspace containing TOF data");

  declareProperty(
      new API::FunctionProperty("ComptonFunction"),
      "Function that is able to compute the mass spectrum for the input data"
      "This will usually be the output from the Fitting");

  declareProperty(new ArrayProperty<int>("WorkspaceIndexList"),
                  "Indices of the spectra to include in the correction. If "
                  "provided, the output only include these spectra\n"
                  "(Default: all spectra from input)");

  declareProperty(
      new WorkspaceProperty<>("BackgroundWorkspace", "", Direction::Output),
      "A new workspace containing the calculated background.");
  declareProperty(
      new WorkspaceProperty<>("CorrectedWorkspace", "", Direction::Output),
      "A new workspace containing the calculated background subtracted from "
      "the input.");
}

void CalculateGammaBackground::exec() {
  retrieveInputs();
  createOutputWorkspaces();

  const int64_t nhist = static_cast<int64_t>(m_indices.size());
  const int64_t nreports =
      10 + nhist * (m_npeaks + 2 * m_foils0.size() * NTHETA * NUP * m_npeaks);
  m_progress = new Progress(this, 0.0, 1.0, nreports);

  PARALLEL_FOR3(m_inputWS, m_correctedWS, m_backgroundWS)
  for (int64_t i = 0; i < nhist; ++i) {
    PARALLEL_START_INTERUPT_REGION
    const size_t outputIndex = i;
    auto indexIter = m_indices.cbegin();
    std::advance(indexIter, i);
    const size_t inputIndex = indexIter->second;

    if (!calculateBackground(inputIndex, outputIndex)) {
      g_log.information("No detector defined for index=" +
                        boost::lexical_cast<std::string>(inputIndex) +
                        ". Skipping correction.");
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("BackgroundWorkspace", m_backgroundWS);
  setProperty("CorrectedWorkspace", m_correctedWS);
}

/**
 * Calculate the background from the input spectrum and assign the value to the
 * output one
 * @param inputIndex The index on the input workspace on which to operate
 * @param outputIndex The index on the output workspace where the results are
 * stored
 * @return True if the background was subtracted, false otherwise
 */
bool CalculateGammaBackground::calculateBackground(const size_t inputIndex,
                                                   const size_t outputIndex) {
  // Copy X values
  m_backgroundWS->setX(outputIndex, m_inputWS->refX(inputIndex));
  m_correctedWS->setX(outputIndex, m_inputWS->refX(inputIndex));
  // Copy errors to corrected
  m_correctedWS->dataE(outputIndex) = m_inputWS->readE(inputIndex);

  try {
    const auto *inSpec = m_inputWS->getSpectrum(inputIndex);
    const specid_t spectrumNo(inSpec->getSpectrumNo());
    m_backgroundWS->getSpectrum(outputIndex)->copyInfoFrom(*inSpec);
    m_correctedWS->getSpectrum(outputIndex)->copyInfoFrom(*inSpec);

    if (spectrumNo >= FORWARD_SCATTER_SPECMIN &&
        spectrumNo <= FORWARD_SCATTER_SPECMAX) {
      applyCorrection(inputIndex, outputIndex);
    } else {
      g_log.information("Spectrum " +
                        boost::lexical_cast<std::string>(spectrumNo) +
                        " not in forward scatter range. Skipping correction.");
      // Leave background at 0 and just copy data to corrected
      m_correctedWS->dataY(outputIndex) = m_inputWS->readY(inputIndex);
    }
    return true;
  } catch (Exception::NotFoundError &) {
    return false;
  }
}

/**
* Calculate & apply gamma correction for the given index of the
* input workspace
* @param inputIndex A workspace index that defines the input spectrum to correct
* @param outputIndex A workspace index that defines the output to hold the
* results
*/
void CalculateGammaBackground::applyCorrection(const size_t inputIndex,
                                               const size_t outputIndex) {
  m_progress->report("Computing TOF from detector");

  // results go straight in m_correctedWS to save memory allocations
  calculateSpectrumFromDetector(inputIndex, outputIndex);

  m_progress->report("Computing TOF foils");
  // Output goes to m_background to save memory allocations
  calculateBackgroundFromFoils(inputIndex, outputIndex);

  m_progress->report("Computing correction to input");
  // Compute total counts from input data, (detector-foil) contributions
  // assume constant binning
  const size_t nbins = m_correctedWS->blocksize();
  const auto &inY = m_inputWS->readY(inputIndex);
  auto &detY = m_correctedWS->dataY(outputIndex);
  auto &foilY = m_backgroundWS->dataY(outputIndex);
  const double deltaT = m_correctedWS->readX(outputIndex)[1] -
                        m_correctedWS->readX(outputIndex)[0];

  double dataCounts(0.0), simulCounts(0.0);
  for (size_t j = 0; j < nbins; ++j) {
    dataCounts += inY[j] * deltaT;
    simulCounts += (detY[j] - foilY[j]) * deltaT;
  }

  // Now corrected for the calculated background
  const double corrFactor = dataCounts / simulCounts;
  if (g_log.is(Logger::Priority::PRIO_INFORMATION))
    g_log.information() << "Correction factor for background=" << corrFactor
                        << "\n";

  for (size_t j = 0; j < nbins; ++j) {
    // m_backgroundWS already contains the foil values, careful not to overwrite
    // them
    double &foilValue = foilY[j]; // non-const reference
    foilValue *= corrFactor;
    detY[j] = (inY[j] - foilValue);
  }
}

/**
* Results are placed in the mapped index on the output corrected workspace
* @param inputIndex Workspace index that defines the input spectrum to correct
* @param outputIndex Workspace index that defines the spectrum to hold the
* results
*/
void CalculateGammaBackground::calculateSpectrumFromDetector(
    const size_t inputIndex, const size_t outputIndex) {
  // -- Setup detector & resolution parameters --
  DetectorParams detPar =
      ConvertToYSpace::getDetectorParameters(m_inputWS, inputIndex);
  ResolutionParams detRes =
      VesuvioResolution::getResolutionParameters(m_inputWS, inputIndex);

  // Compute a time of flight spectrum convolved with a Voigt resolution
  // function for each mass
  // at the detector point & sum to a single spectrum
  auto &ctdet = m_correctedWS->dataY(outputIndex);
  std::vector<double> tmpWork(ctdet.size());
  calculateTofSpectrum(ctdet, tmpWork, outputIndex, detPar, detRes);
  // Correct for distance to the detector: 0.5/l2^2
  const double detDistCorr = 0.5 / detPar.l2 / detPar.l2;
  std::transform(ctdet.begin(), ctdet.end(), ctdet.begin(),
                 std::bind2nd(std::multiplies<double>(), detDistCorr));
}

/**
* Calculate & apply gamma correction for the given index of the
* input workspace
* @param inputIndex Workspace index that defines the input spectrum to correct
* @param outputIndex Workspace index that defines the spectrum to hold the
* results
*/
void CalculateGammaBackground::calculateBackgroundFromFoils(
    const size_t inputIndex, const size_t outputIndex) {
  // -- Setup detector & resolution parameters --
  DetectorParams detPar =
      ConvertToYSpace::getDetectorParameters(m_inputWS, inputIndex);
  ResolutionParams detRes =
      VesuvioResolution::getResolutionParameters(m_inputWS, inputIndex);

  const size_t nxvalues = m_backgroundWS->blocksize();
  std::vector<double> foilSpectrum(nxvalues);
  auto &ctfoil = m_backgroundWS->dataY(outputIndex);

  // Compute (C1 - C0) where C1 is counts in pos 1 and C0 counts in pos 0
  assert(m_foils0.size() == m_foils1.size());
  for (size_t i = 0; i < m_foils0.size(); ++i) {
    foilSpectrum.assign(nxvalues, 0.0);
    calculateBackgroundSingleFoil(foilSpectrum, outputIndex, m_foils1[i],
                                  detPar.pos, detPar, detRes);
    // sum spectrum values from first position
    std::transform(ctfoil.begin(), ctfoil.end(), foilSpectrum.begin(),
                   ctfoil.begin(), std::plus<double>());

    foilSpectrum.assign(nxvalues, 0.0);
    calculateBackgroundSingleFoil(foilSpectrum, outputIndex, m_foils0[i],
                                  detPar.pos, detPar, detRes);
    // subtract spectrum values from zeroth position
    std::transform(ctfoil.begin(), ctfoil.end(), foilSpectrum.begin(),
                   ctfoil.begin(), std::minus<double>());
  }
  bool reversed = (m_reversed.count(m_inputWS->getSpectrum(inputIndex)
                                        ->getSpectrumNo()) != 0);
  // This is quicker than the if within the loop
  if (reversed) {
    // The reversed ones should be (C0 - C1)
    std::transform(ctfoil.begin(), ctfoil.end(), ctfoil.begin(),
                   std::bind2nd(std::multiplies<double>(), -1.0));
  }
}

/**
* Integrates over the foil area defined by the foil radius to accumulate an
* estimate of the counts
* resulting from this region
* @param ctfoil Output vector to hold results
* @param wsIndex Index on output background workspaces currently operating
* @param foilInfo Foil description object
* @param detPos The pre-calculated detector V3D
* @param detPar DetectorParams object that defines information on the detector
* associated with spectrum at wsIndex
* @param detRes ResolutionParams object that defines information on the
* resolution associated with spectrum at wsIndex
*/
void CalculateGammaBackground::calculateBackgroundSingleFoil(
    std::vector<double> &ctfoil, const size_t wsIndex, const FoilInfo &foilInfo,
    const V3D &detPos, const DetectorParams &detPar,
    const ResolutionParams &detRes) {
  /** Integrates over the foils
  *  by dividing into 2cm^2 elements
  *  The integration is performed in cylindrical coordinates
  */

  const double thetaStep =
      (foilInfo.thetaMax - foilInfo.thetaMin) / static_cast<double>(NTHETA);
  const double thetaStepRad = thetaStep * DEG2RAD;
  const double upStep = (m_foilUpMax - m_foilUpMin) / static_cast<double>(NUP);
  const double elementArea = abs(m_foilRadius * upStep * thetaStepRad);
  V3D elementPos; // reusable vector for computing distances

  // Structs to hold geometry & resolution information
  DetectorParams foilPar = detPar; // copy
  foilPar.t0 = 0.0;
  ResolutionParams foilRes = detRes; // copy
  foilRes.dEnGauss = foilInfo.gaussWidth;
  foilRes.dEnLorentz = foilInfo.lorentzWidth;

  const size_t nvalues = ctfoil.size();
  std::vector<double> singleElement(nvalues), tmpWork(nvalues);

  for (size_t i = 0; i < NTHETA; ++i) {
    double thetaZeroRad =
        (foilInfo.thetaMin + (static_cast<double>(i) + 0.5) * thetaStep) *
        DEG2RAD;
    elementPos.setZ(m_foilRadius * cos(thetaZeroRad));
    elementPos.setX(m_foilRadius * sin(thetaZeroRad));
    for (size_t j = 0; j < NUP; ++j) {
      double ypos = m_foilUpMin + (static_cast<double>(j) + 0.5) * upStep;
      elementPos.setY(ypos);
      foilPar.l2 = m_samplePos.distance(elementPos);
      foilPar.theta = acos(m_foilRadius * cos(thetaZeroRad) /
                           foilPar.l2); // scattering angle in radians

      // Spectrum for this position
      singleElement.assign(nvalues, 0.0);
      calculateTofSpectrum(singleElement, tmpWork, wsIndex, foilPar, foilRes);

      // Correct for absorption & distance
      const double den = (elementPos.Z() * cos(thetaZeroRad) +
                          elementPos.X() * sin(thetaZeroRad));
      const double absorbFactor =
          1.0 / (1.0 - exp(-ABSORB_WAVELENGTH * foilPar.l2 / den));
      const double elemDetDist = elementPos.distance(detPos);
      const double distFactor =
          elementArea /
          (4.0 * M_PI * foilPar.l2 * foilPar.l2 * elemDetDist * elemDetDist);
      // Add on to other contributions
      for (size_t k = 0; k < nvalues; ++k) {
        ctfoil[k] += singleElement[k] * absorbFactor * distFactor;
      }
    }
  }
}

/**
* Uses the compton profile functions to compute a particular mass spectrum
* @param result [Out] The value of the computed spectrum
* @param tmpWork [In] Pre-allocated working area that will be overwritten
* @param wsIndex Index on the output background workspace that gives the X
* values to use
* @param detpar Struct containing parameters about the detector
* @param respar Struct containing parameters about the resolution
*/
void CalculateGammaBackground::calculateTofSpectrum(
    std::vector<double> &result, std::vector<double> &tmpWork,
    const size_t wsIndex, const DetectorParams &detpar,
    const ResolutionParams &respar) {
  assert(result.size() == tmpWork.size());

  // Assumes the input is in seconds, transform it temporarily
  auto &tseconds = m_backgroundWS->dataX(wsIndex);
  std::transform(tseconds.begin(), tseconds.end(), tseconds.begin(),
                 std::bind2nd(std::multiplies<double>(), 1e-6));

  // retrieveInputs ensures we will get a composite function and that each
  // member is a ComptonProfile
  // we can't static_cast though due to the virtual inheritance with IFunction
  auto profileFunction = boost::dynamic_pointer_cast<CompositeFunction>(
      FunctionFactory::Instance().createInitialized(m_profileFunction));

  for (size_t i = 0; i < m_npeaks; ++i) {
    auto profile = boost::dynamic_pointer_cast<ComptonProfile>(
        profileFunction->getFunction(i));
    profile->disableLogging();
    profile->setUpForFit();
    profile->cacheYSpaceValues(tseconds, false, detpar, respar);

    profile->massProfile(tmpWork.data(), tmpWork.size());
    // Add to final result
    std::transform(result.begin(), result.end(), tmpWork.begin(),
                   result.begin(), std::plus<double>());
    m_progress->report();
  }
  // Put X back microseconds
  std::transform(tseconds.begin(), tseconds.end(), tseconds.begin(),
                 std::bind2nd(std::multiplies<double>(), 1e6));
}

/**
* Caches input details for the peak information
*/
void CalculateGammaBackground::retrieveInputs() {
  m_inputWS = getProperty("InputWorkspace");
  m_profileFunction = getPropertyValue("ComptonFunction");
  if (m_profileFunction.find(";") == std::string::npos) // not composite
  {
    m_profileFunction = "composite=CompositeFunction;" + m_profileFunction;
  }

  IFunction_sptr profileFunction =
      FunctionFactory::Instance().createInitialized(m_profileFunction);
  if (auto composite =
          boost::dynamic_pointer_cast<CompositeFunction>(profileFunction)) {
    m_npeaks = composite->nFunctions();
    for (size_t i = 0; i < m_npeaks; ++i) {
      auto single = boost::dynamic_pointer_cast<ComptonProfile>(
          composite->getFunction(i));
      if (!single) {
        throw std::invalid_argument("Invalid function. Composite must contain "
                                    "only ComptonProfile functions");
      }
    }
  } else {
    throw std::invalid_argument(
        "Invalid function found. Expected ComptonFunction to contain a "
        "composite of ComptonProfiles or a single ComptonProfile.");
  }

  // Spectrum numbers whose calculation of background from foils is reversed
  m_reversed.clear();
  for (specid_t i = 143; i < 199; ++i) {
    if ((i >= 143 && i <= 150) || (i >= 159 && i <= 166) ||
        (i >= 175 && i <= 182) || (i >= 191 && i <= 198))
      m_reversed.insert(i);
  }

  // Workspace indices mapping input->output
  m_indices.clear();
  std::vector<int> requestedIndices = getProperty("WorkspaceIndexList");
  if (requestedIndices.empty()) {
    for (size_t i = 0; i < m_inputWS->getNumberHistograms(); ++i) {
      m_indices.insert(std::make_pair(i, i)); // 1-to-1
    }
  } else {
    for (size_t i = 0; i < requestedIndices.size(); ++i) {
      m_indices.insert(std::make_pair(
          i, static_cast<size_t>(
                 requestedIndices[i]))); // user-requested->increasing on output
    }
  }

  cacheInstrumentGeometry();
}

/**
* Create & cache output workspaces
*/
void CalculateGammaBackground::createOutputWorkspaces() {
  const size_t nhist = m_indices.size();
  m_backgroundWS = WorkspaceFactory::Instance().create(m_inputWS, nhist);
  m_correctedWS = WorkspaceFactory::Instance().create(m_inputWS, nhist);
}

/**
*/
void CalculateGammaBackground::cacheInstrumentGeometry() {
  auto inst = m_inputWS->getInstrument();
  auto refFrame = inst->getReferenceFrame();
  auto upAxis = refFrame->pointingUp();
  auto source = inst->getSource();
  auto sample = inst->getSample();
  m_samplePos = sample->getPos();
  m_l1 = m_samplePos.distance(source->getPos());

  // foils
  auto changer = boost::dynamic_pointer_cast<const Geometry::IObjComponent>(
      inst->getComponentByName("foil-changer"));
  if (!changer) {
    throw std::invalid_argument(
        "Input workspace has no component named foil-changer. "
        "One is required to define integration area.");
  }

  // 'height' of box sets limits in beam direction
  Geometry::BoundingBox boundBox;
  changer->getBoundingBox(boundBox);
  m_foilUpMin = boundBox.minPoint()[upAxis];
  m_foilUpMax = boundBox.maxPoint()[upAxis];

  // foil geometry
  // there should be the same number in each position
  const auto &pmap = m_inputWS->constInstrumentParameters();
  auto foils0 = inst->getAllComponentsWithName("foil-pos0");
  auto foils1 = inst->getAllComponentsWithName("foil-pos1");
  const size_t nfoils = foils0.size();
  if (nfoils != foils1.size()) {
    std::ostringstream os;
    os << "Mismatch in number of foils between pos 0 & 1: pos0=" << nfoils
       << ", pos1=" << foils1.size();
    throw std::runtime_error(os.str());
  }
  // It is assumed that the foils all lie on a circle of the same radius from
  // the sample position
  auto firstFoilPos = foils0[0]->getPos();
  double dummy(0.0);
  firstFoilPos.getSpherical(m_foilRadius, dummy, dummy);

  // Cache min/max theta values
  m_foils0.resize(nfoils);
  m_foils1.resize(nfoils);
  for (size_t i = 0; i < nfoils; ++i) {
    const auto &foil0 = foils0[i];
    auto thetaRng0 = calculateThetaRange(foil0, m_foilRadius,
                                         refFrame->pointingHorizontal());
    FoilInfo descr;
    descr.thetaMin = thetaRng0.first;
    descr.thetaMax = thetaRng0.second;
    descr.lorentzWidth =
        ConvertToYSpace::getComponentParameter(foil0, pmap, "hwhm_lorentz");
    descr.gaussWidth =
        ConvertToYSpace::getComponentParameter(foil0, pmap, "sigma_gauss");
    m_foils0[i] = descr; // copy

    const auto &foil1 = foils1[i];
    auto thetaRng1 = calculateThetaRange(foil1, m_foilRadius,
                                         refFrame->pointingHorizontal());
    descr.thetaMin = thetaRng1.first;
    descr.thetaMax = thetaRng1.second;
    descr.lorentzWidth =
        ConvertToYSpace::getComponentParameter(foil1, pmap, "hwhm_lorentz");
    descr.gaussWidth =
        ConvertToYSpace::getComponentParameter(foil1, pmap, "sigma_gauss");
    m_foils1[i] = descr; // copy
  }

  if (g_log.is(Kernel::Logger::Priority::PRIO_INFORMATION)) {
    std::ostringstream os;
    os << "Instrument geometry:\n"
       << "  l1 = " << m_l1 << "m\n"
       << "  foil radius = " << m_foilRadius << "\n"
       << "  foil integration min = " << m_foilUpMin << "\n"
       << "  foil integration max = " << m_foilUpMax << "\n";
    std::ostringstream secondos;
    for (size_t i = 0; i < nfoils; ++i) {
      const auto &descr0 = m_foils0[i];
      os << "  foil theta range in position 0: theta_min=" << descr0.thetaMin
         << ", theta_max=" << descr0.thetaMax << "\n";
      const auto &descr1 = m_foils1[i];
      secondos << "  foil theta range in position 1: theta_min="
               << descr1.thetaMin << ", theta_max=" << descr1.thetaMax << "\n";
    }
    g_log.information() << os.str() << secondos.str();
  }
}

/**
* @param foilComp A pointer to the foil component
* @param radius The radius that gives the distance to the centre of the bounding
* box
* @param horizDir An enumeration defining which direction is horizontal
* @return The min/max angle in theta(degrees) (horizontal direction if you
* assume mid-point theta = 0)
*/
std::pair<double, double> CalculateGammaBackground::calculateThetaRange(
    const Geometry::IComponent_const_sptr &foilComp, const double radius,
    const unsigned int horizDir) const {
  auto shapedObject =
      boost::dynamic_pointer_cast<const Geometry::IObjComponent>(foilComp);
  if (!shapedObject) {
    throw std::invalid_argument("A foil has been defined without a shape. "
                                "Please check instrument definition.");
  }

  // First get current theta position
  auto pos = foilComp->getPos();
  double theta(0.0), dummy(0.0);
  pos.getSpherical(dummy, theta, dummy); // absolute angle values
  if (pos[horizDir] < 0.0)
    theta *= -1.0; // negative quadrant for theta

  // Compute dtheta from bounding box & radius
  const auto &box = shapedObject->shape()->getBoundingBox();
  // box has center at 0,0,0
  double xmax = box.maxPoint()[0];
  double dtheta = std::asin(xmax / radius) * 180.0 / M_PI; // degrees
  return std::make_pair(theta - dtheta, theta + dtheta);
}

} // namespace CurveFitting
} // namespace Mantid
