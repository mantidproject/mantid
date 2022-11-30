// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/EigenFortranDefs.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

void MANTID_CURVEFITTING_DLL calculateEigensystem(DoubleFortranVector &eigenvalues, ComplexFortranMatrix &eigenvectors,
                                                  ComplexFortranMatrix &hamiltonian, ComplexFortranMatrix &hzeeman,
                                                  int nre, const DoubleFortranVector &bmol,
                                                  const DoubleFortranVector &bext, const ComplexFortranMatrix &bkq,
                                                  double alpha_euler = 0.0, double beta_euler = 0.0,
                                                  double gamma_euler = 0.0);

inline void MANTID_CURVEFITTING_DLL calculateEigensystem(
    DoubleFortranVector &eigenvalues, ComplexFortranMatrix &eigenvectors, ComplexFortranMatrix &hamiltonian, int nre,
    const DoubleFortranVector &bmol, const DoubleFortranVector &bext, const ComplexFortranMatrix &bkq,
    double alpha_euler = 0.0, double beta_euler = 0.0, double gamma_euler = 0.0) {
  ComplexFortranMatrix hzeeman;
  calculateEigensystem(eigenvalues, eigenvectors, hamiltonian, hzeeman, nre, bmol, bext, bkq, alpha_euler, beta_euler,
                       gamma_euler);
}

void MANTID_CURVEFITTING_DLL calculateZeemanEigensystem(DoubleFortranVector &eigenvalues,
                                                        ComplexFortranMatrix &eigenvectors,
                                                        const ComplexFortranMatrix &hamiltonian, int nre,
                                                        const DoubleFortranVector &bext);

void MANTID_CURVEFITTING_DLL calculateIntensities(int nre, const DoubleFortranVector &energies,
                                                  const ComplexFortranMatrix &wavefunctions, double temperature,
                                                  double de, IntFortranVector &degeneration,
                                                  DoubleFortranVector &e_energies, DoubleFortranMatrix &i_energies);

void MANTID_CURVEFITTING_DLL calculateExcitations(const DoubleFortranVector &e_energies,
                                                  const DoubleFortranMatrix &i_energies, double de, double di,
                                                  DoubleFortranVector &e_excitations,
                                                  DoubleFortranVector &i_excitations);

void MANTID_CURVEFITTING_DLL calculateMagneticMoment(const ComplexFortranMatrix &ev, const DoubleFortranVector &Hmag,
                                                     const int nre, DoubleFortranVector &moment);

void MANTID_CURVEFITTING_DLL calculateMagneticMomentMatrix(const ComplexFortranMatrix &ev,
                                                           const std::vector<double> &Hdir, const int nre,
                                                           ComplexFortranMatrix &mumat);

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
