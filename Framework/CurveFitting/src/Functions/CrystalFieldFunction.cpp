#include "MantidCurveFitting/Functions/CrystalFieldFunction.h"
#include "MantidCurveFitting/Functions/CrystalElectricField.h"
#include "MantidCurveFitting/Functions/CrystalFieldHeatCapacity.h"
#include "MantidCurveFitting/Functions/CrystalFieldMagnetisation.h"
#include "MantidCurveFitting/Functions/CrystalFieldMoment.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeakUtils.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeaks.h"
#include "MantidCurveFitting/Functions/CrystalFieldSusceptibility.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/ParameterTie.h"

#include "MantidKernel/Exception.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/Strings.h"
#include <iostream>
#include <regex>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;
using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(CrystalFieldFunction)

namespace {

// Regex for names of attributes/parameters for a particular spectrum
// Example: sp1.FWHMX
const std::regex SPECTRUM_ATTR_REGEX("sp([0-9]+)\\.(.+)");


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
} // namespace

/// Constructor
CrystalFieldFunction::CrystalFieldFunction() : IFunction() {
  declareAttribute("Ions", Attribute(""));
  declareAttribute("Symmetries", Attribute(""));
  declareAttribute("Temperatures", Attribute(std::vector<double>()));
  declareAttribute("ToleranceEnergy", Attribute(1.0e-10));
  declareAttribute("ToleranceIntensity", Attribute(1.0e-1));
  // declareAttribute("PhysicalProperties",
  //                 Attribute(std::vector<double>(1, 0.0)));
}

// Evaluates the function
void CrystalFieldFunction::function(const FunctionDomain &domain,
                                    FunctionValues &values) const {
  updateTargetFunction();
  if (!m_target) {
    throw std::logic_error(
        "FunctionGenerator failed to generate target function.");
  }
  m_target->function(domain, values);
}

