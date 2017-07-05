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

#include <boost/optional.hpp>
#include <iostream>
#include <regex>
#include <limits>

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
// Regex for names of attributes/parameters for a background
// Example: bg.A1
const std::regex BACKGROUND_ATTR_REGEX("bg([0-9]*)\\.(.+)");
// Regex for names of attributes/parameters for peaks
// Example: pk1.PeakCentre
const std::regex PEAK_ATTR_REGEX("pk([0-9]+)\\.(.+)");
// Regex for names of attributes/parameters for peaks
// Example: ion1.pk0.PeakCentre
const std::regex ION_ATTR_REGEX("ion([0-9]+)\\.(.+)");


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

/// Value representing an undefined index.
const size_t UNDEFINED_INDEX = std::numeric_limits<size_t>::max();

enum ReferenceTupleType {Background, Peak, PhysProp, Other};

/// A type that defines a structure to reference a parameter or attribute
/// of a composite function
struct ReferenceTuple {
  /// Parameter name in the function specified by the indices
  std::string name;
  /// The ion index
  size_t ionIndex;
  /// The spectrum
  size_t spectrumIndex;
  /// The peak
  size_t peakIndex;
  /// What kind of parameter is referenced
  ReferenceTupleType type;
};

/// Work out parameter of which function the name referes to..
/// @param name :: A name to parse.
ReferenceTuple getReferenceTuple(const std::string &name) {
  auto localName = name;
  size_t ionIndex(UNDEFINED_INDEX);
  size_t spectrumIndex(UNDEFINED_INDEX);
  size_t peakIndex(UNDEFINED_INDEX);
  ReferenceTupleType type(Other);

  std::smatch match;
  if (std::regex_match(name, match, ION_ATTR_REGEX)) {
    ionIndex = std::stoul(match[1].str());
    localName = match[2].str();
  }

  if (std::regex_match(localName, match, SPECTRUM_ATTR_REGEX)) {
    spectrumIndex = std::stoul(match[1].str());
    localName = match[2].str();
  }

  if (std::regex_match(localName, match, BACKGROUND_ATTR_REGEX)) {
    auto indexStr = match[1].str();
    spectrumIndex = indexStr.empty() ? 0 : std::stoul(indexStr);
    localName = match[2].str();
    type = Background;
  }

  if (std::regex_match(localName, match, PEAK_ATTR_REGEX)) {
    if (type == Background) {
      throw std::invalid_argument("Parameter or attribute cannot be both background and peak.");
    }
    peakIndex = std::stoul(match[1].str());
    localName = match[2].str();
    type = Peak;
    if (spectrumIndex == UNDEFINED_INDEX) {
      spectrumIndex = 0;
    }
  }

  return ReferenceTuple({localName, ionIndex, spectrumIndex, peakIndex, type});
}

} // namespace

/// Constructor
CrystalFieldFunction::CrystalFieldFunction() : IFunction(), m_dirtyTarget(true) {
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
  checkSourceFunction();
  if (i < m_nControlParams) {
    m_control.setParameter(i, value, explicitlySet);
    m_dirtyTarget = true;
  } else if (i < m_nSourceParams) {
    m_source->setParameter(i - m_nControlParams, value, explicitlySet);
    m_dirtyTarget = true;
  } else {
    checkTargetFunction();
    m_target->setParameter(i - m_nSourceParams, value, explicitlySet);
  }
}

/// Set i-th parameter description
void CrystalFieldFunction::setParameterDescription(
    size_t i, const std::string &description) {
  checkSourceFunction();
  if (i < m_nControlParams) {
    m_control.setParameterDescription(i, description);
  } else if (i < m_nSourceParams) {
    m_source->setParameterDescription(i - m_nControlParams, description);
  } else {
    checkTargetFunction();
    m_target->setParameterDescription(i - m_nSourceParams, description);
  }
}

/// Get i-th parameter
double CrystalFieldFunction::getParameter(size_t i) const {
  checkSourceFunction();
  checkTargetFunction();
  if (i < m_nControlParams) {
    return m_control.getParameter(i);
  } else if (i < m_nSourceParams) {
    return m_source->getParameter(i - m_nControlParams);
  } else {
    return m_target->getParameter(i - m_nSourceParams);
  }
}

