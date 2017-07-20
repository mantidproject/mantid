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

#include <boost/make_shared.hpp>
#include <boost/optional.hpp>
#include <iostream>
#include <limits>
#include <regex>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

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
const std::regex SPECTRUM_ATTR_REGEX(SPECTRUM_PREFIX + "([0-9]+)\\.(.+)");
// Regex for names of attributes/parameters for a background
// Example: bg.A1
const std::regex BACKGROUND_ATTR_REGEX(BACKGROUND_PREFIX + "\\.(.+)");
// Regex for names of attributes/parameters for peaks
// Example: pk1.PeakCentre
const std::regex PEAK_ATTR_REGEX(PEAK_PREFIX + "([0-9]+)\\.(.+)");
// Regex for names of attributes/parameters for peaks
// Example: ion1.pk0.PeakCentre
const std::regex ION_ATTR_REGEX(ION_PREFIX + "([0-9]+)\\.(.+)");


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
    localName = match[1].str();
    type = Background;
    //if (spectrumIndex == UNDEFINED_INDEX) {
    //  spectrumIndex = 0;
    //}
  }

  if (std::regex_match(localName, match, PEAK_ATTR_REGEX)) {
    if (type == Background) {
      throw std::invalid_argument("Parameter or attribute cannot be both background and peak.");
    }
    peakIndex = std::stoul(match[1].str());
    localName = match[2].str();
    type = Peak;
    //if (spectrumIndex == UNDEFINED_INDEX) {
    //  spectrumIndex = 0;
    //}
  }

  return ReferenceTuple({localName, ionIndex, spectrumIndex, peakIndex, type});
}

CompositeFunction *getSpectrum(CompositeFunction *control,
                               CompositeFunction *source,
                               CompositeFunction *target,
                               const ReferenceTuple &refTuple) {
  CompositeFunction *spectrum = target;
  if (refTuple.spectrumIndex != UNDEFINED_INDEX) {
    spectrum = dynamic_cast<CompositeFunction*>(spectrum->getFunction(refTuple.spectrumIndex).get());
  } else {
  }

  if (refTuple.ionIndex != UNDEFINED_INDEX) {
    spectrum = dynamic_cast<CompositeFunction*>(spectrum->getFunction(refTuple.ionIndex).get());
  }

  if (refTuple.name == "IntensityScaling") {

  }
  return spectrum;
}

/// Forms a name of a parameter of a composite function giving it a custom prefix.
/// @param composite :: Some composite function.
/// @param index :: An index of a parameter
/// @param prefix :: A prefix for the returned name
/// @return A name that starts with the prefix followed by an index of a member function
///   followed by a local name. For example sp0.IntensityScaling
std::string makeName(const CompositeFunction &composite, size_t index,
                     const std::string &prefix) {
  auto funIndex = composite.functionIndex(index);
  auto paramName = composite.parameterLocalName(index);
  std::string name(prefix);
  if (prefix != "bg") {
    name.append(std::to_string(funIndex));
  }
  name.append(".").append(paramName);
  return name;
}

/// Forms a more complex custom parameter name
/// @param composite :: Some composite function.
/// @param index :: An index of a parameter
/// @param prefix :: A prefix for the returned name
/// @return A name that starts with parentPrefix1 followed by an index then 
/// parentPrefix2 if not empty with another index and finally followed by
/// prefix and a local parameter name. Examples: sp0.bg.A1, ion1.sp0.pk3.Sigma
std::string makeComplexName(const CompositeFunction &composite, size_t index,
                            const std::string &prefix,
                            const std::string &parentPrefix1,
                            const std::string &parentPrefix2 = "") {
  auto funIndex = composite.functionIndex(index);
  auto &fun =
      dynamic_cast<const CompositeFunction &>(*composite.getFunction(funIndex));
  size_t localIndex = composite.parameterLocalIndex(index);
  std::string paramName;
  paramName.append(parentPrefix1).append(std::to_string(funIndex)).append(".");
  if (parentPrefix2.empty()) {
    paramName.append(makeName(fun, localIndex, prefix));
  } else {
    paramName.append(makeComplexName(fun, localIndex, prefix, parentPrefix2));
  }
  return paramName;
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
  return m_nControlParams + m_nSourceParams + m_target->nParams();
}

