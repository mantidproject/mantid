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
  void functionGeneral(const API::FunctionDomainGeneral &generalDomain,
                       API::FunctionValues &values) const override {
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
  declareAttribute("FWHM", Attribute(0.0));
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

  auto temperatures = getAttribute("Temperatures").asVector();
  for (size_t i = 0; i < temperatures.size(); ++i) {
    fun->addFunction(buildSpectrum(nre, en, wf, temperatures[i]));
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
API::IFunction_sptr
CrystalFieldMultiSpectrum::buildSpectrum(int nre, const DoubleFortranVector &en,
                                         const ComplexFortranMatrix &wf,
                                         double temperature) const {
  DoubleFortranVector eExcitations;
  DoubleFortranVector iExcitations;
  calcExcitations(nre, en, wf, temperature, eExcitations, iExcitations);
  size_t nPeaks = eExcitations.size();

  auto peakShape = IFunction::getAttribute("PeakShape").asString();
  auto bkgdShape = IFunction::getAttribute("Background").asString();
  auto fwhm = IFunction::getAttribute("FWHM").asDouble();

  auto spectrum = new CompositeFunction;
  auto background = API::FunctionFactory::Instance().createFunction(bkgdShape);
  spectrum->addFunction(background);
  for (size_t i = 0; i < nPeaks; ++i) {
    auto fun = API::FunctionFactory::Instance().createFunction(peakShape);
    auto peak = boost::dynamic_pointer_cast<API::IPeakFunction>(fun);
    if (!peak) {
      throw std::runtime_error("A peak function is expected.");
    }
    peak->fixCentre();
    peak->fixIntensity();
    peak->setCentre(eExcitations.get(i));
    peak->setIntensity(iExcitations.get(i));
    peak->setFwhm(fwhm);
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
    auto res = updateSpectrum(*fun.getFunction(i), nre, en, wf, temperatures[i]);
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

  auto &composite = dynamic_cast<CompositeFunction &>(spectrum);
  if (nPeaks + 1 != composite.nFunctions()) {
    return false;
  }
  for (size_t i = 0; i < nPeaks; ++i) {
    auto fun = composite.getFunction(i + 1);
    auto &peak = dynamic_cast<API::IPeakFunction&>(*fun);
    peak.setCentre(eExcitations.get(i));
    peak.setIntensity(iExcitations.get(i));
  }
  return true;
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
