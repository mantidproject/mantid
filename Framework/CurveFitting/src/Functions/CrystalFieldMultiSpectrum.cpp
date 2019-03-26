// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Functions/CrystalFieldMultiSpectrum.h"
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

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;
using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(CrystalFieldMultiSpectrum)

namespace {

// Regex for the FWHMX# type strings (single-site mode)
const boost::regex FWHMX_ATTR_REGEX("FWHMX([0-9]+)");
const boost::regex FWHMY_ATTR_REGEX("FWHMY([0-9]+)");

/// Define the source function for CrystalFieldMultiSpectrum.
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
  std::vector<size_t> m_PPChi0IdxChild;
  std::vector<size_t> m_PPChi0IdxSelf;
  /// Declare the intensity scaling parameters: one per spectrum.
  void declareIntensityScaling(size_t nSpec) {
    m_IntensityScalingIdx.clear();
    m_PPLambdaIdxChild.resize(nSpec, -1);
    m_PPLambdaIdxSelf.resize(nSpec, -1);
    m_PPChi0IdxChild.resize(nSpec, -1);
    m_PPChi0IdxSelf.resize(nSpec, -1);
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
  /// Declare the Lambda parameter for susceptibility
  void declarePPLambda(size_t iSpec) {
    if (m_PPLambdaIdxSelf.size() <= iSpec) {
      m_PPLambdaIdxSelf.resize(iSpec + 1, -1);
      m_PPLambdaIdxChild.resize(iSpec + 1, -1);
      m_PPChi0IdxSelf.resize(iSpec + 1, -1);
      m_PPChi0IdxChild.resize(iSpec + 1, -1);
    }
    auto si = std::to_string(iSpec);
    try { // If parameter has already been declared, don't declare it.
      declareParameter("Lambda" + si, 0.0,
                       "Effective exchange coupling of dataset " + si);
    } catch (std::invalid_argument &) {
    }
    try { // If parameter has already been declared, don't declare it.
      declareParameter("Chi0" + si, 0.0,
                       "Effective exchange coupling of dataset " + si);
    } catch (std::invalid_argument &) {
    }
    m_PPLambdaIdxSelf[iSpec] = parameterIndex("Lambda" + si);
    m_PPChi0IdxSelf[iSpec] = parameterIndex("Chi0" + si);
  }
};
} // namespace

/// Constructor
CrystalFieldMultiSpectrum::CrystalFieldMultiSpectrum()
    : FunctionGenerator(IFunction_sptr(new Peaks)) {
  declareAttribute("Temperatures", Attribute(std::vector<double>(1, 1.0)));
  declareAttribute("Background", Attribute("FlatBackground", true));
  declareAttribute("PeakShape", Attribute("Lorentzian"));
  declareAttribute("FWHMs", Attribute(std::vector<double>(1, 0.0)));
  declareAttribute("FWHMX0", Attribute(std::vector<double>()));
  declareAttribute("FWHMY0", Attribute(std::vector<double>()));
  declareAttribute("FWHMVariation", Attribute(0.1));
  declareAttribute("NPeaks", Attribute(0));
  declareAttribute("FixAllPeaks", Attribute(false));
  declareAttribute("PhysicalProperties",
                   Attribute(std::vector<double>(1, 0.0)));
}

size_t CrystalFieldMultiSpectrum::getNumberDomains() const {
  if (!m_target) {
    buildTargetFunction();
  }
  if (!m_target) {
    throw std::runtime_error("Failed to build target function.");
  }
  return m_target->getNumberDomains();
}

std::vector<IFunction_sptr>
CrystalFieldMultiSpectrum::createEquivalentFunctions() const {
  checkTargetFunction();
  std::vector<IFunction_sptr> funs;
  auto &composite = dynamic_cast<CompositeFunction &>(*m_target);
  for (size_t i = 0; i < composite.nFunctions(); ++i) {
    funs.push_back(composite.getFunction(i));
  }
  return funs;
}

