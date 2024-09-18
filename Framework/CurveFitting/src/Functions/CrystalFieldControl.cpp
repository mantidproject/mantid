// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Functions/CrystalFieldControl.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeaks.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/Strings.h"

namespace Mantid::CurveFitting::Functions {

using namespace API;

CrystalFieldControl::CrystalFieldControl() : CompositeFunction() {
  const bool quotedString = true;
  // A comma-separated list of ion names
  declareAttribute("Ions", Attribute("", quotedString));
  // A comma-separated list of symmetry names
  declareAttribute("Symmetries", Attribute("", quotedString));
  // Temperature values for each spectrum.
  declareAttribute("Temperatures", Attribute(std::vector<double>()));
  // Default widths for peaks in each spectrum. If given it must have
  // the same size as Temperatures or size == 1 in which case it's used
  // for all spectra.
  declareAttribute("FWHMs", Attribute(std::vector<double>()));
  // Variation in FWHM of peaks when with model is used (FWHMX and FWHMY)
  declareAttribute("FWHMVariation", Attribute(0.1));
  // Definition of the background function
  declareAttribute("Background", Attribute("", quotedString));
  // Name of a IPeakFunction to use for peaks
  declareAttribute("PeakShape", Attribute("Lorentzian"));
  // Energy tolerance in crystal field calculations
  declareAttribute("ToleranceEnergy", Attribute(1.0e-10));
  // Intensity tolerance in crystal field calculations
  declareAttribute("ToleranceIntensity", Attribute(1.0e-1));
  // A comma-separated list of physical properties
  declareAttribute("PhysicalProperties", Attribute(""));
  declareAttribute("NPeaks", Attribute(0));
  declareAttribute("FixAllPeaks", Attribute(false));
}

/// Set a value to attribute attName
void CrystalFieldControl::setAttribute(const std::string &name, const API::IFunction::Attribute &attr) {
  if (name == "Ions") {
    parseStringListAttribute("Ions", attr.asUnquotedString(), m_ions);
  } else if (name == "Symmetries") {
    parseStringListAttribute("Symmetries", attr.asUnquotedString(), m_symmetries);
  } else if (name == "PhysicalProperties") {
    parseStringListAttribute("PhysicalProperties", attr.asString(), m_physProps);
    buildPhysPropControls();
  } else {
    if (name == "Temperatures") {
      m_temperatures = attr.asVector();
      buildControls();
    } else if (name == "FWHMs") {
      const size_t nSpec = m_temperatures.size();
      m_FWHMs = attr.asVector();
      if (m_FWHMs.empty())
        return;
      if (m_FWHMs.size() == 1 && m_FWHMs.size() != nSpec) {
        auto frontValue = m_FWHMs.front();
        m_FWHMs.assign(nSpec, frontValue);
      }
      m_fwhmX.resize(nSpec);
      m_fwhmY.resize(nSpec);
      for (size_t i = 0; i < nSpec; ++i) {
        m_fwhmX[i].clear();
        m_fwhmY[i].clear();
        if (nSpec > 1) {
          auto &control = *getFunction(i);
          control.setAttributeValue("FWHMX", std::vector<double>());
          control.setAttributeValue("FWHMY", std::vector<double>());
        } else {
          API::IFunction::setAttributeValue("FWHMX", std::vector<double>());
          API::IFunction::setAttributeValue("FWHMY", std::vector<double>());
        }
      }
    } else if ((name.compare(0, 5, "FWHMX") == 0 || name.compare(0, 5, "FWHMY") == 0) && !attr.asVector().empty()) {
      m_FWHMs.clear();
    }
    API::IFunction::setAttribute(name, attr);
  }
}

/// Parse a comma-separated list attribute
/// @param attName :: A name of the attribute to parse.
/// @param value :: A value to parse.
/// @param cache :: A vector to cache the parsed values.
void CrystalFieldControl::parseStringListAttribute(const std::string &attName, const std::string &value,
                                                   std::vector<std::string> &cache) {
  // Split the attribute value into separate names, white spaces removed
  Kernel::StringTokenizer tokenizer(value, ",", Kernel::StringTokenizer::TOK_TRIM);
  cache.clear();
  cache.insert(cache.end(), tokenizer.begin(), tokenizer.end());
  auto attrValue = Kernel::Strings::join(cache.begin(), cache.end(), ",");
  // Store back the trimmed names - we can't use setAttributeValue as it will
  // call this setAttribute
  auto attrCopy = getAttribute(attName);
  attrCopy.setString(attrValue);
  API::IFunction::setAttribute(attName, attrCopy);
}

/// Cache function attributes.
void CrystalFieldControl::cacheAttributes() {
  const auto nSpec = m_temperatures.size();
  m_fwhmX.clear();
  m_fwhmY.clear();
  if (nSpec == 1) {
    auto fwhmX = getAttribute("FWHMX").asVector();
    auto fwhmY = getAttribute("FWHMY").asVector();
    m_fwhmX.emplace_back(fwhmX);
    m_fwhmY.emplace_back(fwhmY);
  } else {
    for (size_t i = 0; i < nSpec; ++i) {
      auto &control = *getFunction(i);
      auto fwhmX = control.getAttribute("FWHMX").asVector();
      auto fwhmY = control.getAttribute("FWHMY").asVector();
      m_fwhmX.emplace_back(fwhmX);
      m_fwhmY.emplace_back(fwhmY);
    }
  }
}

/// Check that all attributes are consistent
void CrystalFieldControl::checkConsistent() {
  if (m_ions.empty()) {
    throw std::runtime_error("No ions are set.");
  }
  if (m_ions.size() != m_symmetries.size()) {
    throw std::runtime_error("Number of ions is different from number of symmetries.");
  }
  if (!m_temperatures.empty()) {
    // If temperatures are given then there will be spectra with peaks
    // and peaks need some width information. It comes either from
    // FWHMs attribute or FWHMX and FWHMY attributes of the spectrum
    // control functions.
    const auto nSpec = m_temperatures.size();
    if (nSpec > 1 && nSpec > nFunctions()) {
      // Some of the member control funtions must be spectra
      throw std::logic_error("Too few spectrum functions.");
    }
    // Check if the FWHMX and FWHMY attributes of the spectrum
    // control functions are set and if they are they have equal lengths
    bool allXYEmpty = true;
    bool someXYEmpty = false;
    for (size_t i = 0; i < nSpec; ++i) {
      if (nSpec > 1) {
        auto specFun = getFunction(i).get();
        if (!dynamic_cast<CrystalFieldSpectrumControl *>(specFun)) {
          throw std::logic_error("CrystalFieldSpectrumControl function expected");
        }
      }
      if (m_fwhmX[i].size() != m_fwhmY[i].size()) {
        throw std::runtime_error("Vectors in each pair of (FWHMX, FWHMY) "
                                 "attributes must have the same size");
      }
      someXYEmpty = someXYEmpty || m_fwhmX[i].empty() || m_fwhmY[i].empty();
      allXYEmpty = allXYEmpty && m_fwhmX[i].empty() && m_fwhmY[i].empty();
    }
    if (m_FWHMs.empty()) {
      if (allXYEmpty) {
        // No width information given
        throw std::runtime_error("No peak width settings (FWHMs and FWHMX and "
                                 "FWHMY attributes not set).");
      } else if (someXYEmpty) {
        // If given they all must be given
        throw std::runtime_error("FWHMX, FWHMY attributes are not given for all spectra.");
      }
    } else if (m_FWHMs.size() != nSpec) {
      if (m_FWHMs.size() == 1) {
        // If single value then use it for all spectra
        m_FWHMs.assign(nSpec, m_FWHMs.front());
      } else {
        // Otherwise it's an error
        throw std::runtime_error("Vector of FWHMs must either have same size as "
                                 "Temperatures (" +
                                 std::to_string(nSpec) + ") or have size 1.");
      }
    } else if (!allXYEmpty) {
      // Conflicting width attributes
      throw std::runtime_error("Either FWHMs or (FWHMX and FWHMY) can be set but not all.");
    }
  } else if (physProps().empty()) {
    throw std::runtime_error("No temperatures are set.");
  }
}

const std::vector<double> &CrystalFieldControl::temperatures() const { return m_temperatures; }

const std::vector<double> &CrystalFieldControl::FWHMs() const { return m_FWHMs; }

const std::vector<std::string> &CrystalFieldControl::physProps() const { return m_physProps; }

/// Build control functions for individual spectra.
void CrystalFieldControl::buildControls() {
  const auto nSpec = m_temperatures.size();
  if (nSpec == 1) {
    declareAttribute("FWHMX", Attribute(std::vector<double>()));
    declareAttribute("FWHMY", Attribute(std::vector<double>()));
  } else {
    for (size_t i = 0; i < nSpec; ++i) {
      addFunction(API::IFunction_sptr(new CrystalFieldSpectrumControl));
    }
  }
}

/// Build control functions for phys properties.
void CrystalFieldControl::buildPhysPropControls() {
  const auto nSpec = m_temperatures.size();
  if (nSpec == 1) {
    addFunction(API::IFunction_sptr(new CrystalFieldSpectrumControl));
  }
}

/// Check if the function is set up for a multi-site calculations.
/// (Multiple ions defined)
bool CrystalFieldControl::isMultiSite() const { return m_ions.size() > 1; }

bool CrystalFieldControl::isMultiSpectrum() const { return m_temperatures.size() > 1 || !m_physProps.empty(); }

/// Any peaks defined?
bool CrystalFieldControl::hasPeaks() const { return !m_temperatures.empty(); }

/// Check if there are any phys. properties.
bool CrystalFieldControl::hasPhysProperties() const { return !m_physProps.empty(); }

/// Build the source function.
API::IFunction_sptr CrystalFieldControl::buildSource() {
  cacheAttributes();
  checkConsistent();
  if (isMultiSite()) {
    return buildMultiSite();
  } else {
    return buildSingleSite();
  }
}

/// Build the source function in a single site case.
API::IFunction_sptr CrystalFieldControl::buildSingleSite() {
  if (isMultiSpectrum()) {
    return buildSingleSiteMultiSpectrum();
  } else {
    return buildSingleSiteSingleSpectrum();
  }
}

/// Build the source function in a multi site case.
API::IFunction_sptr CrystalFieldControl::buildMultiSite() {
  if (isMultiSpectrum()) {
    return buildMultiSiteMultiSpectrum();
  } else {
    return buildMultiSiteSingleSpectrum();
  }
}

/// Build the source function in a single site - single spectrum case.
API::IFunction_sptr CrystalFieldControl::buildSingleSiteSingleSpectrum() {
  if (m_temperatures.empty()) {
    throw std::runtime_error("No tmperature was set.");
  }
  auto source = IFunction_sptr(new CrystalFieldPeaks);
  source->setAttributeValue("Ion", m_ions[0]);
  source->setAttributeValue("Symmetry", m_symmetries[0]);
  source->setAttribute("ToleranceEnergy", IFunction::getAttribute("ToleranceEnergy"));
  source->setAttribute("ToleranceIntensity", IFunction::getAttribute("ToleranceIntensity"));
  source->setAttributeValue("Temperature", m_temperatures[0]);
  return source;
}

/// Build the source function in a single site - multi spectrum case.
API::IFunction_sptr CrystalFieldControl::buildSingleSiteMultiSpectrum() {
  auto source = IFunction_sptr(new CrystalFieldPeaksBaseImpl);
  source->setAttributeValue("Ion", m_ions[0]);
  source->setAttributeValue("Symmetry", m_symmetries[0]);
  source->setAttribute("ToleranceEnergy", IFunction::getAttribute("ToleranceEnergy"));
  source->setAttribute("ToleranceIntensity", IFunction::getAttribute("ToleranceIntensity"));
  return source;
}

/// Build the source function in a multi site - single spectrum case.
API::IFunction_sptr CrystalFieldControl::buildMultiSiteSingleSpectrum() {
  auto source = CompositeFunction_sptr(new CompositeFunction);
  auto nSites = m_ions.size();
  auto temperature = getAttribute("Temperatures").asVector()[0];
  for (size_t i = 0; i < nSites; ++i) {
    auto peakSource = IFunction_sptr(new CrystalFieldPeaks);
    source->addFunction(peakSource);
    peakSource->setAttributeValue("Ion", m_ions[i]);
    peakSource->setAttributeValue("Symmetry", m_symmetries[i]);
    peakSource->setAttribute("ToleranceEnergy", IFunction::getAttribute("ToleranceEnergy"));
    peakSource->setAttribute("ToleranceIntensity", IFunction::getAttribute("ToleranceIntensity"));
    peakSource->setAttributeValue("Temperature", temperature);
  }
  return source;
}

/// Build the source function in a multi site - multi spectrum case.
API::IFunction_sptr CrystalFieldControl::buildMultiSiteMultiSpectrum() {
  auto source = CompositeFunction_sptr(new CompositeFunction);
  auto nSites = m_ions.size();
  for (size_t i = 0; i < nSites; ++i) {
    auto peakSource = IFunction_sptr(new CrystalFieldPeaks);
    source->addFunction(peakSource);
    peakSource->setAttributeValue("Ion", m_ions[i]);
    peakSource->setAttributeValue("Symmetry", m_symmetries[i]);
    peakSource->setAttribute("ToleranceEnergy", IFunction::getAttribute("ToleranceEnergy"));
    peakSource->setAttribute("ToleranceIntensity", IFunction::getAttribute("ToleranceIntensity"));
  }
  return source;
}

// -----------------------------------------------------------------------------------
// //

CrystalFieldSpectrumControl::CrystalFieldSpectrumControl() : ParamFunction() {
  declareAttribute("FWHMX", Attribute(std::vector<double>()));
  declareAttribute("FWHMY", Attribute(std::vector<double>()));
  declareParameter("IntensityScaling", 1.0, "Scales intensities of peaks in a spectrum.");
}

std::string CrystalFieldSpectrumControl::name() const { return "CrystalFieldSpectrumControl"; }

void CrystalFieldSpectrumControl::function(const API::FunctionDomain & /*domain*/,
                                           API::FunctionValues & /*values*/) const {
  throw Kernel::Exception::NotImplementedError("This method is intentionally not implemented.");
}

// -----------------------------------------------------------------------------------
// //

CrystalFieldPhysPropControl::CrystalFieldPhysPropControl() : ParamFunction() {}

std::string CrystalFieldPhysPropControl::name() const { return "CrystalFieldPhysPropControl"; }

void CrystalFieldPhysPropControl::function(const API::FunctionDomain & /*domain*/,
                                           API::FunctionValues & /*values*/) const {
  throw Kernel::Exception::NotImplementedError("This method is intentionally not implemented.");
}

} // namespace Mantid::CurveFitting::Functions
