#include "MantidCurveFitting/Functions/CrystalElectricField.h"
#include "MantidCurveFitting/Functions/CrystalFieldMultiSpectrum.h"
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
};
}

/// Constructor
CrystalFieldMultiSpectrum::CrystalFieldMultiSpectrum()
    : FunctionGenerator(IFunction_sptr(new Peaks)) {
  declareAttribute("Temperatures", Attribute(std::vector<double>(1, 1.0)));
  declareAttribute("Background", Attribute("FlatBackground"));
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
  for (size_t i = 0; i < temperatures.size(); ++i) {
    fun->addFunction(buildSpectrum(nre, en, wf, temperatures[i], fwhms[i]));
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
    double temperature, double fwhm) const {
  DoubleFortranVector eExcitations;
  DoubleFortranVector iExcitations;
  calcExcitations(nre, en, wf, temperature, eExcitations, iExcitations);
  size_t nPeaks = eExcitations.size();
  size_t maxNPeaks = nPeaks + nPeaks / 2 +
                     1; // m_source->getAttribute("MaxPeakCount").asInt();
  bool fixAll = getAttribute("FixAllPeakParameters").asBool();

  auto peakShape = IFunction::getAttribute("PeakShape").asString();
  auto bkgdShape = IFunction::getAttribute("Background").asString();

  auto spectrum = new CompositeFunction;
  auto background = API::FunctionFactory::Instance().createFunction(bkgdShape);
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
      peak->setIntensity(iExcitations.get(i));
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
    auto res =
        updateSpectrum(*fun.getFunction(i), nre, en, wf, temperatures[i]);
    if (!res) {
      buildTargetFunction();
      return;
    }
  }
}

/// Update a function for a single spectrum.
/// If returns false the target must be rebuilt with buldTargetFunction()
bool CrystalFieldMultiSpectrum::updateSpectrum(API::IFunction &spectrum,
                                               int nre,
                                               const DoubleFortranVector &en,
                                               const ComplexFortranMatrix &wf,
                                               double temperature) const {
  DoubleFortranVector eExcitations;
  DoubleFortranVector iExcitations;
  calcExcitations(nre, en, wf, temperature, eExcitations, iExcitations);
  size_t nPeaks = eExcitations.size();
  size_t maxNPeaks = nPeaks + nPeaks / 2 +
                     1; // m_source->getAttribute("MaxPeakCount").asInt();

  auto &composite = dynamic_cast<CompositeFunction &>(spectrum);
  if (maxNPeaks + 1 != composite.nFunctions()) {
    return false;
  }
  for (size_t i = 0; i < maxNPeaks; ++i) {
    auto fun = composite.getFunction(i + 1);
    auto &peak = dynamic_cast<API::IPeakFunction &>(*fun);
    if (i < nPeaks) {
      peak.setCentre(eExcitations.get(i));
      peak.setIntensity(iExcitations.get(i));
    } else {
      peak.setHeight(0.0);
      peak.fixAll();
    }
  }
  return true;
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
