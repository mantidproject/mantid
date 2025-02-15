// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
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

#include <boost/regex.hpp>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;
using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(CrystalFieldFunction)

namespace {

const std::string ION_PREFIX("ion");
const std::string SPECTRUM_PREFIX("sp");
const std::string BACKGROUND_PREFIX("bg");
const std::string PEAK_PREFIX("pk");

// Regex for names of attributes/parameters for a particular spectrum
// Example: sp1.FWHMX
const boost::regex SPECTRUM_ATTR_REGEX(SPECTRUM_PREFIX + "([0-9]+)\\.(.+)");
// Regex for names of attributes/parameters for a background
// Example: bg.A1
const boost::regex BACKGROUND_ATTR_REGEX(BACKGROUND_PREFIX + "\\.(.+)");
// Regex for names of attributes/parameters for peaks
// Example: pk1.PeakCentre
const boost::regex PEAK_ATTR_REGEX(PEAK_PREFIX + "([0-9]+)\\.(.+)");
// Regex for names of attributes/parameters for peaks
// Example: ion1.pk0.PeakCentre
const boost::regex ION_ATTR_REGEX(ION_PREFIX + "([0-9]+)\\.(.+)");
// Regex for names of attributes/parameters for physical properties
// Example: cv.ScaleFactor
const boost::regex PHYS_PROP_ATTR_REGEX("((ion[0-9]+\\.)?(cv|chi|mh|mt))\\.(.+)");

/// Define the source function for CrystalFieldFunction.
/// Its function() method is not needed.
class Peaks : public CrystalFieldPeaksBase, public API::IFunctionGeneral {
public:
  Peaks() : CrystalFieldPeaksBase() {}
  std::string name() const override { return "Peaks"; }
  size_t getNumberDomainColumns() const override {
    throw Exception::NotImplementedError("This method is intentionally not implemented.");
  }
  size_t getNumberValuesPerArgument() const override {
    throw Exception::NotImplementedError("This method is intentionally not implemented.");
  }
  void functionGeneral(const API::FunctionDomainGeneral & /*domain*/, API::FunctionValues & /*values*/) const override {
    throw Exception::NotImplementedError("This method is intentionally not implemented.");
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
        declareParameter("IntensityScaling" + si, 1.0, "Intensity scaling factor for spectrum " + si);
      } catch (std::invalid_argument &) {
      }
      m_IntensityScalingIdx.emplace_back(parameterIndex("IntensityScaling" + si));
    }
  }
};

} // namespace

/// Constructor
CrystalFieldFunction::CrystalFieldFunction()
    : IFunction(), m_nControlParams(0), m_nControlSourceParams(0), m_dirtyTarget(true) {}

// Evaluates the function
void CrystalFieldFunction::function(const FunctionDomain &domain, FunctionValues &values) const {
  updateTargetFunction();
  if (!m_target) {
    throw std::logic_error("FunctionGenerator failed to generate target function.");
  }
  m_target->function(domain, values);
}

/// Set the source function
/// @param source :: New source function.
void CrystalFieldFunction::setSource(IFunction_sptr source) const { m_source = std::move(source); }

size_t CrystalFieldFunction::getNumberDomains() const {
  if (!m_target) {
    buildTargetFunction();
  }
  // The call to buildTargetFunction() above may have failed to set m_target.
  if (!m_target) {
    throw std::runtime_error("Failed to build target function.");
  }
  return m_target->getNumberDomains();
}

std::vector<IFunction_sptr> CrystalFieldFunction::createEquivalentFunctions() const {
  checkTargetFunction();
  std::vector<IFunction_sptr> funs;
  auto &composite = dynamic_cast<CompositeFunction &>(*m_target);
  for (size_t i = 0; i < composite.nFunctions(); ++i) {
    auto fun = composite.getFunction(i);
    auto cfun = dynamic_cast<CompositeFunction *>(fun.get());
    if (cfun) {
      cfun->checkFunction();
    }
    funs.emplace_back(fun);
  }
  return funs;
}

/// Set i-th parameter
void CrystalFieldFunction::setParameter(size_t i, const double &value, bool explicitlySet) {
  checkSourceFunction();
  if (i < m_nControlParams) {
    m_control.setParameter(i, value, explicitlySet);
    m_dirtyTarget = true;
  } else if (i < m_nControlSourceParams) {
    m_source->setParameter(i - m_nControlParams, value, explicitlySet);
    m_dirtyTarget = true;
  } else {
    checkTargetFunction();
    m_target->setParameter(i - m_nControlSourceParams, value, explicitlySet);
  }
}

/// Set i-th parameter description
void CrystalFieldFunction::setParameterDescription(size_t i, const std::string &description) {
  checkSourceFunction();
  if (i < m_nControlParams) {
    m_control.setParameterDescription(i, description);
  } else if (i < m_nControlSourceParams) {
    m_source->setParameterDescription(i - m_nControlParams, description);
  } else {
    checkTargetFunction();
    m_target->setParameterDescription(i - m_nControlSourceParams, description);
  }
}

/// Get i-th parameter
double CrystalFieldFunction::getParameter(size_t i) const {
  checkSourceFunction();
  checkTargetFunction();
  if (i < m_nControlParams) {
    return m_control.getParameter(i);
  } else if (i < m_nControlSourceParams) {
    return m_source->getParameter(i - m_nControlParams);
  } else {
    return m_target->getParameter(i - m_nControlSourceParams);
  }
}

/// Check if function has a parameter with this name.
bool CrystalFieldFunction::hasParameter(const std::string &name) const {
  try {
    parameterIndex(name);
    return true;
  } catch (std::invalid_argument &) {
    return false;
  }
}

/// Set parameter by name.
void CrystalFieldFunction::setParameter(const std::string &name, const double &value, bool explicitlySet) {
  try {
    auto index = parameterIndex(name);
    setParameter(index, value, explicitlySet);
  } catch (std::invalid_argument &) {
    // Allow ignoring peak parameters: the peak may not exist.
    boost::smatch match;
    if (!boost::regex_search(name, match, PEAK_ATTR_REGEX)) {
      throw;
    }
  }
}

/// Set description of parameter by name.
void CrystalFieldFunction::setParameterDescription(const std::string &name, const std::string &description) {
  auto index = parameterIndex(name);
  setParameterDescription(index, description);
}

/// Get parameter by name.
double CrystalFieldFunction::getParameter(const std::string &name) const {
  auto index = parameterIndex(name);
  return getParameter(index);
}

/// Total number of parameters
size_t CrystalFieldFunction::nParams() const {
  if (!m_source) {
    // This method can be called on an uninitialised function (by tests for
    // example).
    // Return 0 so no exception is thrown an it should prevent attemts to access
    // parameters.
    return 0;
  }
  checkSourceFunction();
  checkTargetFunction();
  return m_nControlSourceParams + m_target->nParams();
}

