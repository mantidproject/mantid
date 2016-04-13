#ifndef MANTID_CURVEFITTING_CRYSTALELECTRICFIELD_H_
#define MANTID_CURVEFITTING_CRYSTALELECTRICFIELD_H_

#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/FortranDefs.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

void MANTID_CURVEFITTING_DLL
calculateEigesystem(DoubleFortranVector &eigenvalues,
                    ComplexFortranMatrix &eigenvectors,
                    ComplexFortranMatrix &hamiltonian, int nre,
                    const DoubleFortranVector &bmol,
                    const DoubleFortranVector &bext,
                    const ComplexFortranMatrix &bkq, double alpha_euler = 0.0,
                    double beta_euler = 0.0, double gamma_euler = 0.0);

void MANTID_CURVEFITTING_DLL
calculateIntensities(int nre, const DoubleFortranVector &energies,
                     const ComplexFortranMatrix &wavefunctions,
                     double temperature, double de,
                     IntFortranVector &degeneration,
                     DoubleFortranVector &e_energies,
                     DoubleFortranMatrix &i_energies);

void MANTID_CURVEFITTING_DLL
calculateExcitations(const DoubleFortranVector &e_energies,
                     const DoubleFortranMatrix &i_energies, double de,
                     double di, DoubleFortranVector &e_excitations,
                     DoubleFortranVector &i_excitations);

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_CRYSTALELECTRICFIELD_H_*/