/// Set parameter by name.
void CrystalFieldFunction::setParameter(const std::string &name,
                                        const double &value,
                                        bool explicitlySet) {
  checkSourceFunction();
  checkTargetFunction();
  auto ref = getParameterReference(name);
  ref.setParameter(value, explicitlySet);
}

/// Set description of parameter by name.
void CrystalFieldFunction::setParameterDescription(
    const std::string &name, const std::string &description) {
  checkSourceFunction();
  checkTargetFunction();
  auto ref = getParameterReference(name);
  ref.getLocalFunction()->setParameterDescription(ref.getLocalIndex(),
                                                  description);
}

/// Get parameter by name.
double CrystalFieldFunction::getParameter(const std::string &name) const {
  checkSourceFunction();
  checkTargetFunction();
  auto ref = getParameterReference(name);
  return ref.getParameter();
}

/// Total number of parameters
size_t CrystalFieldFunction::nParams() const {
  checkSourceFunction();
  checkTargetFunction();
  return m_nSourceParams + m_target->nParams();
}

/// Returns the index of parameter name
size_t CrystalFieldFunction::parameterIndex(const std::string &name) const {
  throw Kernel::Exception::NotImplementedError(
      "CrystalFieldFunction::parameterIndex not implemented properly.");
  // checkSourceFunction();
  // auto ref = getParameterReference(name);
  // auto fun = ref.ownerFunction();
  // auto index = ref.parameterIndex();
  // if (fun == &m_control) {
  //  return index;
  //} else if (fun == m_source.get()) {
  //  return m_nControlParams + index;
  //} else {
  //  checkTargetFunction();
  //  return m_nSourceParams + index;
  //}
}

/// Returns the name of parameter i
std::string CrystalFieldFunction::parameterName(size_t i) const {
  checkSourceFunction();
  checkTargetFunction();
  return i < m_nSourceParams ? m_source->parameterName(i)
                          : m_target->parameterName(i - m_nSourceParams);
}

/// Returns the description of parameter i
std::string CrystalFieldFunction::parameterDescription(size_t i) const {
  checkSourceFunction();
  checkTargetFunction();
  return i < m_nSourceParams ? m_source->parameterDescription(i)
                          : m_target->parameterDescription(i - m_nSourceParams);
}

/// Checks if a parameter has been set explicitly
bool CrystalFieldFunction::isExplicitlySet(size_t i) const {
  checkSourceFunction();
  checkTargetFunction();
  return i < m_nSourceParams ? m_source->isExplicitlySet(i)
                          : m_target->isExplicitlySet(i - m_nSourceParams);
}

/// Get the fitting error for a parameter
double CrystalFieldFunction::getError(size_t i) const {
  checkSourceFunction();
  checkTargetFunction();
  return i < m_nSourceParams ? m_source->getError(i)
                          : m_target->getError(i - m_nSourceParams);
}

/// Set the fitting error for a parameter
void CrystalFieldFunction::setError(size_t i, double err) {
  checkSourceFunction();
  if (i < m_nSourceParams) {
    m_source->setError(i, err);
  } else {
    checkTargetFunction();
    m_target->setError(i - m_nSourceParams, err);
  }
}

/// Change status of parameter
void CrystalFieldFunction::setParameterStatus(
    size_t i, IFunction::ParameterStatus status) {
  checkSourceFunction();
  if (i < m_nSourceParams) {
    m_source->setParameterStatus(i, status);
  } else {
    checkTargetFunction();
    m_target->setParameterStatus(i - m_nSourceParams, status);
  }
}

/// Get status of parameter
IFunction::ParameterStatus
CrystalFieldFunction::getParameterStatus(size_t i) const {
  checkSourceFunction();
  if (i < m_nSourceParams) {
    return m_source->getParameterStatus(i);
  } else {
    checkTargetFunction();
    return m_target->getParameterStatus(i - m_nSourceParams);
  }
}

/// Return parameter index from a parameter reference.
size_t
CrystalFieldFunction::getParameterIndex(const ParameterReference &ref) const {
  checkSourceFunction();
  if (ref.getLocalFunction() == this) {
    auto index = ref.getLocalIndex();
    auto np = nParams();
    if (index < np) {
      return index;
    }
    return np;
  }
  checkTargetFunction();
  return m_target->getParameterIndex(ref) + m_nSourceParams;
}