/// Returns the index of a parameter with a given name
/// @param name :: Name of a parameter.
size_t CrystalFieldFunction::parameterIndex(const std::string &name) const {
  checkSourceFunction();
  checkTargetFunction();
  if (nParams() != m_mapIndices2Names.size()) {
    makeMaps();
  }
  auto found = m_mapNames2Indices.find(name);
  if (found == m_mapNames2Indices.end()) {
    throw std::invalid_argument("CrystalFieldFunction parameter not found: " + name);
  }
  return found->second;
}

/// Returns the name of parameter i
std::string CrystalFieldFunction::parameterName(size_t i) const {
  if (i >= nParams()) {
    throw std::invalid_argument("CrystalFieldFunction's parameter index " + std::to_string(i) + " is out of range " +
                                std::to_string(nParams()));
  }
  checkSourceFunction();
  checkTargetFunction();
  if (nParams() != m_mapIndices2Names.size()) {
    makeMaps();
  }
  return m_mapIndices2Names[i];
}

/// Returns the description of parameter i
std::string CrystalFieldFunction::parameterDescription(size_t i) const {
  checkSourceFunction();
  checkTargetFunction();
  if (i < m_nControlParams) {
    return m_control.parameterDescription(i);
  } else if (i < m_nControlSourceParams) {
    return m_source->parameterDescription(i - m_nControlParams);
  } else {
    return m_target->parameterDescription(i - m_nControlSourceParams);
  }
}

/// Checks if a parameter has been set explicitly
bool CrystalFieldFunction::isExplicitlySet(size_t i) const {
  checkSourceFunction();
  checkTargetFunction();
  if (i < m_nControlParams) {
    return m_control.isExplicitlySet(i);
  } else if (i < m_nControlSourceParams) {
    return m_source->isExplicitlySet(i - m_nControlParams);
  } else {
    return m_target->isExplicitlySet(i - m_nControlSourceParams);
  }
}

/// Get the fitting error for a parameter
double CrystalFieldFunction::getError(size_t i) const {
  checkSourceFunction();
  checkTargetFunction();
  if (i < m_nControlParams) {
    return m_control.getError(i);
  } else if (i < m_nControlSourceParams) {
    return m_source->getError(i - m_nControlParams);
  } else {
    return m_target->getError(i - m_nControlSourceParams);
  }
}

/// Get the fitting error for a parameter
double CrystalFieldFunction::getError(const std::string &name) const {
  auto index = parameterIndex(name);
  checkSourceFunction();
  checkTargetFunction();
  if (index < m_nControlParams) {
    return m_control.getError(index);
  } else if (index < m_nControlSourceParams) {
    return m_source->getError(index - m_nControlParams);
  } else {
    return m_target->getError(index - m_nControlSourceParams);
  }
}

/// Set the fitting error for a parameter
void CrystalFieldFunction::setError(size_t i, double err) {
  checkSourceFunction();
  checkTargetFunction();
  if (i < m_nControlParams) {
    m_control.setError(i, err);
  } else if (i < m_nControlSourceParams) {
    m_source->setError(i - m_nControlParams, err);
  } else {
    m_target->setError(i - m_nControlSourceParams, err);
  }
}

/// Set the fitting error for a parameter
void CrystalFieldFunction::setError(const std::string &name, double err) {
  auto index = parameterIndex(name);
  checkSourceFunction();
  checkTargetFunction();
  if (index < m_nControlParams) {
    m_control.setError(index, err);
  } else if (index < m_nControlSourceParams) {
    m_source->setError(index - m_nControlParams, err);
  } else {
    m_target->setError(index - m_nControlSourceParams, err);
  }
}

/// Change status of parameter
void CrystalFieldFunction::setParameterStatus(size_t i, IFunction::ParameterStatus status) {
  checkSourceFunction();
  checkTargetFunction();
  if (i < m_nControlParams) {
    m_control.setParameterStatus(i, status);
  } else if (i < m_nControlSourceParams) {
    m_source->setParameterStatus(i - m_nControlParams, status);
  } else {
    m_target->setParameterStatus(i - m_nControlSourceParams, status);
  }
}

/// Get status of parameter
IFunction::ParameterStatus CrystalFieldFunction::getParameterStatus(size_t i) const {
  checkSourceFunction();
  checkTargetFunction();
  if (i < m_nControlParams) {
    return m_control.getParameterStatus(i);
  } else if (i < m_nControlSourceParams) {
    return m_source->getParameterStatus(i - m_nControlParams);
  } else {
    return m_target->getParameterStatus(i - m_nControlSourceParams);
  }
}

/// Return parameter index from a parameter reference.
size_t CrystalFieldFunction::getParameterIndex(const ParameterReference &ref) const {
  checkSourceFunction();
  checkTargetFunction();
  if (ref.getLocalFunction() == this) {
    return ref.getLocalIndex();
  }
  auto index = m_control.getParameterIndex(ref);
  if (index < m_nControlParams) {
    return index;
  }
  index = m_source->getParameterIndex(ref);
  if (index < m_source->nParams()) {
    return index + m_nControlParams;
  }
  return m_target->getParameterIndex(ref) + m_nControlSourceParams;
}

/// Set up the function for a fit.
void CrystalFieldFunction::setUpForFit() {
  checkSourceFunction();
  updateTargetFunction();
  IFunction::setUpForFit();
}

/// Declare a new parameter
void CrystalFieldFunction::declareParameter(const std::string & /*name*/, double /*initValue*/,
                                            const std::string & /*description*/) {
  throw Kernel::Exception::NotImplementedError("CrystalFieldFunction cannot have its own parameters.");
}

