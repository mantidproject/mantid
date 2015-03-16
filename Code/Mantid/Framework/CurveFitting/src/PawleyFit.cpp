#include "MantidCurveFitting/PawleyFit.h"

#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace CurveFitting {

using namespace API;
using namespace Kernel;

void PawleyFit::init() {
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "",
                                                         Direction::Input),
                  "Input workspace that contains the spectrum on which to "
                  "perform the Pawley fit.");

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

  //  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace",
  // "",
  //                                                         Direction::Output),
  //                  "Workspace that contains measured spectrum, calculated "
  //                  "spectrum and difference curve.");
}

void exec() {

}

} // namespace CurveFitting
} // namespace Mantid
