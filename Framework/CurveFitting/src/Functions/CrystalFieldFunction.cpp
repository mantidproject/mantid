#include "MantidCurveFitting/Functions/CrystalFieldFunction.h"
#include "MantidCurveFitting/Functions/CrystalElectricField.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeaks.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeakUtils.h"
#include "MantidCurveFitting/Functions/CrystalFieldHeatCapacity.h"
#include "MantidCurveFitting/Functions/CrystalFieldSusceptibility.h"
#include "MantidCurveFitting/Functions/CrystalFieldMagnetisation.h"
#include "MantidCurveFitting/Functions/CrystalFieldMoment.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/ParameterTie.h"

#include "MantidKernel/Exception.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/StringTokenizer.h"
#include <iostream>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;
using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(CrystalFieldFunction)

namespace {

/// Define the source function for CrystalFieldFunction.
/// Its function() method is not needed.
class Peaks : public CrystalFieldPeaksBase, public API::IFunctionGeneral {
public:
  Peaks() : CrystalFieldPeaksBase() {}
  std::string name() const override { return "Peaks"; }
  size_t getNumberDomainColumns() const override {
    throw Exception::NotImplementedError(
        "This method is intentionally not implemented.");
  }
  size_t getNumberValuesPerArgument() const override {
    throw Exception::NotImplementedError(
        "This method is intentionally not implemented.");
  }
  void functionGeneral(const API::FunctionDomainGeneral &,
                       API::FunctionValues &) const override {
    throw Exception::NotImplementedError(
        "This method is intentionally not implemented.");
  }
  std::vector<size_t> m_IntensityScalingIdx;
  std::vector<size_t> m_PPLambdaIdxChild;
  std::vector<size_t> m_PPLambdaIdxSelf;
  /// Declare the intensity scaling parameters: one per spectrum.
  void declareIntensityScaling(size_t nSpec) {
    m_IntensityScalingIdx.clear();
    m_PPLambdaIdxChild.resize(nSpec, -1);
    m_PPLambdaIdxSelf.resize(nSpec, -1);
    for (size_t i = 0; i < nSpec; ++i) {
      auto si = std::to_string(i);
      try { // If parameter has already been declared, don't declare it.
        declareParameter("IntensityScaling" + si, 1.0,
                         "Intensity scaling factor for spectrum " + si);
      } catch (std::invalid_argument &) {
      }
      m_IntensityScalingIdx.push_back(parameterIndex("IntensityScaling" + si));
    }
  }
};
}

/// Constructor
CrystalFieldFunction::CrystalFieldFunction()
    : FunctionGenerator(IFunction_sptr()) {
  declareAttribute("Ions", Attribute(""));
  declareAttribute("Symmetries", Attribute(""));
  declareAttribute("Temperatures", Attribute(std::vector<double>()));
  //declareAttribute("FWHMX0", Attribute(std::vector<double>()));
  //declareAttribute("FWHMY0", Attribute(std::vector<double>()));
  //declareAttribute("FWHMVariation", Attribute(0.1));
  //declareAttribute("PhysicalProperties",
  //                 Attribute(std::vector<double>(1, 0.0)));
}

size_t CrystalFieldFunction::getNumberDomains() const {
  if (!m_target) {
    buildTargetFunction();
  }
  if (!m_target) {
    throw std::runtime_error("Failed to build target function.");
  }
  return m_target->getNumberDomains();
}

std::vector<IFunction_sptr>
CrystalFieldFunction::createEquivalentFunctions() const {
  checkTargetFunction();
  std::vector<IFunction_sptr> funs;
  auto &composite = dynamic_cast<CompositeFunction &>(*m_target);
  for (size_t i = 0; i < composite.nFunctions(); ++i) {
    funs.push_back(composite.getFunction(i));
  }
  return funs;
}

/// Returns the number of attributes associated with the function
size_t CrystalFieldFunction::nAttributes() const {
  //TODO: uncomment
  //checkTargetFunction();
  return IFunction::nAttributes() + m_source->nAttributes() +
         m_target->nAttributes();
}

/// Returns a list of attribute names
std::vector<std::string> CrystalFieldFunction::getAttributeNames() const {
  //TODO: uncomment
  //checkTargetFunction();
  std::vector<std::string> attNames = IFunction::getAttributeNames();
  return attNames;
}