/// Returns the index of a parameter with a given name
/// @param name :: Name of a parameter.
size_t CrystalFieldFunction::parameterIndex(const std::string &name) const {
  checkSourceFunction();
  checkTargetFunction();
  auto ref = getParameterReference(name);
  auto index = m_control.getParameterIndex(ref);
  if (index < m_nControlParams) {
    return index;
  }
  index = m_source->getParameterIndex(ref);
  if (index < m_nSourceParams) {
    return index + m_nControlParams;
  }
  index = m_target->getParameterIndex(ref);
  if (index < m_target->nParams()) {
    return index + m_nControlParams + m_nSourceParams;
  }
  throw std::invalid_argument("CrystalFieldFunction parameter not found: " + name);
}

/// Returns the name of parameter i
std::string CrystalFieldFunction::parameterName(size_t i) const {
  if (i >= nParams()) {
    throw std::invalid_argument("CrystalFieldFunction's parameter index " +
                                std::to_string(i) + " is out of range " +
                                std::to_string(nParams()));
  }
  checkSourceFunction();
  checkTargetFunction();

  if (i < m_nControlParams) {
    if (isMultiSpectrum()) {
      // IntensityScalings for each spectrum
      return makeName(m_control, i, SPECTRUM_PREFIX);
    } else {
      // No parameters here, just for completeness
      return m_control.parameterName(i);
    }
  }

  i -= m_nControlParams;
  if (i < m_nSourceParams) {
    if (isMultiSite()) {
      // Crystal field, intensity scaling for each ion
      return makeName(compositeSource(), i, ION_PREFIX);
    } else {
      // Crystal field
      return m_source->parameterName(i);
    }
  }

  i -= m_nSourceParams;
  if (isMultiSpectrum()) {
    if (isMultiSite()) {
    }
  } else {
    if (isMultiSite()) {
      return makeComplexName(*m_target, i, PEAK_PREFIX, ION_PREFIX);
    } else {
      // Single site, single spectrum
      if (hasBackground()) {
        if (i == 0) {
        }
      }
      return makeName(*m_target, i, PEAK_PREFIX);
    }
  }

  throw Kernel::Exception::NotImplementedError(
      "CrystalFieldFunction::parameterName not implemented properly.");
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
  auto controlAttributeNames = m_control.getAttributeNames();
  // Lambda function that moves a attribute name from controlAttributeNames
  // to attNames.
  auto moveAttributeName =
      [&](const std::string &name) {
        auto iterFound = std::find(controlAttributeNames.begin(),
                                   controlAttributeNames.end(), name);
        if (iterFound != controlAttributeNames.end()) {
          controlAttributeNames.erase(iterFound);
          attNames.push_back(name);
        }
  };
  // These names must appear first and in this order in the output vector
  moveAttributeName("Ions");
  moveAttributeName("Symmetries");
  moveAttributeName("Temperatures");
  // Copy the rest of the names
  attNames.insert(attNames.end(), controlAttributeNames.begin(),
                        controlAttributeNames.end());
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
  const auto &ionIndex = refTuple.ionIndex;
  const auto &spectrumIndex = refTuple.spectrumIndex;
  auto &name = refTuple.name;

  try {
  // Check if it's a background's parameter
  if (refTuple.type == Background) {
    auto function = getBackground(spectrumIndex);
    return API::ParameterReference(function, function->parameterIndex(name));
  }

  // Check if it's a peak parameter
  if (refTuple.type == Peak) {
    auto function = getPeak(ionIndex, spectrumIndex, refTuple.peakIndex);
    return API::ParameterReference(function, function->parameterIndex(name));
  }

  // Check if it's a phys prop parameter
  if (refTuple.type == PhysProp) {
    throw Kernel::Exception::NotImplementedError("PhysProps are not implemented.");
  }

  // Check if it's a parameter of a spectrum function
  if (spectrumIndex != UNDEFINED_INDEX) {
    auto function = getSpectrumControl(spectrumIndex);
    return API::ParameterReference(function, function->parameterIndex(name));
  }

  // Check for a ion-specific params
  if (ionIndex != UNDEFINED_INDEX) {
    auto function = getIon(ionIndex);
    return API::ParameterReference(function,
                                   function->parameterIndex(name));
  }

  // A parameter without a prefix is a parameter of m_control
  // for multi-site ...
  if (isMultiSite()) {
    return API::ParameterReference(&m_control,
                                   m_control.parameterIndex(paramName));
  }

  // ... and m_source for single site
  return API::ParameterReference(m_source.get(),
                                 m_source->parameterIndex(paramName));
  } catch (std::invalid_argument &) {
    throw std::invalid_argument("Parameter " + paramName + " not found.");
  }
}