/// Set up the function for a fit.
void CrystalFieldFunction::setUpForFit() {
  checkSourceFunction();
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
  checkSourceFunction();
  checkTargetFunction();
  return IFunction::nAttributes() + m_source->nAttributes() +
         m_target->nAttributes();
}

/// Returns a list of attribute names
std::vector<std::string> CrystalFieldFunction::getAttributeNames() const {
  checkSourceFunction();
  checkTargetFunction();
  std::vector<std::string> attNames = IFunction::getAttributeNames();
  return attNames;
}

/// Return a value of attribute attName
/// @param attName :: Name of an attribute.
IFunction::Attribute
CrystalFieldFunction::getAttribute(const std::string &attName) const {

  auto attRef = getAttributeReference(attName);
  if (attRef.first == nullptr) {
    // This will throw an exception because attribute doesn't exist
    return IFunction::getAttribute(attName);
  }

  return attRef.first->getAttribute(attRef.second);
}

/// Perform custom actions on setting certain attributes.
void CrystalFieldFunction::setAttribute(const std::string &attName,
                                        const Attribute &attr) {
  auto attRef = getAttributeReference(attName);
  if (attRef.first == nullptr) {
    // This will throw an exception because attribute doesn't exist
    IFunction::setAttribute(attName, attr);
  } else if (attRef.first == &m_control) {
    m_source.reset();
  }
  attRef.first->setAttribute(attRef.second, attr);
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
std::pair<API::IFunction *, std::string>
CrystalFieldFunction::getAttributeReference(const std::string &attName) const {
  std::smatch match;
  if (std::regex_match(attName, match, SPECTRUM_ATTR_REGEX)) {
    auto i = std::stoul(match[1]);
    auto name = match[2].str();
    if (m_control.nFunctions() == 0) {
      m_control.buildControls();
    }
    if (name == "FWHMX" || name == "FWHMY") {
      if (i < m_control.nFunctions()) {
        return std::make_pair(m_control.getFunction(i).get(), name);
      } else {
        return std::make_pair(nullptr, "");
      }
    }
    return std::make_pair(nullptr, "");
  }
  return std::make_pair(&m_control, attName);
}

/// Get a reference to a parameter
API::ParameterReference CrystalFieldFunction::getParameterReference(
    const std::string &paramName) const {

  const auto refTuple = getReferenceTuple(paramName);
  const auto &spectrumIndex = refTuple.spectrumIndex;
  auto &name = refTuple.name;
  
  // Check if it's a parameter of a spectrum function
  if (refTuple.type == Other && spectrumIndex != UNDEFINED_INDEX) {
    checkSpectrumIndex(spectrumIndex);
    // IntensityScaling is stored in m_control
    if (name == "IntensityScaling") {
      auto function = m_control.getFunction(spectrumIndex).get();
      return API::ParameterReference(function, function->parameterIndex(name));
    }
    auto function = m_target->getFunction(spectrumIndex).get();
    return API::ParameterReference(function, function->parameterIndex(name));
  }

  // Check if it's a background's parameter
  if (refTuple.type == Background) {
    if (!hasBackground()) {
      throw std::invalid_argument("CrystalFieldFunction has no background.");
    }
    IFunction* function(nullptr);
    if (isMultiSpectrum()) {
      auto &spectrum = dynamic_cast<CompositeFunction&>(*m_target->getFunction(spectrumIndex));
      function = spectrum.getFunction(0).get();
    } else if (spectrumIndex > 0) {
      throw std::invalid_argument("CrystalFieldFunction has no spectrum " + std::to_string(spectrumIndex));
    } else {
      function = m_target->getFunction(0).get();
    }
    return API::ParameterReference(function, function->parameterIndex(name));
  }

  // Check if it's a peak parameter
  if (refTuple.type == Peak) {
    size_t indexShift = hasBackground() ? 1 : 0;
    IFunction* function(nullptr);
    if (isMultiSpectrum()) {
      auto &spectrum = dynamic_cast<CompositeFunction&>(*m_target->getFunction(spectrumIndex));
      function = spectrum.getFunction(refTuple.peakIndex + indexShift).get();
    } else if (spectrumIndex > 0) {
      throw std::invalid_argument("CrystalFieldFunction has no spectrum " + std::to_string(spectrumIndex));
    } else {
      function = m_target->getFunction(refTuple.peakIndex + indexShift).get();
    }
    return API::ParameterReference(function, function->parameterIndex(name));
  }

  // A parameter without a prefix is a parameter of m_source
  return API::ParameterReference(m_source.get(),
                                 m_source->parameterIndex(paramName));
}

/// Get number of the number of spectra (excluding phys prop data).
size_t CrystalFieldFunction::nSpectra() const {
  return m_control.nFunctions();
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
  //checkSpectrumIndex(iSpec);
  //if (attName == "FWHMX") {
  //  if (iSpec < m_fwhmX.size()) {
  //    return Attribute(m_fwhmX[iSpec]);
  //  } else {
  //    return Attribute(std::vector<double>());
  //  }
  //} else if (attName == "FWHMY") {
  //  if (iSpec < m_fwhmY.size()) {
  //    return Attribute(m_fwhmY[iSpec]);
  //  } else {
  //    return Attribute(std::vector<double>());
  //  }
  //} else if (attName == "Temperature") {
  //  return Attribute(m_temperatures[iSpec]);
  //}
  throw std::runtime_error("Attribute " + attName + " not found.");
}

/// Set a value to a spectrum-specific attribute
/// @param iSpec :: Index of a spectrum.
/// @param attName :: Name of an attribute.
/// @param value :: New value of the attribute.
void CrystalFieldFunction::setSpectrumAttribute(size_t iSpec, const std::string &attName, const Attribute &value) {
  //checkSpectrumIndex(iSpec);
  //if (attName == "FWHMX") {
  //  if (iSpec < m_fwhmX.size()) {
  //    m_fwhmX[iSpec] = value.asVector();
  //  }
  //} else if (attName == "FWHMY") {
  //  if (iSpec < m_fwhmY.size()) {
  //    m_fwhmY[iSpec] = value.asVector();
  //  }
  //} else if (attName == "Temperature") {
  //  m_temperatures[iSpec] = value.asDouble();
  //  IFunction::storeAttributeValue("Temperatures", Attribute(m_temperatures));
  //}
}

/// Get the tie for i-th parameter
ParameterTie *CrystalFieldFunction::getTie(size_t i) const {
  checkSourceFunction();
  auto tie = IFunction::getTie(i);
  if (!tie) {
    return nullptr;
  }
  if (i < m_nSourceParams) {
    tie = m_source->getTie(i);
  } else {
    checkTargetFunction();
    tie = m_target->getTie(i - m_nSourceParams);
  }
  return tie;
}

/// Get the i-th constraint
IConstraint *CrystalFieldFunction::getConstraint(size_t i) const {
  checkSourceFunction();
  auto constraint = IFunction::getConstraint(i);
  if (constraint == nullptr) {
    if (i < m_nSourceParams) {
      constraint = m_source->getConstraint(i);
    } else {
      checkTargetFunction();
      constraint = m_target->getConstraint(i - m_nSourceParams);
    }
  }
  return constraint;
}

void CrystalFieldFunction::setIonsAttribute(const std::string &name,
                                            const Attribute &attr) {
}

void CrystalFieldFunction::setSymmetriesAttribute(const std::string &name,
                                                  const Attribute &attr) {
}

//void CrystalFieldFunction::setTemperaturesAttribute(const std::string &name,
//                                                    const Attribute &attr) {
//  m_temperatures = attr.asVector();
//  IFunction::storeAttributeValue(name, attr);
//  declareAttribute("Background", Attribute("", true));
//  declareAttribute("PeakShape", Attribute("Lorentzian"));
//  declareAttribute("FWHMs", Attribute(std::vector<double>()));
//  declareAttribute("FWHMVariation", Attribute(0.1));
//  if (m_temperatures.size() == 1) {
//    declareAttribute("FWHMX", Attribute(std::vector<double>()));
//    declareAttribute("FWHMY", Attribute(std::vector<double>()));
//  }
//  declareAttribute("NPeaks", Attribute(0));
//  declareAttribute("FixAllPeaks", Attribute(false));
//
//  // Define (declare) the parameters for intensity scaling.
//  // const auto nSpec = attr.asVector().size();
//  // dynamic_cast<Peaks &>(*m_source).declareIntensityScaling(nSpec);
//  // m_nSourceParams = m_source->nParams();
//  // m_fwhmX.resize(nSpec);
//  // m_fwhmY.resize(nSpec);
//  // for (size_t iSpec = 0; iSpec < nSpec; ++iSpec) {
//  //  const auto suffix = std::to_string(iSpec);
//  //  declareAttribute("FWHMX" + suffix, Attribute(m_fwhmX[iSpec]));
//  //  declareAttribute("FWHMY" + suffix, Attribute(m_fwhmY[iSpec]));
//  //}
//}

/// Check if the function is set up for a multi-site calculations.
/// (Multiple ions defined)
bool CrystalFieldFunction::isMultiSite() const {
  return m_control.isMultiSite();
}

/// Check if the function is set up for a multi-spectrum calculations
/// (Multiple temperatures defined)
bool CrystalFieldFunction::isMultiSpectrum() const {
  return m_control.isMultiSpectrum();
}

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

/// Test if a name (parameter's or attribute's) belongs to m_source
/// @param aName :: A name to test.
bool CrystalFieldFunction::isSourceName(const std::string &aName) const {
  if (aName.empty()) {
    throw std::invalid_argument(
        "Parameter or attribute name cannot be empty string.");
  }

  return (aName.front() != 'f' || aName.find('.') == std::string::npos);
}

/// Get a reference to the source function if it's composite
API::CompositeFunction &CrystalFieldFunction::compositeSource() const {
  auto composite = dynamic_cast<CompositeFunction*>(m_source.get());
  if (composite == nullptr) {
    throw std::logic_error("Source of CrystalFieldFunction is not composite.");
  }
  return *composite;
}

///// Check that attributes needed to build the source are consistent
//void CrystalFieldFunction::checkSourceConsistent() const {
//  if (m_ions.empty()) {
//    throw std::runtime_error("No ions are set.");
//  }
//  if (m_ions.size() != m_symmetries.size()) {
//    throw std::runtime_error(
//        "Number of ions is different from number of symmetries.");
//  }
//}

/// Check that attributes and parameters are consistent.
/// If not excepion is thrown.
//void CrystalFieldFunction::checkConsistent() const {
//  if (m_control.nFunctions() == 0) {
//    m_control.buildControls();
//  }
//  m_control.checkConsistent();
//}

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
  m_nSourceParams = m_nControlParams + m_source->nParams();
}