/// Return a value of attribute attName
IFunction::Attribute
CrystalFieldFunction::getAttribute(const std::string &attName) const {
  return FunctionGenerator::getAttribute(attName);

  //if (IFunction::hasAttribute(attName)) {
  //  return IFunction::getAttribute(attName);
  //} else if (isSourceName(attName)) {
  //  return m_source->getAttribute(attName);
  //} else {
  //  checkTargetFunction();
  //  return m_target->getAttribute(attName);
  //}
}

/// Check if attribute attName exists
bool CrystalFieldFunction::hasAttribute(const std::string &attName) const {
  if (IFunction::hasAttribute(attName)) {
    return true;
  }
  if (isSourceName(attName)) {
    return m_source->hasAttribute(attName);
  } else {
    checkTargetFunction();
    return m_target->hasAttribute(attName);
  }
}


/// Perform custom actions on setting certain attributes.
void CrystalFieldFunction::setAttribute(const std::string &name,
                                             const Attribute &attr) {
  if (name == "Ions") {
    setIonsAttribute(name, attr);
  } else if (name == "Symmetries") {
    setSymmetriesAttribute(name, attr);
  } else if (name == "Temperatures") {
    setTemperaturesAttribute(name, attr);
  } else {
    FunctionGenerator::setAttribute(name, attr);
  }
}

void CrystalFieldFunction::setIonsAttribute(const std::string &name, const Attribute &attr) {
  Kernel::StringTokenizer tokenizer(attr.asString(), ",", Kernel::StringTokenizer::TOK_TRIM);
  m_ions.clear();
  m_ions.insert(m_ions.end(), tokenizer.begin(), tokenizer.end());
  auto attrValue = Kernel::Strings::join(m_ions.begin(), m_ions.end(), ",");
  FunctionGenerator::storeAttributeValue(name, Attribute(attrValue));
}

void CrystalFieldFunction::setSymmetriesAttribute(const std::string &name, const Attribute &attr) {
  Kernel::StringTokenizer tokenizer(attr.asString(), ",", Kernel::StringTokenizer::TOK_TRIM);
  m_symmetries.clear();
  m_symmetries.insert(m_symmetries.end(), tokenizer.begin(), tokenizer.end());
  auto attrValue = Kernel::Strings::join(m_symmetries.begin(), m_symmetries.end(), ",");
  FunctionGenerator::storeAttributeValue(name, Attribute(attrValue));
}

void CrystalFieldFunction::setTemperaturesAttribute(const std::string &name, const Attribute &attr) {
  m_temperatures = attr.asVector();
  FunctionGenerator::storeAttributeValue(name, attr);
  declareAttribute("Background", Attribute("", true));
  declareAttribute("PeakShape", Attribute("Lorentzian"));
  declareAttribute("FWHMs", Attribute(std::vector<double>()));
  declareAttribute("FWHMVariation", Attribute(0.1));
  if (m_temperatures.size() == 1) {
    declareAttribute("FWHMX", Attribute(std::vector<double>()));
    declareAttribute("FWHMY", Attribute(std::vector<double>()));
  }
  declareAttribute("NPeaks", Attribute(0));
  declareAttribute("FixAllPeaks", Attribute(false));

  // Define (declare) the parameters for intensity scaling.
  //const auto nSpec = attr.asVector().size();
  //dynamic_cast<Peaks &>(*m_source).declareIntensityScaling(nSpec);
  //m_nOwnParams = m_source->nParams();
  //m_fwhmX.resize(nSpec);
  //m_fwhmY.resize(nSpec);
  //for (size_t iSpec = 0; iSpec < nSpec; ++iSpec) {
  //  const auto suffix = std::to_string(iSpec);
  //  declareAttribute("FWHMX" + suffix, Attribute(m_fwhmX[iSpec]));
  //  declareAttribute("FWHMY" + suffix, Attribute(m_fwhmY[iSpec]));
  //}
}

/// Check if the function is set up for a multi-site calculations.
/// (Multiple ions defined)
bool CrystalFieldFunction::isMultiSite() const {
  return m_ions.size() > 1;
}