/// Build and cache the attribute names
void CrystalFieldFunction::buildAttributeNames() const {
  checkSourceFunction();
  checkTargetFunction();
  if (!m_attributeNames.empty()) {
    return;
  }
  auto numAttributes = IFunction::nAttributes();
  for (size_t i = 0; i < numAttributes; ++i) {
    m_attributeNames.emplace_back(IFunction::attributeName(i));
  }
  auto controlAttributeNames = m_control.getAttributeNames();

  // Lambda function that moves a attribute name from controlAttributeNames
  // to attNames.
  auto moveAttributeName = [&](const std::string &name) {
    auto iterFound = std::find(controlAttributeNames.begin(), controlAttributeNames.end(), name);
    if (iterFound != controlAttributeNames.end()) {
      controlAttributeNames.erase(iterFound);
      m_attributeNames.emplace_back(name);
    }
  };
  // Prepend a prefix to attribute names, ignore NumDeriv attribute.
  auto prependPrefix = [&](const std::string &prefix, const std::vector<std::string> &names) {
    for (auto name : names) {
      if (name.find("NumDeriv") != std::string::npos)
        continue;
      name.insert(name.begin(), prefix.begin(), prefix.end());
      m_attributeNames.emplace_back(name);
    }
  };
  // These names must appear first and in this order in the output vector
  moveAttributeName("Ions");
  moveAttributeName("Symmetries");
  moveAttributeName("Temperatures");
  moveAttributeName("Background");

  // Only copy the unprefixed attributes - as the loop below will include
  // And modify the prefixed attributes accordingly
  std::copy_if(controlAttributeNames.begin(), controlAttributeNames.end(), std::back_inserter(m_attributeNames),
               [](const auto &name) { return name.find(".") == std::string::npos; });
  // Get
  for (size_t iSpec = 0; iSpec < m_control.nFunctions(); ++iSpec) {
    std::string prefix(SPECTRUM_PREFIX);
    prefix.append(std::to_string(iSpec)).append(".");
    auto attrNames = m_control.getFunction(iSpec)->getAttributeNames();
    for (auto &attrName : attrNames) {
      attrName.insert(attrName.begin(), prefix.begin(), prefix.end());
    }
    m_attributeNames.insert(m_attributeNames.end(), attrNames.begin(), attrNames.end());
  }
  // Attributes of physical properties
  for (size_t iSpec = nSpectra(); iSpec < m_target->nFunctions(); ++iSpec) {
    auto fun = m_target->getFunction(iSpec).get();
    auto compositePhysProp = dynamic_cast<CompositeFunction *>(fun);
    if (compositePhysProp) {
      // Multi-site case
      std::string physPropPrefix(compositePhysProp->getFunction(0)->name());
      physPropPrefix.append(".");
      for (size_t ion = 0; ion < compositePhysProp->nFunctions(); ++ion) {
        std::string prefix(ION_PREFIX);
        prefix.append(std::to_string(ion)).append(".").append(physPropPrefix);
        auto names = compositePhysProp->getFunction(ion)->getAttributeNames();
        prependPrefix(prefix, names);
      }
    } else {
      // Single-site
      std::string prefix(fun->name());
      prefix.append(".");
      auto names = fun->getAttributeNames();
      prependPrefix(prefix, names);
    }
  }
}

/// Returns the number of attributes associated with the function
size_t CrystalFieldFunction::nAttributes() const {
  buildAttributeNames();
  return m_attributeNames.size();
}

/// Returns a list of attribute names
std::vector<std::string> CrystalFieldFunction::getAttributeNames() const {
  buildAttributeNames();
  return m_attributeNames;
}

/// Return a value of attribute attName
/// @param attName :: Name of an attribute.
IFunction::Attribute CrystalFieldFunction::getAttribute(const std::string &attName) const {
  auto attRef = getAttributeReference(attName);
  if (attRef.first == nullptr) {
    // This will throw an exception because attribute doesn't exist
    return IFunction::getAttribute(attName);
  }
  return attRef.first->getAttribute(attRef.second);
}

/// Perform custom actions on setting certain attributes.
void CrystalFieldFunction::setAttribute(const std::string &attName, const Attribute &attr) {
  auto attRef = getAttributeReference(attName);
  if (attRef.first == nullptr) {
    // This will throw an exception because attribute doesn't exist
    IFunction::setAttribute(attName, attr);
  } else if (attRef.first == &m_control) {
    cacheSourceParameters();
    m_source.reset();
  }
  attRef.first->setAttribute(attRef.second, attr);
  if (attName.find("FWHM") != std::string::npos || attName.find("Background") != std::string::npos) {
    m_dirtyTarget = true;
  }
}

/// Check if attribute attName exists
bool CrystalFieldFunction::hasAttribute(const std::string &attName) const {
  auto attRef = getAttributeReference(attName);
  if (attRef.first == nullptr) {
    return false;
  }
  return attRef.first->hasAttribute(attRef.second);
}

/// Get a reference to an attribute.
/// @param attName :: A name of an attribute. It can be a code rather than an
/// actual name.  This method interprets the code and finds the function and
/// attribute it refers to.
/// @returns :: A pair (IFunction, attribute_name) where attribute_name is a
/// name that the IFunction has.
std::pair<API::IFunction *, std::string> CrystalFieldFunction::getAttributeReference(const std::string &attName) const {
  boost::smatch match;
  if (boost::regex_match(attName, match, SPECTRUM_ATTR_REGEX)) {
    auto index = std::stoul(match[1]);
    auto attNameNoIndex = match[2].str();
    if (m_control.nFunctions() == 0) {
      m_control.buildControls();
    }
    if (attNameNoIndex == "FWHMX" || attNameNoIndex == "FWHMY") {
      if (index < m_control.nFunctions()) {
        return std::make_pair(m_control.getFunction(index).get(), attNameNoIndex);
      } else {
        return std::make_pair(nullptr, "");
      }
    }
    return std::make_pair(nullptr, "");
  } else if (boost::regex_match(attName, match, PHYS_PROP_ATTR_REGEX)) {
    auto prop = match[1].str();
    auto nameRemainder = match[4].str();
    auto propIt = m_mapPrefixes2PhysProps.find(prop);
    if (propIt != m_mapPrefixes2PhysProps.end()) {
      return std::make_pair(propIt->second.get(), nameRemainder);
    }
    return std::make_pair(nullptr, "");
  }
  return std::make_pair(&m_control, attName);
}

/// Get number of the number of spectra (excluding phys prop data).
size_t CrystalFieldFunction::nSpectra() const {
  auto nFuns = m_control.nFunctions();
  return nFuns;
}

/// Get the tie for i-th parameter
ParameterTie *CrystalFieldFunction::getTie(size_t i) const {
  checkSourceFunction();
  checkTargetFunction();
  auto tie = IFunction::getTie(i);
  if (tie) {
    return tie;
  }
  if (i < m_nControlParams) {
    tie = m_control.getTie(i);
  } else if (i < m_nControlSourceParams) {
    tie = m_source->getTie(i - m_nControlParams);
  } else {
    tie = m_target->getTie(i - m_nControlSourceParams);
  }
  return tie;
}

/// Checks if whether tie should be ignored
bool CrystalFieldFunction::ignoreTie(const ParameterTie &tie) const {
  // Ignore height ties for Gaussian peaks as component of a CrystalFieldFunction to avoid problems during tie sorting
  return tie.ownerFunction()->name() == "Gaussian" && tie.parameterName() == "Height" &&
         tie.asString().find("/Sigma") != std::string::npos;
}

/// Get the i-th constraint
IConstraint *CrystalFieldFunction::getConstraint(size_t i) const {
  checkSourceFunction();
  auto constraint = IFunction::getConstraint(i);
  if (constraint == nullptr) {
    if (i < m_nControlParams) {
      constraint = m_control.getConstraint(i);
    } else if (i < m_nControlSourceParams) {
      constraint = m_source->getConstraint(i - m_nControlParams);
    } else {
      checkTargetFunction();
      constraint = m_target->getConstraint(i - m_nControlSourceParams);
    }
  }
  return constraint;
}

