#include "MantidCurveFitting/Functions/CrystalFieldMultiSpectrum.h"
#include "MantidCurveFitting/Functions/CrystalElectricField.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeaks.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IConstraint.h"
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
class Peaks : public CrystalFieldPeaksBase {
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
  declareAttribute("Temperatures", Attribute(std::vector<double>(1, 1.0)));
  declareAttribute("Background", Attribute("FlatBackground", true));
  declareAttribute("PeakShape", Attribute("Lorentzian"));
  declareAttribute("FWHMs", Attribute(std::vector<double>(1, 0.0)));
  declareAttribute("FixAllPeakParameters", Attribute(false));
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
  int nre = 0;
  auto &peakCalculator = dynamic_cast<Peaks &>(*m_source);
  peakCalculator.calculateEigenSystem(en, wf, nre);

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
  // Create the single-spectrum functions.
  auto nSpec = temperatures.size();
  m_nPeaks.resize(nSpec);
  m_maxNPeaks.resize(nSpec);
  for (size_t i = 0; i < nSpec; ++i) {
    fun->addFunction(buildSpectrum(nre, en, wf, temperatures[i], fwhms[i], i));
    fun->setDomainIndex(i, i);
  }
}

/// Calculate excitations at given temperature
void CrystalFieldMultiSpectrum::calcExcitations(
    int nre, const DoubleFortranVector &en, const ComplexFortranMatrix &wf,
    double temperature, DoubleFortranVector &eExcitations,
    DoubleFortranVector &iExcitations) const {
  IntFortranVector degeneration;
  DoubleFortranVector eEnergies;
  DoubleFortranMatrix iEnergies;
  const double de = getAttribute("ToleranceEnergy").asDouble();
  const double di = getAttribute("ToleranceIntensity").asDouble();
  calculateIntensities(nre, en, wf, temperature, de, degeneration, eEnergies,
                       iEnergies);
  calculateExcitations(eEnergies, iEnergies, de, di, eExcitations,
                       iExcitations);
}

/// Build a function for a single spectrum.
API::IFunction_sptr CrystalFieldMultiSpectrum::buildSpectrum(
    int nre, const DoubleFortranVector &en, const ComplexFortranMatrix &wf,
    double temperature, double fwhm, size_t iSpec) const {
  DoubleFortranVector eExcitations;
  DoubleFortranVector iExcitations;
  calcExcitations(nre, en, wf, temperature, eExcitations, iExcitations);
  auto nPeaks = eExcitations.size();
  auto maxNPeaks = nPeaks + nPeaks / 2 + 1;
  m_nPeaks[iSpec] = nPeaks;
  m_maxNPeaks[iSpec] = maxNPeaks;
  bool fixAll = getAttribute("FixAllPeakParameters").asBool();
  const size_t nSpec = m_nPeaks.size();
  // Get intensity scaling parameter "IntensityScaling" + std::to_string(iSpec)
  // using an index instead of a name for performance reasons
  double intensityScaling = getParameter(m_nOwnParams - nSpec + iSpec);

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
  for (size_t i = 0; i < maxNPeaks; ++i) {
    auto fun = API::FunctionFactory::Instance().createFunction(peakShape);
    auto peak = boost::dynamic_pointer_cast<API::IPeakFunction>(fun);
    if (!peak) {
      throw std::runtime_error("A peak function is expected.");
    }
    if (i < nPeaks) {
      peak->fixCentre();
      peak->fixIntensity();
      peak->setCentre(eExcitations.get(i));
      peak->setIntensity(iExcitations.get(i) * intensityScaling);
      peak->setFwhm(fwhm);
      if (fixAll) {
        peak->fixAll();
      }
    } else {
      peak->setHeight(0.0);
      peak->fixAll();
    }
    spectrum->addFunction(peak);
  }
  return IFunction_sptr(spectrum);
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
  int nre = 0;
  auto &peakCalculator = dynamic_cast<Peaks &>(*m_source);
  peakCalculator.calculateEigenSystem(en, wf, nre);

  auto &fun = dynamic_cast<MultiDomainFunction &>(*m_target);
  auto temperatures = getAttribute("Temperatures").asVector();
  for (size_t i = 0; i < temperatures.size(); ++i) {
    updateSpectrum(*fun.getFunction(i), nre, en, wf, temperatures[i], i);
  }
}

/// Update a function for a single spectrum.
void CrystalFieldMultiSpectrum::updateSpectrum(
    API::IFunction &spectrum, int nre, const DoubleFortranVector &en,
    const ComplexFortranMatrix &wf, double temperature, size_t iSpec) const {
  DoubleFortranVector eExcitations;
  DoubleFortranVector iExcitations;
  calcExcitations(nre, en, wf, temperature, eExcitations, iExcitations);
  size_t nGoodPeaks = eExcitations.size();
  size_t nOriginalPeaks = m_nPeaks[iSpec];
  size_t maxNPeaks = m_maxNPeaks[iSpec];

  auto &composite = dynamic_cast<CompositeFunction &>(spectrum);
  if (maxNPeaks + 1 != composite.nFunctions()) {
    throw std::logic_error(
        "CrystalFieldMultiSpectrum target function size mismatch.");
  }
  const size_t nSpec = m_nPeaks.size();
  // Get intensity scaling parameter "IntensityScaling" + std::to_string(iSpec)
  // using an index instead of a name for performance reasons
  double intensityScaling = getParameter(m_nOwnParams - nSpec + iSpec);

  for (size_t i = 0; i < maxNPeaks; ++i) {
    auto fun = composite.getFunction(i + 1);
    auto &peak = dynamic_cast<API::IPeakFunction &>(*fun);
    if (i < nGoodPeaks) {
      peak.setCentre(eExcitations.get(i));
      peak.setIntensity(iExcitations.get(i) * intensityScaling);
    } else {
      peak.setHeight(0.0);
      if (i > nOriginalPeaks) {
        peak.fixAll();
      }
    }
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