/// Perform custom actions on setting certain attributes.
void CrystalFieldMultiSpectrum::setAttribute(const std::string &name,
                                             const Attribute &attr) {
  boost::smatch match;
  if (name == "Temperatures") {
    // Define (declare) the parameters for intensity scaling.
    const auto nSpec = attr.asVector().size();
    dynamic_cast<Peaks &>(*m_source).declareIntensityScaling(nSpec);
    m_nOwnParams = m_source->nParams();
    m_fwhmX.resize(nSpec);
    m_fwhmY.resize(nSpec);
    std::vector<double> new_fwhm = getAttribute("FWHMs").asVector();
    const auto nWidths = new_fwhm.size();
    if (nWidths != nSpec) {
      new_fwhm.resize(nSpec);
      if (nWidths > nSpec) {
        for (size_t iSpec = nWidths; iSpec < nSpec; ++iSpec) {
          new_fwhm[iSpec] = new_fwhm[0];
        }
      }
    }
    FunctionGenerator::setAttribute("FWHMs", Attribute(new_fwhm));
    for (size_t iSpec = 0; iSpec < nSpec; ++iSpec) {
      const auto suffix = std::to_string(iSpec);
      declareAttribute("FWHMX" + suffix, Attribute(m_fwhmX[iSpec]));
      declareAttribute("FWHMY" + suffix, Attribute(m_fwhmY[iSpec]));
    }
  } else if (name == "PhysicalProperties") {
    const auto physpropId = attr.asVector();
    const auto nSpec = physpropId.size();
    auto &source = dynamic_cast<Peaks &>(*m_source);
    for (size_t iSpec = 0; iSpec < nSpec; ++iSpec) {
      const auto suffix = std::to_string(iSpec);
      const auto pptype = static_cast<int>(physpropId[iSpec]);
      switch (pptype) {
      case MagneticMoment: // Hmag, Hdir, inverse, Unit, powder
        declareAttribute("Hmag" + suffix, Attribute(1.0));
      // fall through
      case Susceptibility: // Hdir, inverse, Unit, powder
        declareAttribute("inverse" + suffix, Attribute(false));
      // fall through
      case Magnetisation: // Hdir, Unit, powder
        declareAttribute("Hdir" + suffix,
                         Attribute(std::vector<double>{0., 0., 1.}));
        declareAttribute("Unit" + suffix, Attribute("bohr"));
        declareAttribute("powder" + suffix, Attribute(false));
        break;
      }
      if (pptype == Susceptibility) {
        source.declarePPLambda(iSpec);
        m_nOwnParams = m_source->nParams();
      }
    }
  } else if (boost::regex_match(name, match, FWHMX_ATTR_REGEX)) {
    auto iSpec = std::stoul(match[1]);
    if (m_fwhmX.size() > iSpec) {
      m_fwhmX[iSpec].clear();
    } else {
      throw std::invalid_argument(
          "Temperatures must be defined before resolution model");
    }
  } else if (boost::regex_match(name, match, FWHMY_ATTR_REGEX)) {
    auto iSpec = std::stoul(match[1]);
    if (m_fwhmY.size() > iSpec) {
      m_fwhmY[iSpec].clear();
    } else {
      throw std::invalid_argument(
          "Temperatures must be defined before resolution model");
    }
  }
  FunctionGenerator::setAttribute(name, attr);
}

