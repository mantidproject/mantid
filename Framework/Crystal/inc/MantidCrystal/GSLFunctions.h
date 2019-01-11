// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_GSLFUNCTIONS_H_
#define MANTID_CURVEFITTING_GSLFUNCTIONS_H_

namespace Mantid {
double gsl_costFunction(const gsl_vector *x, void *params);
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_GSLFUNCTIONS_H_*/