/// Get number of the number of spectra (excluding phys prop data).
size_t CrystalFieldFunction::nSpectra() const {
  return m_control.nFunctions();
}

/// Get a reference to the control function
IFunction *CrystalFieldFunction::getControl() const {
  return &m_control;
}

/// Get a reference to a spectrum control function
IFunction *CrystalFieldFunction::getSpectrumControl(size_t spectrumIndex) const {
  return m_control.getFunction(spectrumIndex).get();
}

/// Get a reference to a function with ion parameters
IFunction *CrystalFieldFunction::getIon(size_t ionIndex) const {
  if (isMultiSite()) {
    return compositeSource().getFunction(ionIndex).get();
  } else {
    return m_source.get();
  }
}

/// Get a reference to a spectrum function
CompositeFunction *
CrystalFieldFunction::getCompositeFor(size_t ionIndex, size_t spectrumIndex,
                                      size_t peakIndex) const {
  CompositeFunction *spectrum = m_target.get();
  if (isMultiSpectrum()) {
    if (spectrumIndex == UNDEFINED_INDEX) {
      throw std::invalid_argument("Spectrum parameter doesn't exist.");
    }
    spectrum = dynamic_cast<CompositeFunction*>(spectrum->getFunction(spectrumIndex).get());
    if (!spectrum) {
      throw std::logic_error("Spectrum function must be composite.");
    }
  } else if (spectrumIndex != UNDEFINED_INDEX) {
    throw std::invalid_argument("Function is not multispectrum.");
  }
  if (isMultiSite() && peakIndex != UNDEFINED_INDEX) {
    size_t indexShift = hasBackground() ? 1 : 0;
    spectrum = dynamic_cast<CompositeFunction*>(spectrum->getFunction(ionIndex + indexShift).get());
    if (!spectrum) {
      throw std::logic_error("Spectrum function must be composite.");
    }
  }
  return spectrum;
}

/// Get a reference to a function with background parameters
IFunction *CrystalFieldFunction::getBackground(size_t spectrumIndex) const {
  if (!hasBackground()) {
    throw std::invalid_argument("Function has no background");
  }
  return getCompositeFor(0, spectrumIndex, UNDEFINED_INDEX)->getFunction(0).get();
}

/// Get a reference to a function with peak parameters
IFunction *CrystalFieldFunction::getPeak(size_t ionIndex, size_t spectrumIndex, size_t peakIndex) const {
  size_t indexShift = 0;
  if (hasBackground() && !isMultiSite()) {
    indexShift = 1;
  }
  return getCompositeFor(ionIndex, spectrumIndex, peakIndex)->getFunction(peakIndex + indexShift).get();
}

  /// Get the i-th spectrum
