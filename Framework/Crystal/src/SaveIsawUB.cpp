#include "MantidCrystal/SaveIsawUB.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Sample.h"

#include <fstream>
#include <iomanip>

using Mantid::Geometry::OrientedLattice;
using Mantid::Kernel::DblMatrix;

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveIsawUB)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace std;

/** Initialize the algorithm's properties.
 */
void SaveIsawUB::init() {
  declareProperty(make_unique<WorkspaceProperty<Workspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input workspace containing the orientation matrix.");

  const std::vector<std::string> exts{".mat", ".ub", ".txt"};
  declareProperty(Kernel::make_unique<FileProperty>("Filename", "",
                                                    FileProperty::Save, exts),
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

/** Execute the algorithm.
 */
void SaveIsawUB::exec() {
  try {
    Workspace_sptr ws1 = getProperty("InputWorkspace");
    ExperimentInfo_sptr ws;
    MultipleExperimentInfos_sptr MDWS =
        boost::dynamic_pointer_cast<MultipleExperimentInfos>(ws1);
    if (MDWS != nullptr) {
      ws = MDWS->getExperimentInfo(0);
    } else {
      ws = boost::dynamic_pointer_cast<ExperimentInfo>(ws1);
    }

    if (!ws)
      throw std::invalid_argument("Must specify either a MatrixWorkspace or a "
                                  "PeaksWorkspace or a MDWorkspace.");

    if (!ws->sample().hasOrientedLattice())
      throw std::invalid_argument(
          "Workspace must have an oriented lattice to save");

    std::string Filename = getProperty("Filename");

    ofstream out;
    out.open(Filename.c_str());

    OrientedLattice lattice = ws->sample().getOrientedLattice();
    Kernel::DblMatrix ub = lattice.getUB();
    Kernel::DblMatrix modub = lattice.getModUB();

    // Write the ISAW UB matrix
    const int beam = 2;
    const int up = 1;
    const int back = 0;
    out << fixed;

    for (size_t basis = 0; basis < 3; basis++) {
      out << setw(11) << setprecision(8) << ub[beam][basis] << setw(12)
          << setprecision(8) << ub[back][basis] << setw(12) << setprecision(8)
          << ub[up][basis] << " \n";
    }

    int ModDim = 0;
    for (int i = 0; i < 3; i++) {
      if (lattice.getModVec(i + 1) == V3D(0, 0, 0))
        continue;
      else
        ModDim++;
    }

    if (ModDim > 0) {
      out << "ModUB: \n";
      for (size_t basis = 0; basis < 3; basis++) {
        out << setw(11) << setprecision(8) << modub[beam][basis] << setw(12)
            << setprecision(8) << modub[back][basis] << setw(12)
            << setprecision(8) << modub[up][basis] << " \n";
      }
    }

    //                out << "Lattice Parameters: \n";
    out << setw(11) << setprecision(4) << lattice.a() << setw(12)
        << setprecision(4) << lattice.b() << setw(12) << setprecision(4)
        << lattice.c() << setw(12) << setprecision(4) << lattice.alpha()
        << setw(12) << setprecision(4) << lattice.beta() << setw(12)
        << setprecision(4) << lattice.gamma() << setw(12) << setprecision(4)
        << lattice.volume() << " \n";
    double ErrorVolume = getErrorVolume(lattice);
    out << setw(11) << setprecision(4) << lattice.errora() << setw(12)
        << setprecision(4) << lattice.errorb() << setw(12) << setprecision(4)
        << lattice.errorc() << setw(12) << setprecision(4)
        << lattice.erroralpha() << setw(12) << setprecision(4)
        << lattice.errorbeta() << setw(12) << setprecision(4)
        << lattice.errorgamma() << setw(12) << setprecision(4) << ErrorVolume
        << " \n";

    out << "\n";
    if (ModDim == 1) {
      out << "Modulation Vector 1:   " << setw(12) << setprecision(4)
          << lattice.getdh(1) << setw(12) << setprecision(4) << lattice.getdk(1)
          << setw(12) << setprecision(4) << lattice.getdl(1) << " \n";

      out << "Modulation Vector 1 error:   " << setw(6) << setprecision(4)
          << lattice.getdherr(1) << setw(12) << setprecision(4)
          << lattice.getdkerr(1) << setw(12) << setprecision(4)
          << lattice.getdlerr(1) << " \n\n";

      out << "Max Order:        " << lattice.getMaxOrder() << " \n";
      out << "Cross Terms:      " << lattice.getCrossTerm() << " \n";
    }
    if (ModDim == 2) {
      out << "Modulation Vector 1:   " << setw(12) << setprecision(4)
          << lattice.getdh(1) << setw(12) << setprecision(4) << lattice.getdk(1)
          << setw(12) << setprecision(4) << lattice.getdl(1) << " \n";

      out << "Modulation Vector 1 error:   " << setw(6) << setprecision(4)
          << lattice.getdherr(1) << setw(12) << setprecision(4)
          << lattice.getdkerr(1) << setw(12) << setprecision(4)
          << lattice.getdlerr(1) << " \n";

      out << "Modulation Vector 2:   " << setw(12) << setprecision(4)
          << lattice.getdh(2) << setw(12) << setprecision(4) << lattice.getdk(2)
          << setw(12) << setprecision(4) << lattice.getdl(2) << " \n";

      out << "Modulation Vector 2 error:   " << setw(6) << setprecision(4)
          << lattice.getdherr(2) << setw(12) << setprecision(4)
          << lattice.getdkerr(2) << setw(12) << setprecision(4)
          << lattice.getdlerr(2) << " \n\n";

      out << "Max Order:        " << lattice.getMaxOrder() << " \n";
      out << "Cross Terms:      " << lattice.getCrossTerm() << " \n";
    }
    if (ModDim == 3) {
      out << "Modulation Vector 1:   " << setw(12) << setprecision(4)
          << lattice.getdh(1) << setw(12) << setprecision(4) << lattice.getdk(1)
          << setw(12) << setprecision(4) << lattice.getdl(1) << " \n";

      out << "Modulation Vector 1 error:   " << setw(6) << setprecision(4)
          << lattice.getdherr(1) << setw(12) << setprecision(4)
          << lattice.getdkerr(1) << setw(12) << setprecision(4)
          << lattice.getdlerr(1) << " \n";

      out << "Modulation Vector 2:   " << setw(12) << setprecision(4)
          << lattice.getdh(2) << setw(12) << setprecision(4) << lattice.getdk(2)
          << setw(12) << setprecision(4) << lattice.getdl(2) << " \n";

      out << "Modulation Vector 2 error:   " << setw(6) << setprecision(4)
          << lattice.getdherr(2) << setw(12) << setprecision(4)
          << lattice.getdkerr(2) << setw(12) << setprecision(4)
          << lattice.getdlerr(2) << " \n";

      out << "Modulation Vector 2:   " << setw(12) << setprecision(4)
          << lattice.getdh(3) << setw(12) << setprecision(4) << lattice.getdk(3)
          << setw(12) << setprecision(4) << lattice.getdl(3) << " \n";

      out << "Modulation Vector 2 error:   " << setw(6) << setprecision(4)
          << lattice.getdherr(3) << setw(12) << setprecision(4)
          << lattice.getdkerr(3) << setw(12) << setprecision(4)
          << lattice.getdlerr(3) << " \n\n";

      out << "Max Order:        " << lattice.getMaxOrder() << " \n";
      out << "Cross Terms:      " << lattice.getCrossTerm() << " \n";
    }

    out << "\n\n";

    out << "The above matrix is the Transpose of the UB Matrix. ";
    out << "The UB matrix maps the column\n";
    out << "vector (h,k,l ) to the column vector ";
    out << "(q'x,q'y,q'z).\n";
    out << "|Q'|=1/dspacing and its coordinates are a ";
    out << "right-hand coordinate system where\n";
    out << " x is the beam direction and z is vertically ";
    out << "upward.(IPNS convention)\n";

    out.close();

  } catch (exception &s) {
    throw std::invalid_argument(s.what());
  }
}

} // namespace Crystal
} // namespace Mantid