/// Check if the function is set up for a multi-site calculations.
/// (Multiple ions defined)
bool CrystalFieldFunction::isMultiSite() const { return m_control.isMultiSite(); }

/// Check if the function is set up for a multi-spectrum calculations
/// (Multiple temperatures defined)
bool CrystalFieldFunction::isMultiSpectrum() const { return m_control.isMultiSpectrum(); }

/// Check if the spectra have a background.
bool CrystalFieldFunction::hasBackground() const {
  if (!hasAttribute("Background")) {
    return false;
  }
  return !getAttribute("Background").isEmpty();
}

/// Check if there are peaks (there is at least one spectrum).
bool CrystalFieldFunction::hasPeaks() const { return m_control.hasPeaks(); }

/// Check if there are any phys. properties.
bool CrystalFieldFunction::hasPhysProperties() const { return m_control.hasPhysProperties(); }

/// Get a reference to the source function if it's composite
API::CompositeFunction &CrystalFieldFunction::compositeSource() const {
  auto composite = dynamic_cast<CompositeFunction *>(m_source.get());
  if (composite == nullptr) {
    throw std::logic_error("Source of CrystalFieldFunction is not composite.");
  }
  return *composite;
}

/// Build source function if necessary.
void CrystalFieldFunction::checkSourceFunction() const {
  if (!m_source) {
    buildSourceFunction();
  }
}

/// Build the source function
void CrystalFieldFunction::buildSourceFunction() const {
  setSource(m_control.buildSource());
  m_nControlParams = m_control.nParams();
  m_nControlSourceParams = m_nControlParams + m_source->nParams();
  if (!m_parameterResetCache.empty() && m_parameterResetCache.size() == m_source->nParams()) {
    for (size_t i = 0; i < m_parameterResetCache.size(); ++i) {
      m_source->setParameter(i, m_parameterResetCache[i]);
      if (m_fixResetCache[i])
        m_source->fix(i);
    }
  }
  m_parameterResetCache.clear();
  m_fixResetCache.clear();
}

/// Update spectrum function if necessary.
void CrystalFieldFunction::checkTargetFunction() const {
  if (m_dirtyTarget) {
    updateTargetFunction();
  }
  if (!m_target) {
    throw std::logic_error("CrystalFieldFunction failed to generate target function.");
  }
}

/// Uses source to calculate peak centres and intensities
/// then populates m_spectrum with peaks of type given in PeakShape attribute.
void CrystalFieldFunction::buildTargetFunction() const {
  checkSourceFunction();
  m_dirtyTarget = false;
  if (isMultiSite()) {
    buildMultiSite();
  } else {
    buildSingleSite();
  }
  m_attributeNames.clear();
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
  m_target->setAttributeValue("NumDeriv", true);
  auto bkgdShape = getAttribute("Background").asUnquotedString();
  bool fixAllPeaks = getAttribute("FixAllPeaks").asBool();

  if (!bkgdShape.empty()) {
    auto background = API::FunctionFactory::Instance().createInitialized(bkgdShape);
    spectrum->addFunction(background);
  }

  FunctionDomainGeneral domain;
  FunctionValues values;
  m_source->function(domain, values);

  if (values.size() == 0) {
    return;
  }

  if (values.size() % 2 != 0) {
    throw std::runtime_error("CrystalFieldPeaks returned odd number of values.");
  }

  auto xVec = m_control.getAttribute("FWHMX").asVector();
  auto yVec = m_control.getAttribute("FWHMY").asVector();
  const auto &FWHMs = m_control.FWHMs();
  auto defaultFWHM = FWHMs.empty() ? 0.0 : FWHMs[0];

  auto fwhmVariation = getAttribute("FWHMVariation").asDouble();
  auto peakShape = getAttribute("PeakShape").asString();
  size_t nRequiredPeaks = getAttribute("NPeaks").asInt();
  CrystalFieldUtils::buildSpectrumFunction(*spectrum, peakShape, values, xVec, yVec, fwhmVariation, defaultFWHM,
                                           nRequiredPeaks, fixAllPeaks);
}

/// Build the target function in a single site - multi spectrum case.
void CrystalFieldFunction::buildSingleSiteMultiSpectrum() const {
  auto fun = new MultiDomainFunction;
  m_target.reset(fun);

  DoubleFortranVector energies;
  ComplexFortranMatrix waveFunctions;
  ComplexFortranMatrix hamiltonian;
  ComplexFortranMatrix hamiltonianZeeman;
  int nre = 0;
  const auto &peakCalculator = dynamic_cast<CrystalFieldPeaksBase &>(*m_source);
  peakCalculator.calculateEigenSystem(energies, waveFunctions, hamiltonian, hamiltonianZeeman, nre);
  hamiltonian += hamiltonianZeeman;

  const auto nSpec = nSpectra();
  const auto &temperatures = m_control.temperatures();
  const auto &FWHMs = m_control.FWHMs();
  const bool addBackground = true;
  for (size_t i = 0; i < nSpec; ++i) {
    auto intensityScaling = m_control.getFunction(i)->getParameter("IntensityScaling");
    fun->addFunction(buildSpectrum(nre, energies, waveFunctions, temperatures[i], FWHMs.size() > i ? FWHMs[i] : 0., i,
                                   addBackground, intensityScaling));
    fun->setDomainIndex(i, i);
  }
  const auto &physProps = m_control.physProps();
  size_t i = nSpec;
  for (const auto &prop : physProps) {
    auto physPropFun = buildPhysprop(nre, energies, waveFunctions, hamiltonian, prop);
    fun->addFunction(physPropFun);
    fun->setDomainIndex(i, i);
    m_mapPrefixes2PhysProps[prop] = physPropFun;
    ++i;
  }
}

/// Build the target function in a multi site - single spectrum case.
void CrystalFieldFunction::buildMultiSiteSingleSpectrum() const {

  auto spectrum = new CompositeFunction;
  m_target.reset(spectrum);
  m_target->setAttributeValue("NumDeriv", true);
  auto bkgdShape = getAttribute("Background").asUnquotedString();
  bool fixAllPeaks = getAttribute("FixAllPeaks").asBool();

  if (!bkgdShape.empty()) {
    auto background = API::FunctionFactory::Instance().createInitialized(bkgdShape);
    spectrum->addFunction(background);
  }

  const auto &FWHMs = m_control.FWHMs();
  auto defaultFWHM = FWHMs.empty() ? 0.0 : FWHMs[0];
  auto fwhmVariation = getAttribute("FWHMVariation").asDouble();
  auto peakShape = getAttribute("PeakShape").asString();
  size_t nRequiredPeaks = getAttribute("NPeaks").asInt();
  auto xVec = m_control.getAttribute("FWHMX").asVector();
  auto yVec = m_control.getAttribute("FWHMY").asVector();

  const auto &compSource = compositeSource();
  for (size_t ionIndex = 0; ionIndex < compSource.nFunctions(); ++ionIndex) {
    FunctionDomainGeneral domain;
    FunctionValues values;
    compSource.getFunction(ionIndex)->function(domain, values);

    if (values.size() == 0) {
      continue;
    }

    if (values.size() % 2 != 0) {
      throw std::runtime_error("CrystalFieldPeaks returned odd number of values.");
    }

    auto ionSpectrum = std::make_shared<CompositeFunction>();
    CrystalFieldUtils::buildSpectrumFunction(*ionSpectrum, peakShape, values, xVec, yVec, fwhmVariation, defaultFWHM,
                                             nRequiredPeaks, fixAllPeaks);
    spectrum->addFunction(ionSpectrum);
  }
}