CompositeFunction_sptr CrystalFieldFunction::getSpectrum(size_t spectrumIndex) {
  checkTargetFunction();
  if (isMultiSpectrum()) {
    return boost::dynamic_pointer_cast<CompositeFunction>(m_target->getFunction(spectrumIndex));
  } else {
    return m_target;
  }
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
  auto xVec = m_control.getAttribute("FWHMX").asVector();
  auto yVec = m_control.getAttribute("FWHMY").asVector();
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
  const bool addBackground = true;
  for (size_t i = 0; i < nSpec; ++i) {
    auto intensityScaling = m_control.getFunction(i)->getParameter("IntensityScaling");
    fun->addFunction(
        buildSpectrum(nre, en, wf, temperatures[i], FWHMs[i], i, addBackground, intensityScaling));
    fun->setDomainIndex(i, i);
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
    auto background =
        API::FunctionFactory::Instance().createInitialized(bkgdShape);
    spectrum->addFunction(background);
    if (fixAllPeaks) {
      background->fixAll();
    }
  }

  auto &FWHMs = m_control.FWHMs();
  auto defaultFWHM = FWHMs.empty() ? 0.0 : FWHMs[0];
  auto fwhmVariation = getAttribute("FWHMVariation").asDouble();
  auto peakShape = getAttribute("PeakShape").asString();
  size_t nRequiredPeaks = getAttribute("NPeaks").asInt();
  auto xVec = m_control.getAttribute("FWHMX").asVector();
  auto yVec = m_control.getAttribute("FWHMY").asVector();

  auto &compSource = compositeSource();
  for (size_t ionIndex = 0; ionIndex < compSource.nFunctions(); ++ionIndex) {
    FunctionDomainGeneral domain;
    FunctionValues values;
    compSource.getFunction(ionIndex)->function(domain, values);

    if (values.size() == 0) {
      continue;
    }

    if (values.size() % 2 != 0) {
      throw std::runtime_error(
          "CrystalFieldPeaks returned odd number of values.");
    }

    auto ionSpectrum = boost::make_shared<CompositeFunction>();
    CrystalFieldUtils::buildSpectrumFunction(*ionSpectrum, peakShape, values, xVec,
                                             yVec, fwhmVariation, defaultFWHM,
                                             nRequiredPeaks, fixAllPeaks);
    spectrum->addFunction(ionSpectrum);
  }
}

/// Build the target function in a multi site - multi spectrum case.
void CrystalFieldFunction::buildMultiSiteMultiSpectrum() const {
  auto multiDomain = new MultiDomainFunction;
  m_target.reset(multiDomain);

  const auto nSpec = nSpectra();
  std::vector<CompositeFunction*> spectra(nSpec);
  for (size_t i = 0; i < nSpec; ++i) {
    auto spectrum = boost::make_shared<CompositeFunction>();
    spectra[i] = spectrum.get();
    multiDomain->addFunction(spectrum);
    multiDomain->setDomainIndex(i, i);
  }

  auto &compSource = compositeSource();
  for (size_t ionIndex = 0; ionIndex < compSource.nFunctions(); ++ionIndex) {
    DoubleFortranVector en;
    ComplexFortranMatrix wf;
    ComplexFortranMatrix ham;
    ComplexFortranMatrix hz;
    int nre = 0;
    auto &peakCalculator = dynamic_cast<CrystalFieldPeaksBase &>(*compSource.getFunction(ionIndex));
    peakCalculator.calculateEigenSystem(en, wf, ham, hz, nre);
    ham += hz;

    auto &temperatures = m_control.temperatures();
    auto &FWHMs = m_control.FWHMs();
    const bool addBackground = ionIndex == 0;
    auto ionIntensityScaling = compSource.getFunction(ionIndex)->getParameter("IntensityScaling");
    for (size_t i = 0; i < nSpec; ++i) {
      auto spectrumIntensityScaling = m_control.getFunction(i)->getParameter("IntensityScaling");
      spectra[i]->addFunction(
          buildSpectrum(nre, en, wf, temperatures[i], FWHMs[i], i, addBackground, ionIntensityScaling * spectrumIntensityScaling));
    }
  }
}