/// Set the source function
/// @param source :: New source function.
void CrystalFieldFunction::setSource(IFunction_sptr source) const {
  m_source = source;
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

/// Set i-th parameter
void CrystalFieldFunction::setParameter(size_t i, const double &value,
                                        bool explicitlySet) {
  if (i < m_nOwnParams) {
    m_source->setParameter(i, value, explicitlySet);
    m_dirty = true;
  } else {
    checkTargetFunction();
    m_target->setParameter(i - m_nOwnParams, value, explicitlySet);
  }
}

/// Set i-th parameter description
void CrystalFieldFunction::setParameterDescription(
    size_t i, const std::string &description) {
  if (i < m_nOwnParams) {
    m_source->setParameterDescription(i, description);
  } else {
    checkTargetFunction();
    m_target->setParameterDescription(i - m_nOwnParams, description);
  }
}

/// Get i-th parameter
double CrystalFieldFunction::getParameter(size_t i) const {
  checkTargetFunction();
  return i < m_nOwnParams ? m_source->getParameter(i)
                          : m_target->getParameter(i - m_nOwnParams);
}

/// Set parameter by name.
void CrystalFieldFunction::setParameter(const std::string &name,
                                        const double &value,
                                        bool explicitlySet) {
  checkTargetFunction();
  auto i = parameterIndex(name);
  setParameter(i, value, explicitlySet);
}

/// Set description of parameter by name.
void CrystalFieldFunction::setParameterDescription(
    const std::string &name, const std::string &description) {
  checkTargetFunction();
  auto i = parameterIndex(name);
  setParameterDescription(i, description);
}

/// Get parameter by name.
double CrystalFieldFunction::getParameter(const std::string &name) const {
  auto i = parameterIndex(name);
  return getParameter(i);
}

/// Total number of parameters
size_t CrystalFieldFunction::nParams() const {
  checkTargetFunction();
  return m_source->nParams() + m_target->nParams();
}

/// Returns the index of parameter name
size_t CrystalFieldFunction::parameterIndex(const std::string &name) const {
  if (isSourceName(name)) {
    return m_source->parameterIndex(name);
  } else {
    checkTargetFunction();
    return m_target->parameterIndex(name) + m_nOwnParams;
  }
}

/// Returns the name of parameter i
std::string CrystalFieldFunction::parameterName(size_t i) const {
  checkTargetFunction();
  return i < m_nOwnParams ? m_source->parameterName(i)
                          : m_target->parameterName(i - m_nOwnParams);
}

/// Returns the description of parameter i
std::string CrystalFieldFunction::parameterDescription(size_t i) const {
  checkTargetFunction();
  return i < m_nOwnParams ? m_source->parameterDescription(i)
                          : m_target->parameterDescription(i - m_nOwnParams);
}

/// Checks if a parameter has been set explicitly
bool CrystalFieldFunction::isExplicitlySet(size_t i) const {
  checkTargetFunction();
  return i < m_nOwnParams ? m_source->isExplicitlySet(i)
                          : m_target->isExplicitlySet(i - m_nOwnParams);
}

/// Get the fitting error for a parameter
double CrystalFieldFunction::getError(size_t i) const {
  checkTargetFunction();
  return i < m_nOwnParams ? m_source->getError(i)
                          : m_target->getError(i - m_nOwnParams);
}

/// Set the fitting error for a parameter
void CrystalFieldFunction::setError(size_t i, double err) {
  if (i < m_nOwnParams) {
    m_source->setError(i, err);
  } else {
    checkTargetFunction();
    m_target->setError(i - m_nOwnParams, err);
  }
}

/// Change status of parameter
void CrystalFieldFunction::setParameterStatus(
    size_t i, IFunction::ParameterStatus status) {
  if (i < m_nOwnParams) {
    m_source->setParameterStatus(i, status);
  } else {
    checkTargetFunction();
    m_target->setParameterStatus(i - m_nOwnParams, status);
  }
}

/// Get status of parameter
IFunction::ParameterStatus
CrystalFieldFunction::getParameterStatus(size_t i) const {
  if (i < m_nOwnParams) {
    return m_source->getParameterStatus(i);
  } else {
    checkTargetFunction();
    return m_target->getParameterStatus(i - m_nOwnParams);
  }
}

/// Return parameter index from a parameter reference.
size_t
CrystalFieldFunction::getParameterIndex(const ParameterReference &ref) const {
  if (ref.getLocalFunction() == this) {
    auto index = ref.getLocalIndex();
    auto np = nParams();
    if (index < np) {
      return index;
    }
    return np;
  }
  checkTargetFunction();
  return m_target->getParameterIndex(ref) + m_nOwnParams;
}

/// Set up the function for a fit.
void CrystalFieldFunction::setUpForFit() {
  updateTargetFunction();
  IFunction::setUpForFit();
}

/// Declare a new parameter
void CrystalFieldFunction::declareParameter(const std::string &, double,
                                            const std::string &) {
  throw Kernel::Exception::NotImplementedError(
      "CrystalFieldFunction cannot not have its own parameters.");
}

/// Returns the number of attributes associated with the function
size_t CrystalFieldFunction::nAttributes() const {
  // TODO: uncomment
  // checkTargetFunction();
  return IFunction::nAttributes() + m_source->nAttributes() +
         m_target->nAttributes();
}

/// Returns a list of attribute names
std::vector<std::string> CrystalFieldFunction::getAttributeNames() const {
  // TODO: uncomment
  // checkTargetFunction();
  std::vector<std::string> attNames = IFunction::getAttributeNames();
  return attNames;
}

/// Return a value of attribute attName
/// @param attName :: Name of an attribute.
IFunction::Attribute
CrystalFieldFunction::getAttribute(const std::string &attName) const {
  std::smatch match;
  if (std::regex_match(attName, match, SPECTRUM_ATTR_REGEX)) {
    auto i = std::stoul(match[1]);
    return getSpectrumAttribute(i, match[2]);
  }
  return IFunction::getAttribute(attName);

  // if (IFunction::hasAttribute(attName)) {
  //  return IFunction::getAttribute(attName);
  //} else if (isSourceName(attName)) {
  //  return m_source->getAttribute(attName);
  //} else {
  //  checkTargetFunction();
  //  return m_target->getAttribute(attName);
  //}
}

/// Perform custom actions on setting certain attributes.
void CrystalFieldFunction::setAttribute(const std::string &attName,
                                        const Attribute &attr) {
  if (attName == "Ions") {
    setIonsAttribute(attName, attr);
  } else if (attName == "Symmetries") {
    setSymmetriesAttribute(attName, attr);
  } else if (attName == "Temperatures") {
    setTemperaturesAttribute(attName, attr);
  } else {
    std::smatch match;
    if (std::regex_match(attName, match, SPECTRUM_ATTR_REGEX)) {
      auto i = std::stoul(match[1]);
      setSpectrumAttribute(i, match[2], attr);
    }
    API::IFunction::setAttribute(attName, attr);
  }
}

/// Check if attribute attName exists
bool CrystalFieldFunction::hasAttribute(const std::string &attName) const {
  std::smatch match;
  if (std::regex_match(attName, match, SPECTRUM_ATTR_REGEX)) {
    auto i = std::stoul(match[1]);
    return hasSpectrumAttribute(i, match[2]);
  }

  if (IFunction::hasAttribute(attName)) {
    return true;
  }
  if (!m_source) {
    return false;
  }
  if (isSourceName(attName)) {
    return m_source->hasAttribute(attName);
  } else {
    checkTargetFunction();
    return m_target->hasAttribute(attName);
  }
}

/// Get number of the number of spectra (excluding phys prop data).
size_t CrystalFieldFunction::nSpectra() const {
  return m_temperatures.size();
}

/// Check that a spectrum index is within the range
/// @param iSpec :: Index of a spectrum.
/// @throws if index is outside the range.
void CrystalFieldFunction::checkSpectrumIndex(size_t iSpec) const {
  auto nSpec = nSpectra();
  if (nSpec == 0) {
    throw std::runtime_error("No spectra defined.");
  } else if (nSpec == 1) {
    throw std::runtime_error("Cannot use spectra indices in a single-spectrum case.");
  } else if (iSpec >= nSpec) {
    throw std::out_of_range("Spectrum index (" + std::to_string(iSpec) +
                            ") is out side outside the range (N=" +
                            std::to_string(nSpectra()) + ").");
  }
}

/// Check if there is an attribute specific to a spectrum (multi-spectrum case only).
/// @param iSpec :: Index of a spectrum.
/// @param attName :: Name of an attribute to check.
bool CrystalFieldFunction::hasSpectrumAttribute(size_t iSpec, const std::string &attName) const {
  if (nSpectra() < 2) {
    return false;
  }
  checkSpectrumIndex(iSpec);
  if (attName == "FWHMX" || attName == "FWHMY" || attName == "Temperature") {
    return true;
  }
  return false;
}

/// Get an attribute specific to a spectrum (multi-spectrum case only).
/// @param iSpec :: Index of a spectrum.
/// @param attName :: Name of an attribute.
API::IFunction::Attribute
CrystalFieldFunction::getSpectrumAttribute(size_t iSpec,
                                           const std::string &attName) const {
  checkSpectrumIndex(iSpec);
  if (attName == "FWHMX") {
    if (iSpec < m_fwhmX.size()) {
      return Attribute(m_fwhmX[iSpec]);
    } else {
      return Attribute(std::vector<double>());
    }
  } else if (attName == "FWHMY") {
    if (iSpec < m_fwhmY.size()) {
      return Attribute(m_fwhmY[iSpec]);
    } else {
      return Attribute(std::vector<double>());
    }
  } else if (attName == "Temperature") {
    return Attribute(m_temperatures[iSpec]);
  }
  throw std::runtime_error("Attribute " + attName + " not found.");
}

/// Set a value to a spectrum-specific attribute
/// @param iSpec :: Index of a spectrum.
/// @param attName :: Name of an attribute.
/// @param value :: New value of the attribute.
void CrystalFieldFunction::setSpectrumAttribute(size_t iSpec, const std::string &attName, const Attribute &value) {
  checkSpectrumIndex(iSpec);
  if (attName == "FWHMX") {
    if (iSpec < m_fwhmX.size()) {
      m_fwhmX[iSpec] = value.asVector();
    }
  } else if (attName == "FWHMY") {
    if (iSpec < m_fwhmY.size()) {
      m_fwhmY[iSpec] = value.asVector();
    }
  } else if (attName == "Temperature") {
    m_temperatures[iSpec] = value.asDouble();
    IFunction::storeAttributeValue("Temperatures", Attribute(m_temperatures));
  }
}

/// Get the tie for i-th parameter
ParameterTie *CrystalFieldFunction::getTie(size_t i) const {
  auto tie = IFunction::getTie(i);
  if (!tie) {
    return nullptr;
  }
  if (i < m_nOwnParams) {
    tie = m_source->getTie(i);
  } else {
    checkTargetFunction();
    tie = m_target->getTie(i - m_nOwnParams);
  }
  return tie;
}

/// Get the i-th constraint
IConstraint *CrystalFieldFunction::getConstraint(size_t i) const {
  auto constraint = IFunction::getConstraint(i);
  if (constraint == nullptr) {
    if (i < m_nOwnParams) {
      constraint = m_source->getConstraint(i);
    } else {
      checkTargetFunction();
      constraint = m_target->getConstraint(i - m_nOwnParams);
    }
  }
  return constraint;
}

void CrystalFieldFunction::setIonsAttribute(const std::string &name,
                                            const Attribute &attr) {
  Kernel::StringTokenizer tokenizer(attr.asString(), ",",
                                    Kernel::StringTokenizer::TOK_TRIM);
  m_ions.clear();
  m_ions.insert(m_ions.end(), tokenizer.begin(), tokenizer.end());
  auto attrValue = Kernel::Strings::join(m_ions.begin(), m_ions.end(), ",");
  IFunction::storeAttributeValue(name, Attribute(attrValue));
}

void CrystalFieldFunction::setSymmetriesAttribute(const std::string &name,
                                                  const Attribute &attr) {
  Kernel::StringTokenizer tokenizer(attr.asString(), ",",
                                    Kernel::StringTokenizer::TOK_TRIM);
  m_symmetries.clear();
  m_symmetries.insert(m_symmetries.end(), tokenizer.begin(), tokenizer.end());
  auto attrValue =
      Kernel::Strings::join(m_symmetries.begin(), m_symmetries.end(), ",");
  IFunction::storeAttributeValue(name, Attribute(attrValue));
}

void CrystalFieldFunction::setTemperaturesAttribute(const std::string &name,
                                                    const Attribute &attr) {
  m_temperatures = attr.asVector();
  IFunction::storeAttributeValue(name, attr);
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
  // const auto nSpec = attr.asVector().size();
  // dynamic_cast<Peaks &>(*m_source).declareIntensityScaling(nSpec);
  // m_nOwnParams = m_source->nParams();
  // m_fwhmX.resize(nSpec);
  // m_fwhmY.resize(nSpec);
  // for (size_t iSpec = 0; iSpec < nSpec; ++iSpec) {
  //  const auto suffix = std::to_string(iSpec);
  //  declareAttribute("FWHMX" + suffix, Attribute(m_fwhmX[iSpec]));
  //  declareAttribute("FWHMY" + suffix, Attribute(m_fwhmY[iSpec]));
  //}
}

/// Check if the function is set up for a multi-site calculations.
/// (Multiple ions defined)
bool CrystalFieldFunction::isMultiSite() const { return m_ions.size() > 1; }

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
bool CrystalFieldFunction::hasPeaks() const { return !m_temperatures.empty(); }

/// Check if there are any phys. properties.
bool CrystalFieldFunction::hasPhysProperties() const { return false; }

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

/// Test if a name (parameter's or attribute's) belongs to m_source
/// @param aName :: A name to test.
bool CrystalFieldFunction::isSourceName(const std::string &aName) const {
  if (aName.empty()) {
    throw std::invalid_argument(
        "Parameter or attribute name cannot be empty string.");
  }
  return (aName.front() != 'f' || aName.find('.') == std::string::npos);
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
        throw std::runtime_error("No peak width settings (FWHMs and FWHMX and "
                                 "FWHMY attributes not set).");
      }
      if (m_fwhmX.size() != nSpec || m_fwhmY.size() != nSpec) {
        throw std::runtime_error("There must be as many (FWHMX, FWHMY) pairs of attributes as there are Temperatures.");
      }
      for(size_t i = 0; i < nSpec; ++i) {
        if (m_fwhmX[i].size() != m_fwhmY[i].size()) {
          throw std::runtime_error("Vectors in each pair of (FWHMX, FWHMY) attributes must have the same size");
        }
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
    } else if (!m_fwhmX.empty() || !m_fwhmY.empty()) {
      throw std::runtime_error("Either FWHMs or (FWHMX and FWHMY) can be set but not all.");
    }
  }
}