/// Uses source to calculate peak centres and intensities
/// then populates m_spectrum with peaks of type given in PeakShape attribute.
void CrystalFieldMultiSpectrum::buildTargetFunction() const {
  m_dirty = false;

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

  // Get the temperatures from the attribute
  m_temperatures = getAttribute("Temperatures").asVector();
  if (m_temperatures.empty()) {
    throw std::runtime_error("Vector of temperatures cannot be empty.");
  }
  // Get the FWHMs from the attribute and check for consistency.
  m_FWHMs = getAttribute("FWHMs").asVector();
  if (m_FWHMs.size() != m_temperatures.size()) {
    if (m_FWHMs.empty()) {
      throw std::runtime_error("Vector of FWHMs cannot be empty.");
    }
    if (m_FWHMs.size() == 1) {
      auto fwhm = m_FWHMs.front();
      m_FWHMs.resize(m_temperatures.size(), fwhm);
    } else {
      throw std::runtime_error("Vector of FWHMs must either have same size as "
                               "Temperatures (" +
                               std::to_string(m_temperatures.size()) +
                               ") or have size 1.");
    }
  }
  const auto nSpec = m_temperatures.size();
  // Get a list of "spectra" which corresponds to physical properties
  const auto physprops = getAttribute("PhysicalProperties").asVector();
  if (physprops.empty()) {
    m_physprops.resize(nSpec, 0); // Assume no physical properties - just INS
  } else if (physprops.size() != nSpec) {
    if (physprops.size() == 1) {
      int physprop = static_cast<int>(physprops.front());
      m_physprops.resize(nSpec, physprop);
    } else {
      throw std::runtime_error("Vector of PhysicalProperties must have same "
                               "size as Temperatures or size 1.");
    }
  } else {
    m_physprops.clear();
    for (auto elem : physprops) {
      m_physprops.push_back(static_cast<int>(elem));
    }
  }
  // Create the single-spectrum functions.
  m_nPeaks.resize(nSpec);
  if (m_fwhmX.empty()) {
    m_fwhmX.resize(nSpec);
    m_fwhmY.resize(nSpec);
  }
  for (size_t i = 0; i < nSpec; ++i) {
    if (m_physprops[i] > 0) {
      // This "spectrum" is actually a physical properties dataset.
      fun->addFunction(buildPhysprop(nre, en, wf, ham, m_temperatures[i], i));
    } else {
      if (m_fwhmX[i].empty()) {
        auto suffix = std::to_string(i);
        m_fwhmX[i] = IFunction::getAttribute("FWHMX" + suffix).asVector();
        m_fwhmY[i] = IFunction::getAttribute("FWHMY" + suffix).asVector();
      }
      fun->addFunction(
          buildSpectrum(nre, en, wf, m_temperatures[i], m_FWHMs[i], i));
    }
    fun->setDomainIndex(i, i);
  }
}

