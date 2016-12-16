#include "MantidCurveFitting/Functions/CrystalFieldMultiSpectrum.h"
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

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;
using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(CrystalFieldMultiSpectrum)

namespace {

/// Define the source function for CrystalFieldMultiSpectrum.
/// Its function() method is not needed.
class Peaks : public CrystalFieldPeaksBase, 
              public API::IFunctionGeneral {
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
  /// Decalre the intensity scaling parameters: one per spectrum.
  void declareIntensityScaling(size_t nSpec) {
    for (size_t i = 0; i < nSpec; ++i) {
      auto si = std::to_string(i);
      declareParameter("IntensityScaling" + si, 1.0,
                       "Intensity scaling factor for spectrum " + si);
    }
  }
};
}

/// Constructor
CrystalFieldMultiSpectrum::CrystalFieldMultiSpectrum()
    : FunctionGenerator(IFunction_sptr(new Peaks)) {
  declareAttribute("PhysicalProperties", Attribute(std::vector<double>(1, 0.0)));
  declareAttribute("Temperatures", Attribute(std::vector<double>(1, 1.0)));
  declareAttribute("Background", Attribute("FlatBackground", true));
  declareAttribute("PeakShape", Attribute("Lorentzian"));
  declareAttribute("FWHMs", Attribute(std::vector<double>(1, 0.0)));
  declareAttribute("FWHMX0", Attribute(std::vector<double>()));
  declareAttribute("FWHMY0", Attribute(std::vector<double>()));
  declareAttribute("FWHMVariation", Attribute(0.1));
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
  if (name == "Temperatures") {
    // Define (declare) the parameters for intensity scaling.
    auto nSpec = attr.asVector().size();
    dynamic_cast<Peaks &>(*m_source).declareIntensityScaling(nSpec);
    m_nOwnParams = m_source->nParams();
    m_fwhmX.resize(nSpec);
    m_fwhmY.resize(nSpec);
    for (size_t iSpec = 0; iSpec < nSpec; ++iSpec) {
      auto suffix = std::to_string(iSpec);
      declareAttribute("FWHMX" + suffix, Attribute(m_fwhmX[iSpec]));
      declareAttribute("FWHMY" + suffix, Attribute(m_fwhmY[iSpec]));
    }
  }
  if (name == "PhysicalProperties") {
    auto physpropId = attr.asVector();
    auto nSpec = physpropId.size();
    for (size_t iSpec = 0; iSpec < nSpec; ++iSpec) {
      auto suffix = std::to_string(iSpec);
      switch(static_cast<int>(physpropId[iSpec])) {
        case 4: // Hmag, Hdir, inverse, Unit,
          declareAttribute("Hmag" + suffix, Attribute(1.0));
        case 2: // Hdir, inverse, Unit
          declareAttribute("inverse" + suffix, Attribute(false));
        case 3: // Hdir, Unit
          declareAttribute("Hdir" + suffix, Attribute(std::vector<double>{0., 0., 1.}));
          declareAttribute("Unit" + suffix, Attribute("bohr"));
          break;
      }
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
  auto temperatures = getAttribute("Temperatures").asVector();
  if (temperatures.empty()) {
    throw std::runtime_error("Vector of temperatures cannot be empty.");
  }
  // Get the FWHMs from the attribute and check for consistency.
  auto fwhms = getAttribute("FWHMs").asVector();
  if (fwhms.size() != temperatures.size()) {
    if (fwhms.empty()) {
      throw std::runtime_error("Vector of FWHMs cannot be empty.");
    }
    if (fwhms.size() == 1) {
      auto fwhm = fwhms.front();
      fwhms.resize(temperatures.size(), fwhm);
    } else {
      throw std::runtime_error("Vector of FWHMs must either have same size as "
                               "Temperatures or have size 1.");
    }
  }
  auto nSpec = temperatures.size();
  // Get a list of "spectra" which corresponds to physical properties
  auto physprops = getAttribute("PhysicalProperties").asVector();
  if (physprops.empty()) {
    m_physprops.resize(nSpec, 0);  // Assume no physical properties - just INS
  }
  else if (physprops.size() != nSpec) {
    if (physprops.size() == 1) {
      int physprop = (int)physprops.front();
      m_physprops.resize(nSpec, physprop);
    } else {
      throw std::runtime_error("Vector of PhysicalProperties must have same "
                               "size as Temperatures or size 1.");
    }
  } else {
    m_physprops.assign(physprops.begin(), physprops.end());
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
      fun->addFunction(buildPhysprop(nre, en, wf, ham, temperatures[i], i));
    } else {
      if (m_fwhmX[i].empty()) {
        auto suffix = std::to_string(i);
        m_fwhmX[i] = IFunction::getAttribute("FWHMX" + suffix).asVector();
        m_fwhmY[i] = IFunction::getAttribute("FWHMY" + suffix).asVector();
      }
      fun->addFunction(buildSpectrum(nre, en, wf, temperatures[i], fwhms[i], i));
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
  double intensityScaling = getParameter(m_nOwnParams - nSpec + iSpec);
  auto nPeaks = eExcitations.size();
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

  auto fwhmVariation = getAttribute("FWHMVariation").asDouble();
  auto peakShape = IFunction::getAttribute("PeakShape").asString();
  auto bkgdShape = IFunction::getAttribute("Background").asUnquotedString();

  if (!bkgdShape.empty() && bkgdShape.find("name=") != 0 &&
      bkgdShape.front() != '(') {
    bkgdShape = "name=" + bkgdShape;
  }

  auto spectrum = new CompositeFunction;
  auto background =
      API::FunctionFactory::Instance().createInitialized(bkgdShape);
  spectrum->addFunction(background);

  m_nPeaks[iSpec] = CrystalFieldUtils::buildSpectrumFunction(
      *spectrum, peakShape, values, m_fwhmX[iSpec], m_fwhmY[iSpec],
      fwhmVariation, fwhm);

  return IFunction_sptr(spectrum);
}

API::IFunction_sptr CrystalFieldMultiSpectrum::buildPhysprop(
    int nre, const DoubleFortranVector &en, const ComplexFortranMatrix &wf,
    const ComplexFortranMatrix &ham, double temperature, size_t iSpec) const {
  switch(m_physprops[iSpec]) {
    case 1: { // Heat capacity
      auto spectrum = new CrystalFieldHeatCapacity;
      spectrum->set_eigensystem(en, wf, nre);
      return IFunction_sptr(spectrum);
    }
    case 2: { // Susceptibility
      auto spectrum = new CrystalFieldSusceptibility;
      spectrum->set_eigensystem(en, wf, nre);
      auto suffix = std::to_string(iSpec);
      auto hdir = getAttribute("Hdir" + suffix).asVector();
      spectrum->setAttribute("Hdir", Attribute(hdir));
      auto inverse = getAttribute("inverse" + suffix).asBool();
      spectrum->setAttribute("inverse", Attribute(inverse));
      return IFunction_sptr(spectrum);
    }
    case 3: { // Magnetisation
      auto spectrum = new CrystalFieldMagnetisation;
      spectrum->set_hamiltonian(ham, nre);
      spectrum->setAttribute("Temperature", Attribute(temperature));
      auto suffix = std::to_string(iSpec);
      auto unit = getAttribute("Unit" + suffix).asString();
      spectrum->setAttribute("Unit", Attribute(unit));
      auto hdir = getAttribute("Hdir" + suffix).asVector();
      spectrum->setAttribute("Hdir", Attribute(hdir));
      return IFunction_sptr(spectrum);
    }
    case 4: { // Moment vs temperature
      auto spectrum = new CrystalFieldMoment;
      spectrum->set_hamiltonian(ham, nre);
      auto suffix = std::to_string(iSpec);
      auto unit = getAttribute("Unit" + suffix).asString();
      spectrum->setAttribute("Unit", Attribute(unit));
      auto hdir = getAttribute("Hdir" + suffix).asVector();
      spectrum->setAttribute("Hdir", Attribute(hdir));
      auto hmag = getAttribute("Hmag" + suffix).asDouble();
      spectrum->setAttribute("Hmag", Attribute(hmag));
      auto inverse = getAttribute("inverse" + suffix).asBool();
      spectrum->setAttribute("inverse", Attribute(inverse));
      return IFunction_sptr(spectrum);
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
  auto temperatures = getAttribute("Temperatures").asVector();
  for (size_t i = 0; i < temperatures.size(); ++i) {
    updateSpectrum(*fun.getFunction(i), nre, en, wf, ham, temperatures[i], i);
  }
}

/// Update a function for a single spectrum.
void CrystalFieldMultiSpectrum::updateSpectrum(
    API::IFunction &spectrum, int nre, const DoubleFortranVector &en,
    const ComplexFortranMatrix &wf, const ComplexFortranMatrix &ham, 
    double temperature, size_t iSpec) const {
  switch(m_physprops[iSpec]) {
    case 1: {
      auto &heatcap = dynamic_cast<CrystalFieldHeatCapacity &>(spectrum);
      heatcap.set_eigensystem(en, wf, nre);
      break;
    }
    case 2: {
      auto &suscept = dynamic_cast<CrystalFieldSusceptibility &>(spectrum);
      suscept.set_eigensystem(en, wf, nre);
      break;
    }
    case 3: {
      auto &magnetisation = dynamic_cast<CrystalFieldMagnetisation &>(spectrum);
      magnetisation.set_hamiltonian(ham, nre);
      break;
    }
    case 4: {
      auto &moment = dynamic_cast<CrystalFieldMoment &>(spectrum);
      moment.set_hamiltonian(ham, nre);
      break;
    }
    default:
      auto fwhmVariation = getAttribute("FWHMVariation").asDouble();
      FunctionValues values;
      calcExcitations(nre, en, wf, temperature, values, iSpec);
      auto &composite = dynamic_cast<API::CompositeFunction &>(spectrum);
      m_nPeaks[iSpec] = CrystalFieldUtils::updateSpectrumFunction(
          composite, values, m_nPeaks[iSpec], 1, m_fwhmX[iSpec], m_fwhmY[iSpec],
          fwhmVariation);
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