/// Calculate excitations at given temperature
void CrystalFieldFunction::calcExcitations(int nre,
                                           const DoubleFortranVector &en,
                                           const ComplexFortranMatrix &wf,
                                           double temperature,
                                           FunctionValues &values, size_t iSpec,
                                           double intensityScaling) const {
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
    double temperature, double fwhm, size_t iSpec, bool addBackground,
    double intensityScaling) const {
  FunctionValues values;
  calcExcitations(nre, en, wf, temperature, values, iSpec, intensityScaling);
//  CrystalFieldUtils::calculateNPeaks(values);
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
    auto background =
        API::FunctionFactory::Instance().createInitialized(bkgdShape);
    spectrum->addFunction(background);
    if (fixAllPeaks) {
      background->fixAll();
    }
  }

  auto xVec = m_control.getFunction(iSpec)->getAttribute("FWHMX").asVector();
  auto yVec = m_control.getFunction(iSpec)->getAttribute("FWHMX").asVector();
  CrystalFieldUtils::buildSpectrumFunction(
      *spectrum, peakShape, values, xVec, yVec,
      fwhmVariation, fwhm, nRequiredPeaks, fixAllPeaks);
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
  auto xVec = m_control.getAttribute("FWHMX").asVector();
  auto yVec = m_control.getAttribute("FWHMX").asVector();
  auto &FWHMs = m_control.FWHMs();
  auto defaultFWHM = FWHMs.empty() ? 0.0 : FWHMs[0];

  FunctionDomainGeneral domain;
  FunctionValues values;
  m_source->function(domain, values);
  m_target->setAttributeValue("NumDeriv", true);
  auto &spectrum = dynamic_cast<CompositeFunction &>(*m_target);
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
void CrystalFieldFunction::updateMultiSiteSingleSpectrum() const {
  auto fwhmVariation = getAttribute("FWHMVariation").asDouble();
  auto peakShape = getAttribute("PeakShape").asString();
  bool fixAllPeaks = getAttribute("FixAllPeaks").asBool();
  auto xVec = m_control.getAttribute("FWHMX").asVector();
  auto yVec = m_control.getAttribute("FWHMX").asVector();
  auto &FWHMs = m_control.FWHMs();
  auto defaultFWHM = FWHMs.empty() ? 0.0 : FWHMs[0];

  size_t spectrumIndexShift = hasBackground() ? 1 : 0;
  auto &compSource = compositeSource();
  for (size_t ionIndex = 0; ionIndex < compSource.nFunctions(); ++ionIndex) {
    FunctionDomainGeneral domain;
    FunctionValues values;
    compSource.getFunction(ionIndex)->function(domain, values);

    auto &ionSpectrum = dynamic_cast<CompositeFunction &>(
        *m_target->getFunction(ionIndex + spectrumIndexShift));
    CrystalFieldUtils::updateSpectrumFunction(ionSpectrum, peakShape, values, 0,
                                              xVec, yVec, fwhmVariation,
                                              defaultFWHM, fixAllPeaks);
  }
}

/// Update the target function in a multi site - multi spectrum case.
void CrystalFieldFunction::updateMultiSiteMultiSpectrum() const {
  try {
    auto &compSource = compositeSource();
    for (size_t ionIndex = 0; ionIndex < compSource.nFunctions(); ++ionIndex) {
      DoubleFortranVector en;
      ComplexFortranMatrix wf;
      ComplexFortranMatrix ham;
      ComplexFortranMatrix hz;
      int nre = 0;
      auto &peakCalculator = dynamic_cast<CrystalFieldPeaksBase &>(*compSource.getFunction(ionIndex));
      peakCalculator.calculateEigenSystem(en, wf, ham, hz, nre);
      ham += hz;

      auto &temperatures = m_control.temperatures();
      auto &FWHMs = m_control.FWHMs();
      for (size_t i = 0; i < temperatures.size(); ++i) {
        auto &spectrum = dynamic_cast<CompositeFunction &>(*m_target->getFunction(i));
        auto &ionSpectrum = dynamic_cast<CompositeFunction &>(*spectrum.getFunction(ionIndex));
        updateSpectrum(ionSpectrum, nre, en, wf, ham, temperatures[i],
                       FWHMs[i], i);
      }
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
  const auto peakShape = getAttribute("PeakShape").asString();
  const bool fixAllPeaks = getAttribute("FixAllPeaks").asBool();
  auto xVec = m_control.getFunction(iSpec)->getAttribute("FWHMX").asVector();
  auto yVec = m_control.getFunction(iSpec)->getAttribute("FWHMX").asVector();
  auto intensityScaling = m_control.getFunction(iSpec)->getParameter("IntensityScaling");
  FunctionValues values;
  calcExcitations(nre, en, wf, temperature, values, iSpec, intensityScaling);
  auto &composite = dynamic_cast<API::CompositeFunction &>(spectrum);
  CrystalFieldUtils::updateSpectrumFunction(
      composite, peakShape, values, 1, xVec, yVec,
      fwhmVariation, fwhm, fixAllPeaks);
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