/// Build the target function in a multi site - multi spectrum case.
void CrystalFieldFunction::buildMultiSiteMultiSpectrum() const {
  auto multiDomain = new MultiDomainFunction;
  m_target.reset(multiDomain);

  const auto nSpec = nSpectra();
  std::vector<CompositeFunction *> spectra(nSpec);
  for (size_t i = 0; i < nSpec; ++i) {
    auto spectrum = std::make_shared<CompositeFunction>();
    spectra[i] = spectrum.get();
    multiDomain->addFunction(spectrum);
    multiDomain->setDomainIndex(i, i);
  }
  auto &physProps = m_control.physProps();
  std::vector<CompositeFunction_sptr> compositePhysProps(physProps.size());
  std::generate(compositePhysProps.begin(), compositePhysProps.end(),
                []() { return std::make_shared<CompositeFunction>(); });

  const auto &compSource = compositeSource();
  for (size_t ionIndex = 0; ionIndex < compSource.nFunctions(); ++ionIndex) {
    DoubleFortranVector energies;
    ComplexFortranMatrix waveFunctions;
    ComplexFortranMatrix hamiltonian;
    ComplexFortranMatrix hamiltonianZeeman;
    int nre = 0;
    const auto &peakCalculator = dynamic_cast<CrystalFieldPeaksBase &>(*compSource.getFunction(ionIndex));
    peakCalculator.calculateEigenSystem(energies, waveFunctions, hamiltonian, hamiltonianZeeman, nre);
    hamiltonian += hamiltonianZeeman;

    auto &temperatures = m_control.temperatures();
    auto &FWHMs = m_control.FWHMs();
    const bool addBackground = ionIndex == 0;
    auto ionIntensityScaling = compSource.getFunction(ionIndex)->getParameter("IntensityScaling");
    for (size_t i = 0; i < nSpec; ++i) {
      auto spectrumIntensityScaling = m_control.getFunction(i)->getParameter("IntensityScaling");
      spectra[i]->addFunction(buildSpectrum(nre, energies, waveFunctions, temperatures[i],
                                            FWHMs.size() > i ? FWHMs[i] : 0., i, addBackground,
                                            ionIntensityScaling * spectrumIntensityScaling));
    }

    size_t i = 0;
    for (const auto &prop : physProps) {
      auto physPropFun = buildPhysprop(nre, energies, waveFunctions, hamiltonian, prop);
      compositePhysProps[i]->addFunction(physPropFun);
      std::string propName = "ion";
      propName.append(std::to_string(ionIndex)).append(".").append(prop);
      m_mapPrefixes2PhysProps[propName] = physPropFun;
      ++i;
    }
  }
  m_target->checkFunction();
  size_t i = nSpec;
  for (const auto &propFun : compositePhysProps) {
    multiDomain->addFunction(propFun);
    multiDomain->setDomainIndex(i, i);
    ++i;
  }
}

/// Calculate excitations at given temperature.
/// @param nre :: An id of the ion.
/// @param energies :: A vector with energies.
/// @param waveFunctions :: A matrix with wave functions.
/// @param temperature :: A temperature of the spectrum.
/// @param values :: An object to receive computed excitations.
/// @param intensityScaling :: A scaling factor for the intensities.
void CrystalFieldFunction::calcExcitations(int nre, const DoubleFortranVector &energies,
                                           const ComplexFortranMatrix &waveFunctions, double temperature,
                                           FunctionValues &values, double intensityScaling) const {
  IntFortranVector degeneration;
  DoubleFortranVector eEnergies;
  DoubleFortranMatrix iEnergies;

  const double toleranceEnergy = getAttribute("ToleranceEnergy").asDouble();
  const double toleranceIntensity = getAttribute("ToleranceIntensity").asDouble();
  DoubleFortranVector eExcitations;
  DoubleFortranVector iExcitations;
  calculateIntensities(nre, energies, waveFunctions, temperature, toleranceEnergy, degeneration, eEnergies, iEnergies);
  calculateExcitations(eEnergies, iEnergies, toleranceEnergy, toleranceIntensity, eExcitations, iExcitations);
  const auto nPeaks = eExcitations.size();
  values.expand(2 * nPeaks);
  for (size_t i = 0; i < nPeaks; ++i) {
    values.setCalculated(i, eExcitations.get(i));
    values.setCalculated(i + nPeaks, iExcitations.get(i) * intensityScaling);
  }
}

/// Build a function for a single spectrum.
/// @param nre :: An id of the ion.
/// @param energies :: A vector with energies.
/// @param waveFunctions :: A matrix with wave functions.
/// @param temperature :: A temperature of the spectrum.
/// @param fwhm :: A full width at half maximum to set to each peak.
/// @param iSpec :: An index of the created spectrum in m_target composite
/// function.
/// @param addBackground :: An option to add a background to the spectrum.
/// @param intensityScaling :: A scaling factor for the peak intensities.
API::IFunction_sptr CrystalFieldFunction::buildSpectrum(int nre, const DoubleFortranVector &energies,
                                                        const ComplexFortranMatrix &waveFunctions, double temperature,
                                                        double fwhm, size_t iSpec, bool addBackground,
                                                        double intensityScaling) const {
  FunctionValues values;
  calcExcitations(nre, energies, waveFunctions, temperature, values, intensityScaling);
  const auto fwhmVariation = getAttribute("FWHMVariation").asDouble();
  const auto peakShape = getAttribute("PeakShape").asString();
  auto bkgdShape = getAttribute("Background").asUnquotedString();
  const size_t nRequiredPeaks = getAttribute("NPeaks").asInt();
  const bool fixAllPeaks = getAttribute("FixAllPeaks").asBool();

  auto spectrum = new CompositeFunction;

  if (addBackground && !bkgdShape.empty()) {
    if (bkgdShape.find("name=") != 0 && bkgdShape.front() != '(') {
      bkgdShape = "name=" + bkgdShape;
    }
    auto background = API::FunctionFactory::Instance().createInitialized(bkgdShape);
    spectrum->addFunction(background);
  }

  auto xVec = m_control.getFunction(iSpec)->getAttribute("FWHMX").asVector();
  auto yVec = m_control.getFunction(iSpec)->getAttribute("FWHMY").asVector();
  CrystalFieldUtils::buildSpectrumFunction(*spectrum, peakShape, values, xVec, yVec, fwhmVariation, fwhm,
                                           nRequiredPeaks, fixAllPeaks);
  return IFunction_sptr(spectrum);
}

