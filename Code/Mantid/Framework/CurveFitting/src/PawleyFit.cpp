#include "MantidCurveFitting/PawleyFit.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/PawleyFunction.h"

#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidKernel/ListValidator.h"

#include <algorithm>

namespace Mantid {
namespace CurveFitting {

using namespace API;
using namespace Kernel;
using namespace Geometry;

DECLARE_ALGORITHM(PawleyFit);

const std::string PawleyFit::summary() const {
  return "This algorithm performs a Pawley-fit on the supplied workspace.";
}

std::vector<V3D> PawleyFit::hklsFromString(const std::string &hklString) const {
  std::vector<std::string> hklStrings;
  boost::split(hklStrings, hklString, boost::is_any_of(";"));

  std::vector<V3D> hkls(hklStrings.size());
  for (size_t i = 0; i < hkls.size(); ++i) {
    std::istringstream strm(hklStrings[i]);
    strm >> hkls[i];
  }

  return hkls;
}

void PawleyFit::init() {
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "",
                                                         Direction::Input),
                  "Input workspace that contains the spectrum on which to "
                  "perform the Pawley fit.");

  declareProperty("WorkspaceIndex", 0,
                  "Spectrum on which the fit should be performed.");

  std::vector<std::string> crystalSystems;
  crystalSystems.push_back("Cubic");
  crystalSystems.push_back("Tetragonal");
  crystalSystems.push_back("Hexagonal");
  crystalSystems.push_back("Trigonal");
  crystalSystems.push_back("Orthorhombic");
  crystalSystems.push_back("Monoclinic");
  crystalSystems.push_back("Triclinic");

  auto crystalSystemValidator =
      boost::make_shared<StringListValidator>(crystalSystems);

  declareProperty("CrystalSystem", crystalSystems.back(),
                  crystalSystemValidator,
                  "Crystal system to use for refinement.");

  declareProperty("InitialCell", "1.0 1.0 1.0 90.0 90.0 90.0",
                  "Specification of initial unit cell, given as 'a, b, c, "
                  "alpha, beta, gamma'.");

  declareProperty("MillerIndices", "[1,0,0];[1,1,0]",
                  "Semi-colon separated list of Miller indices given in the "
                  "format '[h,k,l]'.");

  declareProperty("RefineZeroShift", false, "If checked, a zero-shift with the "
                                            "same unit as the spectrum is "
                                            "refined.");

  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "Workspace that contains measured spectrum, calculated "
                  "spectrum and difference curve.");
}

void PawleyFit::exec() {
  boost::shared_ptr<PawleyFunction> pawleyFn =
      boost::dynamic_pointer_cast<PawleyFunction>(
          FunctionFactory::Instance().createFunction("PawleyFunction"));

  bool refineZeroShift = getProperty("RefineZeroShift");
  if(!refineZeroShift) {
    pawleyFn->fix(pawleyFn->parameterIndex("f0.ZeroShift"));
  }

  pawleyFn->setProfileFunction("PseudoVoigt");
  pawleyFn->setCrystalSystem(getProperty("CrystalSystem"));
  pawleyFn->setUnitCell(getProperty("InitialCell"));

  std::vector<V3D> hkls = hklsFromString(getProperty("MillerIndices"));

  MatrixWorkspace_sptr ws = getProperty("InputWorkspace");
  int wsIndex = getProperty("WorkspaceIndex");

  const MantidVec &data = ws->readY(static_cast<size_t>(wsIndex));
  pawleyFn->setPeaks(hkls, 0.008, *(std::max_element(data.begin(), data.end())));

  Algorithm_sptr fit = createChildAlgorithm("Fit");
  fit->setProperty("Function", boost::static_pointer_cast<IFunction>(pawleyFn));
  fit->setProperty("InputWorkspace", ws);
  fit->setProperty("WorkspaceIndex", wsIndex);
  fit->setProperty("CreateOutput", true);

  fit->execute();

  for (size_t i = 0; i < pawleyFn->nParams(); ++i) {
    std::cout << i << " " << pawleyFn->parameterName(i) << " "
              << pawleyFn->getParameter(i) << " " << pawleyFn->getError(i)
              << std::endl;
  }

  MatrixWorkspace_sptr output = fit->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", output);
}

} // namespace CurveFitting
} // namespace Mantid