/// Update spectrum function if necessary.
void CrystalFieldFunction::checkTargetFunction() const {
  if (m_dirtyTarget) {
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
  checkSourceFunction();
  m_dirtyTarget = false;
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
  m_target->setAttributeValue("NumDeriv", true);
  auto bkgdShape = getAttribute("Background").asUnquotedString();
  bool fixAllPeaks = getAttribute("FixAllPeaks").asBool();

  if (!bkgdShape.empty()) {
    auto background =
        API::FunctionFactory::Instance().createInitialized(bkgdShape);
    spectrum->addFunction(background);
    if (fixAllPeaks) {
      background->fixAll();
    }
  }

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

  //bool hasWidthModel = !m_fwhmX.empty();
  auto xVec = m_control.getFunction(0)->getAttribute("FWHMX").asVector();
  auto yVec = m_control.getFunction(0)->getAttribute("FWHMX").asVector();
  auto &FWHMs = m_control.FWHMs();
  auto defaultFWHM = FWHMs.empty() ? 0.0 : FWHMs[0];

  auto fwhmVariation = getAttribute("FWHMVariation").asDouble();
  auto peakShape = getAttribute("PeakShape").asString();
  size_t nRequiredPeaks = getAttribute("NPeaks").asInt();
  auto nPeaks = CrystalFieldUtils::buildSpectrumFunction(
      *spectrum, peakShape, values, xVec, yVec, fwhmVariation, defaultFWHM,
      nRequiredPeaks, fixAllPeaks);
  (void)nPeaks;
  // storeReadOnlyAttribute("NPeaks", Attribute(static_cast<int>(m_nPeaks)));
}

/// Build the target function in a single site - multi spectrum case.
void CrystalFieldFunction::buildSingleSiteMultiSpectrum() const {
  auto fun = new MultiDomainFunction;
  m_target.reset(fun);

  DoubleFortranVector en;
  ComplexFortranMatrix wf;
  ComplexFortranMatrix ham;
  ComplexFortranMatrix hz;
  int nre = 0;
  auto &peakCalculator = dynamic_cast<CrystalFieldPeaksBase &>(*m_source);
  peakCalculator.calculateEigenSystem(en, wf, ham, hz, nre);
  ham += hz;

  const auto nSpec = nSpectra();
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

  //if (m_fwhmX.empty()) {
  //  m_fwhmX.resize(nSpec);
  //  m_fwhmY.resize(nSpec);
  //}
  auto &temperatures = m_control.temperatures();
  auto &FWHMs = m_control.FWHMs();
  for (size_t i = 0; i < nSpec; ++i) {
    //if (m_fwhmX[i].empty()) {
      // auto suffix = std::to_string(i);
      // m_fwhmX[i] = IFunction::getAttribute("FWHMX" + suffix).asVector();
      // m_fwhmY[i] = IFunction::getAttribute("FWHMY" + suffix).asVector();
    //}
    fun->addFunction(
        buildSpectrum(nre, en, wf, temperatures[i], FWHMs[i], i));
    fun->setDomainIndex(i, i);
  }
}

/// Build the target function in a multi site - single spectrum case.
void CrystalFieldFunction::buildMultiSiteSingleSpectrum() const {
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
  const size_t nSpec = nSpectra();
  // Get intensity scaling parameter "IntensityScaling" + std::to_string(iSpec)
  // using an index instead of a name for performance reasons
  auto &source = dynamic_cast<CrystalFieldPeaksBase &>(*m_source);
  double intensityScaling = 1.0;
  // if (source.m_IntensityScalingIdx.size() == 0) {
  //  intensityScaling = getParameter(m_nSourceParams - nSpec + iSpec);
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
  const auto peakShape = getAttribute("PeakShape").asString();
  auto bkgdShape = getAttribute("Background").asUnquotedString();
  const size_t nRequiredPeaks = getAttribute("NPeaks").asInt();
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

  auto xVec = m_control.getFunction(iSpec)->getAttribute("FWHMX").asVector();
  auto yVec = m_control.getFunction(iSpec)->getAttribute("FWHMX").asVector();
  nPeaks = CrystalFieldUtils::buildSpectrumFunction(
      *spectrum, peakShape, values, xVec, yVec,
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
  m_dirtyTarget = false;
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
  auto xVec = m_control.getFunction(0)->getAttribute("FWHMX").asVector();
  auto yVec = m_control.getFunction(0)->getAttribute("FWHMX").asVector();
  auto &FWHMs = m_control.FWHMs();
  auto defaultFWHM = FWHMs.empty() ? 0.0 : FWHMs[0];

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
  auto &peakCalculator = dynamic_cast<CrystalFieldPeaksBase &>(*m_source);
  peakCalculator.calculateEigenSystem(en, wf, ham, hz, nre);
  ham += hz;

  auto &fun = dynamic_cast<MultiDomainFunction &>(*m_target);
  try {
    auto &temperatures = m_control.temperatures();
    auto &FWHMs = m_control.FWHMs();
    for (size_t i = 0; i < temperatures.size(); ++i) {
      updateSpectrum(*fun.getFunction(i), nre, en, wf, ham, temperatures[i],
                     FWHMs[i], i);
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
  const auto peakShape = getAttribute("PeakShape").asString();
  const bool fixAllPeaks = getAttribute("FixAllPeaks").asBool();
  auto xVec = m_control.getFunction(iSpec)->getAttribute("FWHMX").asVector();
  auto yVec = m_control.getFunction(iSpec)->getAttribute("FWHMX").asVector();
  FunctionValues values;
  calcExcitations(nre, en, wf, temperature, values, iSpec);
  auto &composite = dynamic_cast<API::CompositeFunction &>(spectrum);
  auto nPeaks = CrystalFieldUtils::updateSpectrumFunction(
      composite, peakShape, values, 1, xVec, yVec,
      fwhmVariation, fwhm, fixAllPeaks);
  (void)nPeaks;
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