/// Build a physical property function.
/// @param nre :: An id of the ion.
/// @param energies :: A vector with energies.
/// @param waveFunctions :: A matrix with wave functions.
/// @param hamiltonian :: A matrix with the hamiltonian.
/// @param propName :: the name of the physical property.
API::IFunction_sptr CrystalFieldFunction::buildPhysprop(int nre, const DoubleFortranVector &energies,
                                                        const ComplexFortranMatrix &waveFunctions,
                                                        const ComplexFortranMatrix &hamiltonian,
                                                        const std::string &propName) const {

  if (propName == "cv") { // HeatCapacity
    auto propFun = std::make_shared<CrystalFieldHeatCapacityCalculation>();
    propFun->setEnergy(energies);
    return propFun;
  }
  if (propName == "chi") { // Susceptibility
    auto propFun = std::make_shared<CrystalFieldSusceptibilityCalculation>();
    propFun->setEigensystem(energies, waveFunctions, nre);
    return propFun;
  }
  if (propName == "mh") { // Magnetisation
    auto propFun = std::make_shared<CrystalFieldMagnetisationCalculation>();
    propFun->setHamiltonian(hamiltonian, nre);
    return propFun;
  }
  if (propName == "mt") { // MagneticMoment
    auto propFun = std::make_shared<CrystalFieldMomentCalculation>();
    propFun->setHamiltonian(hamiltonian, nre);
    return propFun;
  }

  throw std::runtime_error("Physical property type not understood: " + propName);
}

/// Update a physical property function.
/// @param nre :: An id of the ion.
/// @param energies :: A vector with energies.
/// @param waveFunctions :: A matrix with wave functions.
/// @param hamiltonian :: A matrix with the hamiltonian.
/// @param function :: A function to update.
void CrystalFieldFunction::updatePhysprop(int nre, const DoubleFortranVector &energies,
                                          const ComplexFortranMatrix &waveFunctions,
                                          const ComplexFortranMatrix &hamiltonian, API::IFunction &function) const {

  auto propName = function.name();

  if (propName == "cv") { // HeatCapacity
    auto &propFun = dynamic_cast<CrystalFieldHeatCapacityCalculation &>(function);
    propFun.setEnergy(energies);
  } else if (propName == "chi") { // Susceptibility
    auto &propFun = dynamic_cast<CrystalFieldSusceptibilityCalculation &>(function);
    propFun.setEigensystem(energies, waveFunctions, nre);
  } else if (propName == "mh") { // Magnetisation
    auto &propFun = dynamic_cast<CrystalFieldMagnetisationCalculation &>(function);
    propFun.setHamiltonian(hamiltonian, nre);
  } else if (propName == "mt") { // MagneticMoment
    auto &propFun = dynamic_cast<CrystalFieldMomentCalculation &>(function);
    propFun.setHamiltonian(hamiltonian, nre);
  } else {
    throw std::runtime_error("Physical property type not understood: " + propName);
  }
}

/// Update m_spectrum function.
void CrystalFieldFunction::updateTargetFunction() const {
  if (!m_target) {
    buildTargetFunction();
    return;
  }
  m_dirtyTarget = false;
  if (isMultiSite()) {
    updateMultiSite();
  } else {
    updateSingleSite();
  }
  m_target->checkFunction();
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
  auto fwhmVariation = m_control.getAttribute("FWHMVariation").asDouble();
  auto peakShape = m_control.getAttribute("PeakShape").asString();
  bool fixAllPeaks = m_control.getAttribute("FixAllPeaks").asBool();
  auto xVec = m_control.getAttribute("FWHMX").asVector();
  auto yVec = m_control.getAttribute("FWHMY").asVector();
  auto &FWHMs = m_control.FWHMs();
  auto defaultFWHM = FWHMs.empty() ? 0.0 : FWHMs[0];
  size_t indexShift = hasBackground() ? 1 : 0;

  FunctionDomainGeneral domain;
  FunctionValues values;
  m_source->function(domain, values);
  m_target->setAttributeValue("NumDeriv", true);
  auto &spectrum = dynamic_cast<CompositeFunction &>(*m_target);
  CrystalFieldUtils::updateSpectrumFunction(spectrum, peakShape, values, indexShift, xVec, yVec, fwhmVariation,
                                            defaultFWHM, fixAllPeaks);
}

/// Update the target function in a single site - multi spectrum case.
void CrystalFieldFunction::updateSingleSiteMultiSpectrum() const {
  DoubleFortranVector energies;
  ComplexFortranMatrix waveFunctions;
  ComplexFortranMatrix hamiltonian;
  ComplexFortranMatrix hamiltonianZeeman;
  int nre = 0;
  const auto &peakCalculator = dynamic_cast<CrystalFieldPeaksBase &>(*m_source);
  peakCalculator.calculateEigenSystem(energies, waveFunctions, hamiltonian, hamiltonianZeeman, nre);
  hamiltonian += hamiltonianZeeman;
  size_t iFirst = hasBackground() ? 1 : 0;

  const auto &fun = dynamic_cast<MultiDomainFunction &>(*m_target);
  const auto &temperatures = m_control.temperatures();
  const auto &FWHMs = m_control.FWHMs();
  for (size_t iSpec = 0; iSpec < temperatures.size(); ++iSpec) {
    auto intensityScaling = m_control.getFunction(iSpec)->getParameter("IntensityScaling");
    updateSpectrum(*fun.getFunction(iSpec), nre, energies, waveFunctions, temperatures[iSpec],
                   FWHMs.size() > iSpec ? FWHMs[iSpec] : 0., iSpec, iFirst, intensityScaling);
  }

  for (const auto &prop : m_mapPrefixes2PhysProps) {
    updatePhysprop(nre, energies, waveFunctions, hamiltonian, *prop.second);
  }
}