/// Calculate excitations at given temperature
void CrystalFieldMultiSpectrum::calcExcitations(
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
  if (source.m_IntensityScalingIdx.empty()) {
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
API::IFunction_sptr CrystalFieldMultiSpectrum::buildSpectrum(
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
  auto background =
      API::FunctionFactory::Instance().createInitialized(bkgdShape);
  spectrum->addFunction(background);
  if (fixAllPeaks) {
    background->fixAll();
  }

  m_nPeaks[iSpec] = CrystalFieldUtils::buildSpectrumFunction(
      *spectrum, peakShape, values, m_fwhmX[iSpec], m_fwhmY[iSpec],
      fwhmVariation, fwhm, nRequiredPeaks, fixAllPeaks);
  return IFunction_sptr(spectrum);
}

API::IFunction_sptr CrystalFieldMultiSpectrum::buildPhysprop(
    int nre, const DoubleFortranVector &en, const ComplexFortranMatrix &wf,
    const ComplexFortranMatrix &ham, double temperature, size_t iSpec) const {
  switch (m_physprops[iSpec]) {
  case HeatCapacity: {
    IFunction_sptr retval = IFunction_sptr(new CrystalFieldHeatCapacity);
    auto &spectrum = dynamic_cast<CrystalFieldHeatCapacity &>(*retval);
    spectrum.setEnergy(en);
    return retval;
  }
  case Susceptibility: {
    IFunction_sptr retval = IFunction_sptr(new CrystalFieldSusceptibility);
    auto &spectrum = dynamic_cast<CrystalFieldSusceptibility &>(*retval);
    spectrum.setEigensystem(en, wf, nre);
    const auto suffix = std::to_string(iSpec);
    spectrum.setAttribute("Hdir", getAttribute("Hdir" + suffix));
    spectrum.setAttribute("inverse", getAttribute("inverse" + suffix));
    spectrum.setAttribute("powder", getAttribute("powder" + suffix));
    dynamic_cast<Peaks &>(*m_source).m_PPLambdaIdxChild[iSpec] =
        spectrum.parameterIndex("Lambda");
    dynamic_cast<Peaks &>(*m_source).m_PPChi0IdxChild[iSpec] =
        spectrum.parameterIndex("Chi0");
    return retval;
  }
  case Magnetisation: {
    IFunction_sptr retval = IFunction_sptr(new CrystalFieldMagnetisation);
    auto &spectrum = dynamic_cast<CrystalFieldMagnetisation &>(*retval);
    spectrum.setHamiltonian(ham, nre);
    spectrum.setAttribute("Temperature", Attribute(temperature));
    const auto suffix = std::to_string(iSpec);
    spectrum.setAttribute("Unit", getAttribute("Unit" + suffix));
    spectrum.setAttribute("Hdir", getAttribute("Hdir" + suffix));
    spectrum.setAttribute("powder", getAttribute("powder" + suffix));
    return retval;
  }
  case MagneticMoment: {
    IFunction_sptr retval = IFunction_sptr(new CrystalFieldMoment);
    auto &spectrum = dynamic_cast<CrystalFieldMoment &>(*retval);
    spectrum.setHamiltonian(ham, nre);
    const auto suffix = std::to_string(iSpec);
    spectrum.setAttribute("Unit", getAttribute("Unit" + suffix));
    spectrum.setAttribute("Hdir", getAttribute("Hdir" + suffix));
    spectrum.setAttribute("Hmag", getAttribute("Hmag" + suffix));
    spectrum.setAttribute("inverse", getAttribute("inverse" + suffix));
    spectrum.setAttribute("powder", getAttribute("powder" + suffix));
    return retval;
  }
  }
  throw std::runtime_error("Physical property type not understood");
}

/// Update m_spectrum function.
void CrystalFieldMultiSpectrum::updateTargetFunction() const {
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
    fun.checkFunction();
  } catch (std::out_of_range &) {
    buildTargetFunction();
    return;
  }
}

/// Update a function for a single spectrum.
void CrystalFieldMultiSpectrum::updateSpectrum(
    API::IFunction &spectrum, int nre, const DoubleFortranVector &en,
    const ComplexFortranMatrix &wf, const ComplexFortranMatrix &ham,
    double temperature, double fwhm, size_t iSpec) const {
  switch (m_physprops[iSpec]) {
  case HeatCapacity: {
    auto &heatcap = dynamic_cast<CrystalFieldHeatCapacity &>(spectrum);
    heatcap.setEnergy(en);
    break;
  }
  case Susceptibility: {
    auto &suscept = dynamic_cast<CrystalFieldSusceptibility &>(spectrum);
    suscept.setEigensystem(en, wf, nre);
    auto &source = dynamic_cast<Peaks &>(*m_source);
    suscept.setParameter(source.m_PPLambdaIdxChild[iSpec],
                         getParameter(source.m_PPLambdaIdxSelf[iSpec]));
    suscept.setParameter(source.m_PPChi0IdxChild[iSpec],
                         getParameter(source.m_PPChi0IdxSelf[iSpec]));
    break;
  }
  case Magnetisation: {
    auto &magnetisation = dynamic_cast<CrystalFieldMagnetisation &>(spectrum);
    magnetisation.setHamiltonian(ham, nre);
    break;
  }
  case MagneticMoment: {
    auto &moment = dynamic_cast<CrystalFieldMoment &>(spectrum);
    moment.setHamiltonian(ham, nre);
    break;
  }
  default:
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
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