/// Check if the function is set up for a multi-spectrum calculations
/// (Multiple temperatures defined)
bool CrystalFieldFunction::isMultiSpectrum() const {
  return m_temperatures.size() > 1;
}

/// Check if the spectra have a background.
bool CrystalFieldFunction::hasBackground() const {
  if (!hasAttribute("Background")) {
    return false;
  }
  auto background = getAttribute("Background").asString();
  return background != "\"\"";
}

/// Check if there are peaks (there is at least one spectrum).
bool CrystalFieldFunction::hasPeaks() const {
  return !m_temperatures.empty();
}

/// Check if there are any phys. properties.
bool CrystalFieldFunction::hasPhysProperties() const {
  return false;
}

void CrystalFieldFunction::chacheAttributes() const {
  if (hasAttribute("FWHMs")) {
    m_FWHMs = getAttribute("FWHMs").asVector();
  }
  if (hasAttribute("FWHMX")) {
    auto fwhmX = getAttribute("FWHMX").asVector();
    auto fwhmY = getAttribute("FWHMY").asVector();
    if (!fwhmX.empty()) {
      m_fwhmX.clear();
      m_fwhmX.push_back(fwhmX);
    }
    if (!fwhmY.empty()) {
      m_fwhmY.clear();
      m_fwhmY.push_back(fwhmY);
    }
  }
}

/// Check that attributes and parameters are consistent.
/// If not excepion is thrown.
void CrystalFieldFunction::checkConsistent() const {
  if (m_ions.empty()) {
    throw std::runtime_error("No ions are set.");
  }
  if (m_ions.size() != m_symmetries.size()) {
    throw std::runtime_error(
        "Number of ions is different from number of symmetries.");
  }
  chacheAttributes();
  if (!m_temperatures.empty()) {
    const auto nSpec = m_temperatures.size();
    if (m_FWHMs.empty()) {
      if (m_fwhmX.empty() || m_fwhmY.empty()) {
        throw std::runtime_error("No peak width settings (FWHMs and FWHMX and FWHMY attributes not set).");
      }
      m_FWHMs.resize(nSpec, 0.0);
    } else if (m_FWHMs.size() != nSpec) {
      if (m_FWHMs.size() == 1) {
        auto fwhm = m_FWHMs.front();
        m_FWHMs.resize(nSpec, fwhm);
      } else {
        throw std::runtime_error(
            "Vector of FWHMs must either have same size as "
            "Temperatures (" +
            std::to_string(nSpec) + ") or have size 1.");
      }
    }
  }
}

/// Uses source to calculate peak centres and intensities
/// then populates m_spectrum with peaks of type given in PeakShape attribute.
void CrystalFieldFunction::buildTargetFunction() const {
  checkConsistent();
  m_dirty = false;
  if (isMultiSite()) {
    buildMultiSite();
  } else {
    buildSingleSite();
  }
}

/// Build the target function in a single site case.
void CrystalFieldFunction::buildSingleSite() const {
  if (isMultiSpectrum()) {
    buildSingleSiteMultiSpectrum();
  } else {
    buildSingleSiteSingleSpectrum();
  }
}

/// Build the target function in a multi site case.
void CrystalFieldFunction::buildMultiSite() const {
  if (isMultiSpectrum()) {
    buildMultiSiteMultiSpectrum();
  } else {
    buildMultiSiteSingleSpectrum();
  }
}

/// Build the target function in a single site - single spectrum case.
void CrystalFieldFunction::buildSingleSiteSingleSpectrum() const {
  auto spectrum = new CompositeFunction;
  m_target.reset(spectrum);
  m_target->setAttribute("NumDeriv", this->getAttribute("NumDeriv"));

  FunctionDomainGeneral domain;
  FunctionValues values;
  m_source->function(domain, values);

  if (values.size() == 0) {
    return;
  }

  if (values.size() % 2 != 0) {
    throw std::runtime_error(
        "CrystalFieldPeaks returned odd number of values.");
  }

  auto xVec = getAttribute("FWHMX").asVector();
  auto yVec = getAttribute("FWHMY").asVector();
  auto fwhmVariation = getAttribute("FWHMVariation").asDouble();

  auto peakShape = getAttribute("PeakShape").asString();
  auto defaultFWHM = getAttribute("FWHM").asDouble();
  size_t nRequiredPeaks = getAttribute("NPeaks").asInt();
  bool fixAllPeaks = getAttribute("FixAllPeaks").asBool();
  m_nPeaks[0] = CrystalFieldUtils::buildSpectrumFunction(
      *spectrum, peakShape, values, xVec, yVec, fwhmVariation, defaultFWHM,
      nRequiredPeaks, fixAllPeaks);
  //storeReadOnlyAttribute("NPeaks", Attribute(static_cast<int>(m_nPeaks)));
}