/// Update the target function in a multi site - single spectrum case.
void CrystalFieldFunction::updateMultiSiteSingleSpectrum() const {
  auto fwhmVariation = getAttribute("FWHMVariation").asDouble();
  auto peakShape = getAttribute("PeakShape").asString();
  bool fixAllPeaks = getAttribute("FixAllPeaks").asBool();
  auto xVec = m_control.getAttribute("FWHMX").asVector();
  auto yVec = m_control.getAttribute("FWHMY").asVector();
  auto &FWHMs = m_control.FWHMs();
  auto defaultFWHM = FWHMs.empty() ? 0.0 : FWHMs[0];

  size_t spectrumIndexShift = hasBackground() ? 1 : 0;
  auto &compSource = compositeSource();
  for (size_t ionIndex = 0; ionIndex < compSource.nFunctions(); ++ionIndex) {
    FunctionDomainGeneral domain;
    FunctionValues values;
    compSource.getFunction(ionIndex)->function(domain, values);

    auto &ionSpectrum = dynamic_cast<CompositeFunction &>(*m_target->getFunction(ionIndex + spectrumIndexShift));
    CrystalFieldUtils::updateSpectrumFunction(ionSpectrum, peakShape, values, 0, xVec, yVec, fwhmVariation, defaultFWHM,
                                              fixAllPeaks);
  }
}

/// Update the target function in a multi site - multi spectrum case.
void CrystalFieldFunction::updateMultiSiteMultiSpectrum() const {
  auto &compSource = compositeSource();
  for (size_t ionIndex = 0; ionIndex < compSource.nFunctions(); ++ionIndex) {
    DoubleFortranVector energies;
    ComplexFortranMatrix waveFunctions;
    ComplexFortranMatrix hamiltonian;
    ComplexFortranMatrix hamiltonianZeeman;
    int nre = 0;
    auto &peakCalculator = dynamic_cast<CrystalFieldPeaksBase &>(*compSource.getFunction(ionIndex));
    peakCalculator.calculateEigenSystem(energies, waveFunctions, hamiltonian, hamiltonianZeeman, nre);
    hamiltonian += hamiltonianZeeman;
    size_t iFirst = ionIndex == 0 && hasBackground() ? 1 : 0;

    auto &temperatures = m_control.temperatures();
    auto &FWHMs = m_control.FWHMs();
    auto ionIntensityScaling = compSource.getFunction(ionIndex)->getParameter("IntensityScaling");
    for (size_t iSpec = 0; iSpec < temperatures.size(); ++iSpec) {
      auto &spectrum = dynamic_cast<CompositeFunction &>(*m_target->getFunction(iSpec));
      auto &ionSpectrum = dynamic_cast<CompositeFunction &>(*spectrum.getFunction(ionIndex));
      auto spectrumIntensityScaling = m_control.getFunction(iSpec)->getParameter("IntensityScaling");
      updateSpectrum(ionSpectrum, nre, energies, waveFunctions, temperatures[iSpec],
                     FWHMs.size() > iSpec ? FWHMs[iSpec] : 0., iSpec, iFirst,
                     ionIntensityScaling * spectrumIntensityScaling);
    }

    std::string prefix("ion");
    prefix.append(std::to_string(ionIndex)).append(".");
    auto prefixSize = prefix.size();
    for (const auto &prop : m_mapPrefixes2PhysProps) {
      if (prop.first.substr(0, prefixSize) == prefix) {
        updatePhysprop(nre, energies, waveFunctions, hamiltonian, *prop.second);
      }
    }
  }
}

/// Update a function for a single spectrum.
/// @param spectrum :: A Spectrum function to update.
/// @param nre :: An id of the ion.
/// @param energies :: A vector with energies.
/// @param waveFunctions :: A matrix with wave functions.
/// @param temperature :: A temperature of the spectrum.
/// @param fwhm :: A full width at half maximum to set to each peak.
/// @param iSpec :: An index of the created spectrum in m_target composite
/// function.
/// @param iFirst :: An index of the first peak in spectrum composite function.
/// @param intensityScaling :: A scaling factor for the intensities.
void CrystalFieldFunction::updateSpectrum(API::IFunction &spectrum, int nre, const DoubleFortranVector &energies,
                                          const ComplexFortranMatrix &waveFunctions, double temperature, double fwhm,
                                          size_t iSpec, size_t iFirst, double intensityScaling) const {
  const auto fwhmVariation = getAttribute("FWHMVariation").asDouble();
  const auto peakShape = getAttribute("PeakShape").asString();
  const bool fixAllPeaks = getAttribute("FixAllPeaks").asBool();
  auto xVec = m_control.getFunction(iSpec)->getAttribute("FWHMX").asVector();
  auto yVec = m_control.getFunction(iSpec)->getAttribute("FWHMY").asVector();

  FunctionValues values;
  calcExcitations(nre, energies, waveFunctions, temperature, values, intensityScaling);
  auto &composite = dynamic_cast<API::CompositeFunction &>(spectrum);
  CrystalFieldUtils::updateSpectrumFunction(composite, peakShape, values, iFirst, xVec, yVec, fwhmVariation, fwhm,
                                            fixAllPeaks);
}

/// Make maps between parameter names and indices
void CrystalFieldFunction::makeMaps() const {
  m_mapNames2Indices.clear();
  m_mapIndices2Names.resize(nParams());
  if (isMultiSite()) {
    if (isMultiSpectrum()) {
      makeMapsMultiSiteMultiSpectrum();
    } else {
      makeMapsMultiSiteSingleSpectrum();
    }
  } else {
    if (isMultiSpectrum()) {
      makeMapsSingleSiteMultiSpectrum();
    } else {
      makeMapsSingleSiteSingleSpectrum();
    }
  }
}

/// Make parameter names from names of a function and map them to indices
/// @param fun :: A function to get parameter names from.
/// @param iFirst :: An index that maps to the first parameter of fun.
/// @param prefix :: A prefix to add to all parameters
size_t CrystalFieldFunction::makeMapsForFunction(const IFunction &fun, size_t iFirst, const std::string &prefix) const {
  auto n = fun.nParams();
  for (size_t i = 0; i < n; ++i) {
    size_t j = i + iFirst;
    auto paramName(prefix);
    paramName.append(fun.parameterName(i));
    m_mapNames2Indices[paramName] = j;
    m_mapIndices2Names[j] = paramName;
  }
  return n;
}

/// Parameter-index map for single-site single-spectrum
void CrystalFieldFunction::makeMapsSingleSiteSingleSpectrum() const {
  size_t i = makeMapsForFunction(*m_source, 0, "");

  size_t peakIndex = 0;
  // If there is a background it's the first function in m_target
  if (hasBackground()) {
    const auto &background = *m_target->getFunction(0);
    i += makeMapsForFunction(background, i, BACKGROUND_PREFIX + ".");
    peakIndex = 1;
  }
  // All other functions are peaks.
  for (size_t ip = peakIndex; ip < m_target->nFunctions(); ++ip) {
    std::string prefix(PEAK_PREFIX);
    prefix.append(std::to_string(ip - peakIndex)).append(".");
    i += makeMapsForFunction(*m_target->getFunction(ip), i, prefix);
  }
}