/// Update spectrum function if necessary.
void CrystalFieldFunction::checkTargetFunction() const {
  if (m_dirty) {
    updateTargetFunction();
  }
  if (!m_target) {
    throw std::logic_error(
        "CrystalFieldFunction failed to generate target function.");
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
  m_source.reset(new CrystalFieldPeaks);
  if (isMultiSpectrum()) {
    buildSingleSiteMultiSpectrum();
  } else {
    buildSingleSiteSingleSpectrum();
  }
}

/// Build the target function in a multi site case.
void CrystalFieldFunction::buildMultiSite() const {
  m_source.reset(new Peaks);
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
  m_target->setAttributeValue("NumDeriv", true);

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

  checkConsistent();
  bool hasWidthModel = !m_fwhmX.empty();
  auto xVec = hasWidthModel ? m_fwhmX[0] : std::vector<double>();
  auto yVec = hasWidthModel ? m_fwhmY[0] : std::vector<double>();
  auto defaultFWHM = m_FWHMs.empty() ? 0.0 : m_FWHMs[0];

  auto fwhmVariation = getAttribute("FWHMVariation").asDouble();
  auto peakShape = getAttribute("PeakShape").asString();
  size_t nRequiredPeaks = getAttribute("NPeaks").asInt();
  bool fixAllPeaks = getAttribute("FixAllPeaks").asBool();
  auto nPeaks = CrystalFieldUtils::buildSpectrumFunction(
      *spectrum, peakShape, values, xVec, yVec, fwhmVariation, defaultFWHM,
      nRequiredPeaks, fixAllPeaks);
  (void)nPeaks;
  // storeReadOnlyAttribute("NPeaks", Attribute(static_cast<int>(m_nPeaks)));
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
  // const auto physprops = getAttribute("PhysicalProperties").asVector();
  // if (physprops.empty()) {
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
  // m_nPeaks.resize(nSpec);
  if (m_fwhmX.empty()) {
    m_fwhmX.resize(nSpec);
    m_fwhmY.resize(nSpec);
  }
  for (size_t i = 0; i < nSpec; ++i) {
    if (m_fwhmX[i].empty()) {
      // auto suffix = std::to_string(i);
      // m_fwhmX[i] = IFunction::getAttribute("FWHMX" + suffix).asVector();
      // m_fwhmY[i] = IFunction::getAttribute("FWHMY" + suffix).asVector();
    }
    fun->addFunction(
        buildSpectrum(nre, en, wf, m_temperatures[i], m_FWHMs[i], i));
    fun->setDomainIndex(i, i);
  }
}

/// Build the target function in a multi site - single spectrum case.
void CrystalFieldFunction::buildMultiSiteSingleSpectrum() const {
  throw std::runtime_error(
      "buildMultiSiteSingleSpectrum() not implemented yet.");
}

/// Build the target function in a multi site - multi spectrum case.
void CrystalFieldFunction::buildMultiSiteMultiSpectrum() const {
  throw std::runtime_error(
      "buildMultiSiteMultiSpectrum() not implemented yet.");
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
  const size_t nSpec = m_temperatures.size();
  // Get intensity scaling parameter "IntensityScaling" + std::to_string(iSpec)
  // using an index instead of a name for performance reasons
  auto &source = dynamic_cast<Peaks &>(*m_source);
  double intensityScaling = 1.0;
  // if (source.m_IntensityScalingIdx.size() == 0) {
  //  intensityScaling = getParameter(m_nOwnParams - nSpec + iSpec);
  //} else {
  //  intensityScaling = getParameter(source.m_IntensityScalingIdx[iSpec]);
  //}
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
  auto nPeaks = CrystalFieldUtils::calculateNPeaks(values);
  (void)nPeaks;
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

  nPeaks = CrystalFieldUtils::buildSpectrumFunction(
      *spectrum, peakShape, values, m_fwhmX[iSpec], m_fwhmY[iSpec],
      fwhmVariation, fwhm, nRequiredPeaks, fixAllPeaks);
  (void)nPeaks;
  return IFunction_sptr(spectrum);
}

/// Update m_spectrum function.
void CrystalFieldFunction::updateTargetFunction() const {
  if (!m_target) {
    buildTargetFunction();
    return;
  }

  m_dirty = false;
  if (isMultiSite()) {
    updateMultiSite();
  } else {
    updateSingleSite();
  }
}

/// Update the target function in a single site case.
void CrystalFieldFunction::updateSingleSite() const {
  if (isMultiSpectrum()) {
    updateSingleSiteMultiSpectrum();
  } else {
    updateSingleSiteSingleSpectrum();
  }
}

/// Update the target function in a multi site case.
void CrystalFieldFunction::updateMultiSite() const {
  if (isMultiSpectrum()) {
    updateMultiSiteMultiSpectrum();
  } else {
    updateMultiSiteSingleSpectrum();
  }
}

/// Update the target function in a single site - single spectrum case.
void CrystalFieldFunction::updateSingleSiteSingleSpectrum() const {
  auto fwhmVariation = getAttribute("FWHMVariation").asDouble();
  auto peakShape = getAttribute("PeakShape").asString();
  bool fixAllPeaks = getAttribute("FixAllPeaks").asBool();
  bool hasWidthModel = !m_fwhmX.empty();
  auto xVec = hasWidthModel ? m_fwhmX[0] : std::vector<double>();
  auto yVec = hasWidthModel ? m_fwhmY[0] : std::vector<double>();
  auto defaultFWHM = m_FWHMs.empty() ? 0.0 : m_FWHMs[0];

  FunctionDomainGeneral domain;
  FunctionValues values;
  m_source->function(domain, values);
  m_target->setAttributeValue("NumDeriv", true);
  auto &spectrum = dynamic_cast<CompositeFunction &>(*m_target);
  auto nPeaks = CrystalFieldUtils::calculateNPeaks(values);
  (void)nPeaks;
  CrystalFieldUtils::updateSpectrumFunction(spectrum, peakShape, values, 0,
                                            xVec, yVec, fwhmVariation,
                                            defaultFWHM, fixAllPeaks);
}

/// Update the target function in a single site - multi spectrum case.
void CrystalFieldFunction::updateSingleSiteMultiSpectrum() const {
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

/// Update the target function in a multi site - single spectrum case.
void CrystalFieldFunction::updateMultiSiteSingleSpectrum() const {}

/// Update the target function in a multi site - multi spectrum case.
void CrystalFieldFunction::updateMultiSiteMultiSpectrum() const {}

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
  auto nPeaks = CrystalFieldUtils::updateSpectrumFunction(
      composite, peakShape, values, 1, m_fwhmX[iSpec], m_fwhmY[iSpec],
      fwhmVariation, fwhm, fixAllPeaks);
  (void)nPeaks;
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