/// Build the target function in a single site - multi spectrum case.
void CrystalFieldFunction::buildSingleSiteMultiSpectrum() const {
  setSource(IFunction_sptr(new Peaks));
  auto fun = new MultiDomainFunction;
  m_target.reset(fun);

  DoubleFortranVector en;
  ComplexFortranMatrix wf;
  ComplexFortranMatrix ham;
  ComplexFortranMatrix hz;
  int nre = 0;
  auto &peakCalculator = dynamic_cast<Peaks &>(*m_source);
  peakCalculator.calculateEigenSystem(en, wf, ham, hz, nre);
  ham += hz;

  const auto nSpec = m_temperatures.size();
  // Get a list of "spectra" which corresponds to physical properties
  //const auto physprops = getAttribute("PhysicalProperties").asVector();
  //if (physprops.empty()) {
  //  m_physprops.resize(nSpec, 0); // Assume no physical properties - just INS
  //} else if (physprops.size() != nSpec) {
  //  if (physprops.size() == 1) {
  //    int physprop = static_cast<int>(physprops.front());
  //    m_physprops.resize(nSpec, physprop);
  //  } else {
  //    throw std::runtime_error("Vector of PhysicalProperties must have same "
  //                             "size as Temperatures or size 1.");
  //  }
  //} else {
  //  m_physprops.clear();
  //  for (auto elem : physprops) {
  //    m_physprops.push_back(static_cast<int>(elem));
  //  }
  //}
  // Create the single-spectrum functions.
  m_nPeaks.resize(nSpec);
  if (m_fwhmX.empty()) {
    m_fwhmX.resize(nSpec);
    m_fwhmY.resize(nSpec);
  }
  for (size_t i = 0; i < nSpec; ++i) {
    if (m_fwhmX[i].empty()) {
      //auto suffix = std::to_string(i);
      //m_fwhmX[i] = IFunction::getAttribute("FWHMX" + suffix).asVector();
      //m_fwhmY[i] = IFunction::getAttribute("FWHMY" + suffix).asVector();
    }
    fun->addFunction(
        buildSpectrum(nre, en, wf, m_temperatures[i], m_FWHMs[i], i));
    fun->setDomainIndex(i, i);
  }
}

/// Build the target function in a multi site - single spectrum case.
void CrystalFieldFunction::buildMultiSiteSingleSpectrum() const {
  throw std::runtime_error("buildMultiSiteSingleSpectrum() not implemented yet.");
}

/// Build the target function in a multi site - multi spectrum case.
void CrystalFieldFunction::buildMultiSiteMultiSpectrum() const {
  throw std::runtime_error("buildMultiSiteMultiSpectrum() not implemented yet.");
}

/// Calculate excitations at given temperature
void CrystalFieldFunction::calcExcitations(
    int nre, const DoubleFortranVector &en, const ComplexFortranMatrix &wf,
    double temperature, FunctionValues &values, size_t iSpec) const {
  IntFortranVector degeneration;
  DoubleFortranVector eEnergies;
  DoubleFortranMatrix iEnergies;
  const double de = getAttribute("ToleranceEnergy").asDouble();
  const double di = getAttribute("ToleranceIntensity").asDouble();
  DoubleFortranVector eExcitations;
  DoubleFortranVector iExcitations;
  calculateIntensities(nre, en, wf, temperature, de, degeneration, eEnergies,
                       iEnergies);
  calculateExcitations(eEnergies, iEnergies, de, di, eExcitations,
                       iExcitations);
  const size_t nSpec = m_nPeaks.size();
  // Get intensity scaling parameter "IntensityScaling" + std::to_string(iSpec)
  // using an index instead of a name for performance reasons
  auto &source = dynamic_cast<Peaks &>(*m_source);
  double intensityScaling;
  if (source.m_IntensityScalingIdx.size() == 0) {
    intensityScaling = getParameter(m_nOwnParams - nSpec + iSpec);
  } else {
    intensityScaling = getParameter(source.m_IntensityScalingIdx[iSpec]);
  }
  const auto nPeaks = eExcitations.size();
  values.expand(2 * nPeaks);
  for (size_t i = 0; i < nPeaks; ++i) {
    values.setCalculated(i, eExcitations.get(i));
    values.setCalculated(i + nPeaks, iExcitations.get(i) * intensityScaling);
  }
}