/// Parameter-index map for single-site multi-spectrum
void CrystalFieldFunction::makeMapsSingleSiteMultiSpectrum() const {
  size_t i = 0;
  // Intensity scalings for each spectrum
  for (size_t j = 0; j < m_control.nFunctions(); ++j) {
    std::string prefix(SPECTRUM_PREFIX);
    prefix.append(std::to_string(j)).append(".");
    i += makeMapsForFunction(*m_control.getFunction(j), i, prefix);
  }
  // Crystal field parameters
  i += makeMapsForFunction(*m_source, i, "");

  size_t peakIndex = 0;
  for (size_t iSpec = 0; iSpec < m_target->nFunctions(); ++iSpec) {
    if (auto spectrum = dynamic_cast<const CompositeFunction *>(m_target->getFunction(iSpec).get())) {
      // This is a normal spectrum
      std::string spectrumPrefix(SPECTRUM_PREFIX);
      spectrumPrefix.append(std::to_string(iSpec)).append(".");
      // If there is a background it's the first function in spectrum
      if (hasBackground()) {
        const auto &background = *spectrum->getFunction(0);
        i += makeMapsForFunction(background, i, spectrumPrefix + BACKGROUND_PREFIX + ".");
        peakIndex = 1;
      }
      // All other functions are peaks.
      for (size_t ip = peakIndex; ip < spectrum->nFunctions(); ++ip) {
        std::string prefix(spectrumPrefix);
        prefix.append(PEAK_PREFIX).append(std::to_string(ip - peakIndex)).append(".");
        i += makeMapsForFunction(*spectrum->getFunction(ip), i, prefix);
      }
    } else {
      // This is a physical property function
      std::string prefix(m_control.physProps()[iSpec - nSpectra()]);
      prefix.append(".");
      i += makeMapsForFunction(*m_target->getFunction(iSpec), i, prefix);
    }
  }
}

/// Parameter-index map for multi-site single-spectrum
void CrystalFieldFunction::makeMapsMultiSiteSingleSpectrum() const {
  size_t i = 0;
  // Intensity scalings for each ion
  const auto &crystalField = compositeSource();
  for (size_t ion = 0; ion < crystalField.nFunctions(); ++ion) {
    std::string prefix(ION_PREFIX);
    prefix.append(std::to_string(ion)).append(".");
    i += makeMapsForFunction(*crystalField.getFunction(ion), i, prefix);
  }
  // Spectrum split into an optional background and groups of peaks for
  // each ion
  size_t ionIndex = 0;
  // If there is a background it's the first function in spectrum
  if (hasBackground()) {
    const auto &background = *m_target->getFunction(0);
    i += makeMapsForFunction(background, i, BACKGROUND_PREFIX + ".");
    ionIndex = 1;
  }
  // All other functions are ion spectra.
  for (size_t ion = ionIndex; ion < m_target->nFunctions(); ++ion) {
    std::string ionPrefix(ION_PREFIX);
    ionPrefix.append(std::to_string(ion - ionIndex)).append(".");
    // All other functions are peaks.
    auto &spectrum = dynamic_cast<const CompositeFunction &>(*m_target->getFunction(ion));
    for (size_t ip = 0; ip < spectrum.nFunctions(); ++ip) {
      std::string prefix(ionPrefix);
      prefix.append(PEAK_PREFIX).append(std::to_string(ip)).append(".");
      i += makeMapsForFunction(*spectrum.getFunction(ip), i, prefix);
    }
  }
}

/// Parameter-index map for multi-site multi-spectrum
void CrystalFieldFunction::makeMapsMultiSiteMultiSpectrum() const {
  size_t i = 0;
  // Intensity scalings for each spectrum
  for (size_t j = 0; j < m_control.nFunctions(); ++j) {
    std::string prefix(SPECTRUM_PREFIX);
    prefix.append(std::to_string(j)).append(".");
    i += makeMapsForFunction(*m_control.getFunction(j), i, prefix);
  }
  // Intensity scalings for each ion
  const auto &crystalField = compositeSource();
  for (size_t ion = 0; ion < crystalField.nFunctions(); ++ion) {
    std::string prefix(ION_PREFIX);
    prefix.append(std::to_string(ion)).append(".");
    i += makeMapsForFunction(*crystalField.getFunction(ion), i, prefix);
  }

  // The spectra (background and peak) parameters
  for (size_t iSpec = 0; iSpec < nSpectra(); ++iSpec) {
    auto &spectrum = dynamic_cast<const CompositeFunction &>(*m_target->getFunction(iSpec));
    std::string spectrumPrefix(SPECTRUM_PREFIX);
    spectrumPrefix.append(std::to_string(iSpec)).append(".");

    // All other functions are ion spectra.
    for (size_t ion = 0; ion < crystalField.nFunctions(); ++ion) {
      auto &ionSpectrum = dynamic_cast<const CompositeFunction &>(*spectrum.getFunction(ion));
      size_t peakIndex = 0;
      if (ion == 0 && hasBackground()) {
        peakIndex = 1;
        std::string prefix(spectrumPrefix);
        prefix.append(BACKGROUND_PREFIX).append(".");
        i += makeMapsForFunction(*ionSpectrum.getFunction(0), i, prefix);
      }
      std::string ionPrefix(ION_PREFIX);
      ionPrefix.append(std::to_string(ion)).append(".").append(spectrumPrefix);
      // All other functions are peaks.
      for (size_t ip = peakIndex; ip < ionSpectrum.nFunctions(); ++ip) {
        std::string prefix(ionPrefix);
        prefix.append(PEAK_PREFIX).append(std::to_string(ip - peakIndex)).append(".");
        i += makeMapsForFunction(*ionSpectrum.getFunction(ip), i, prefix);
      }
    }
  }
  // The phys prop parameters
  for (size_t iSpec = nSpectra(); iSpec < m_target->nFunctions(); ++iSpec) {
    auto &spectrum = dynamic_cast<const CompositeFunction &>(*m_target->getFunction(iSpec));
    std::string physPropPrefix(spectrum.getFunction(0)->name());
    physPropPrefix.append(".");
    for (size_t ion = 0; ion < crystalField.nFunctions(); ++ion) {
      std::string prefix(ION_PREFIX);
      prefix.append(std::to_string(ion)).append(".").append(physPropPrefix);
      i += makeMapsForFunction(*spectrum.getFunction(ion), i, prefix);
    }
  }
}

/// Temporary cache parameter values of the source function if it's
/// initialised
void CrystalFieldFunction::cacheSourceParameters() const {
  if (!m_source) {
    // No function - nothing to cache
    m_parameterResetCache.clear();
    return;
  }
  auto np = m_source->nParams();
  m_parameterResetCache.resize(np);
  m_fixResetCache.resize(np);
  for (size_t i = 0; i < np; ++i) {
    m_parameterResetCache[i] = m_source->getParameter(i);
    m_fixResetCache[i] = m_source->isFixed(i);
  }
}

} // namespace Mantid::CurveFitting::Functions
