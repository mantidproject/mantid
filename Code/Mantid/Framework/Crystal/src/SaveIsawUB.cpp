#include "MantidAPI/FileProperty.h"
#include "MantidCrystal/SaveIsawUB.h"
#include <fstream>
#include "MantidAPI/IMDEventWorkspace.h"

using Mantid::Kernel::DblMatrix;
using Mantid::Geometry::UnitCell;
using Mantid::Geometry::OrientedLattice;

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveIsawUB)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace std;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SaveIsawUB::SaveIsawUB() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SaveIsawUB::~SaveIsawUB() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SaveIsawUB::init() {
  declareProperty(
      new WorkspaceProperty<Workspace>("InputWorkspace", "", Direction::Input),
      "An input workspace containing the orientation matrix.");

  std::vector<std::string> exts;
  exts.push_back(".mat");
  exts.push_back(".ub");
  exts.push_back(".txt");

  declareProperty(new FileProperty("Filename", "", FileProperty::Save, exts),
                  "Path to an ISAW-style UB matrix text file.");
}

double SaveIsawUB::getErrorVolume(const OrientedLattice &lattice) {
  double Volume;
  double latticeParams[6] = {lattice.a(),     lattice.b(),    lattice.c(),
                             lattice.alpha(), lattice.beta(), lattice.gamma()};
  double lattice_errors[6] = {lattice.errora(),    lattice.errorb(),
                              lattice.errorc(),    lattice.erroralpha(),
                              lattice.errorbeta(), lattice.errorgamma()};
  if (lattice.volume() <= 0) {

    double xA = cos(lattice.alpha() / 180. * M_PI);
    double xB = cos(lattice.beta() / 180. * M_PI);
    double xC = cos(lattice.gamma() / 180. * M_PI);
    Volume = lattice.a() * lattice.b() * lattice.c() *
             sqrt(1 - xA * xA - xB * xB - xC * xC + 2 * xA * xB * xC);
  } else
    Volume = lattice.volume();

  double dV = 0;
  for (int i = 0; i < 3; i++) {
    double U = (Volume / latticeParams[i] * lattice_errors[i]);
    dV += U * U;
  }

  double U = (lattice_errors[3]) * (sin(2 * latticeParams[3] / 180. * M_PI) -
                                    sin(latticeParams[3] / 180. * M_PI) *
                                        cos(latticeParams[4] / 180 * M_PI) *
                                        cos(latticeParams[5] / 180 * M_PI));
  dV += U * U;
  U = (lattice_errors[4]) * (sin(2 * latticeParams[4] / 180. * M_PI) -
                             sin(latticeParams[4] / 180. * M_PI) *
                                 cos(latticeParams[3] / 180 * M_PI) *
                                 cos(latticeParams[5] / 180 * M_PI));
  dV += U * U;
  U = (lattice_errors[5]) * (sin(2 * latticeParams[5] / 180. * M_PI) -
                             sin(latticeParams[5] / 180. * M_PI) *
                                 cos(latticeParams[4] / 180 * M_PI) *
                                 cos(latticeParams[3] / 180 * M_PI));
  dV += U * U;
  dV = sqrt(dV);

  return dV;
}
//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveIsawUB::exec() {
  try {
    Workspace_sptr ws1 = getProperty("InputWorkspace");
    ExperimentInfo_sptr ws;
    IMDEventWorkspace_sptr MDWS =
        boost::dynamic_pointer_cast<IMDEventWorkspace>(ws1);
    if (MDWS != NULL) {
      ws = MDWS->getExperimentInfo(0);
    } else {
      ws = boost::dynamic_pointer_cast<ExperimentInfo>(ws1);
    }

    if (!ws)
      throw std::invalid_argument("Must specify either a MatrixWorkspace or a "
                                  "PeaksWorkspace or a MDEventWorkspace.");

    if (!ws->sample().hasOrientedLattice())
      throw std::invalid_argument(
          "Workspace must have an oriented lattice to save");

    std::string Filename = getProperty("Filename");

    ofstream out;
    out.open(Filename.c_str());

    OrientedLattice lattice = ws->sample().getOrientedLattice();
    Kernel::DblMatrix ub = lattice.getUB();

    // Write the ISAW UB matrix
    const int beam = 2;
    const int up = 1;
    const int back = 0;
    out << fixed;

    for (size_t basis = 0; basis < 3; basis++) {
      out << setw(11) << setprecision(8) << ub[beam][basis] << setw(12)
          << setprecision(8) << ub[back][basis] << setw(12) << setprecision(8)
          << ub[up][basis] << " " << endl;
    }

    out << setw(11) << setprecision(4) << lattice.a() << setw(12)
        << setprecision(4) << lattice.b() << setw(12) << setprecision(4)
        << lattice.c() << setw(12) << setprecision(4) << lattice.alpha()
        << setw(12) << setprecision(4) << lattice.beta() << setw(12)
        << setprecision(4) << lattice.gamma() << setw(12) << setprecision(4)
        << lattice.volume() << " " << endl;
    double ErrorVolume = getErrorVolume(lattice);
    out << setw(11) << setprecision(4) << lattice.errora() << setw(12)
        << setprecision(4) << lattice.errorb() << setw(12) << setprecision(4)
        << lattice.errorc() << setw(12) << setprecision(4)
        << lattice.erroralpha() << setw(12) << setprecision(4)
        << lattice.errorbeta() << setw(12) << setprecision(4)
        << lattice.errorgamma() << setw(12) << setprecision(4) << ErrorVolume
        << " " << endl;

    out << endl << endl;

    out << "The above matrix is the Transpose of the UB Matrix. ";
    out << "The UB matrix maps the column" << endl;
    out << "vector (h,k,l ) to the column vector ";
    out << "(q'x,q'y,q'z)." << endl;
    out << "|Q'|=1/dspacing and its coordinates are a ";
    out << "right-hand coordinate system where" << endl;
    out << " x is the beam direction and z is vertically ";
    out << "upward.(IPNS convention)" << endl;

    out.close();

  } catch (exception &s) {
    throw std::invalid_argument(s.what());
  }
}

} // namespace Mantid
} // namespace Crystal