/// Build a function for a single spectrum.
API::IFunction_sptr CrystalFieldFunction::buildSpectrum(
    int nre, const DoubleFortranVector &en, const ComplexFortranMatrix &wf,
    double temperature, double fwhm, size_t iSpec) const {
  FunctionValues values;
  calcExcitations(nre, en, wf, temperature, values, iSpec);
  m_nPeaks[iSpec] = CrystalFieldUtils::calculateNPeaks(values);

  const auto fwhmVariation = getAttribute("FWHMVariation").asDouble();
  const auto peakShape = IFunction::getAttribute("PeakShape").asString();
  auto bkgdShape = IFunction::getAttribute("Background").asUnquotedString();
  const size_t nRequiredPeaks = IFunction::getAttribute("NPeaks").asInt();
  const bool fixAllPeaks = getAttribute("FixAllPeaks").asBool();

  if (!bkgdShape.empty() && bkgdShape.find("name=") != 0 &&
      bkgdShape.front() != '(') {
    bkgdShape = "name=" + bkgdShape;
  }

  auto spectrum = new CompositeFunction;

  if (!bkgdShape.empty()) {
    auto background =
        API::FunctionFactory::Instance().createInitialized(bkgdShape);
    spectrum->addFunction(background);
    if (fixAllPeaks) {
      background->fixAll();
    }
  }

  m_nPeaks[iSpec] = CrystalFieldUtils::buildSpectrumFunction(
      *spectrum, peakShape, values, m_fwhmX[iSpec], m_fwhmY[iSpec],
      fwhmVariation, fwhm, nRequiredPeaks, fixAllPeaks);
  return IFunction_sptr(spectrum);
}

/// Update m_spectrum function.
void CrystalFieldFunction::updateTargetFunction() const {
  if (!m_target) {
    buildTargetFunction();
    return;
  }
  m_dirty = false;

  DoubleFortranVector en;
  ComplexFortranMatrix wf;
  ComplexFortranMatrix ham;
  ComplexFortranMatrix hz;
  int nre = 0;
  auto &peakCalculator = dynamic_cast<Peaks &>(*m_source);
  peakCalculator.calculateEigenSystem(en, wf, ham, hz, nre);
  ham += hz;

  auto &fun = dynamic_cast<MultiDomainFunction &>(*m_target);
  try {
    for (size_t i = 0; i < m_temperatures.size(); ++i) {
      updateSpectrum(*fun.getFunction(i), nre, en, wf, ham, m_temperatures[i],
                     m_FWHMs[i], i);
    }
  } catch (std::out_of_range &) {
    buildTargetFunction();
    return;
  }
}

/// Update a function for a single spectrum.
void CrystalFieldFunction::updateSpectrum(API::IFunction &spectrum, int nre,
                                          const DoubleFortranVector &en,
                                          const ComplexFortranMatrix &wf,
                                          const ComplexFortranMatrix &ham,
                                          double temperature, double fwhm,
                                          size_t iSpec) const {
  const auto fwhmVariation = getAttribute("FWHMVariation").asDouble();
  const auto peakShape = IFunction::getAttribute("PeakShape").asString();
  const bool fixAllPeaks = getAttribute("FixAllPeaks").asBool();
  FunctionValues values;
  calcExcitations(nre, en, wf, temperature, values, iSpec);
  auto &composite = dynamic_cast<API::CompositeFunction &>(spectrum);
  m_nPeaks[iSpec] = CrystalFieldUtils::updateSpectrumFunction(
      composite, peakShape, values, 1, m_fwhmX[iSpec], m_fwhmY[iSpec],
      fwhmVariation, fwhm, fixAllPeaks);
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
