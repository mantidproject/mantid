// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHM_APODIZATIONFUNCTIONS_H_
#define MANTID_ALGORITHM_APODIZATIONFUNCTIONS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

/*
Contains the functions for calculating apodization,
which can be applied to data prior to a FFT.

@author Anthony Lim
@date 10/08/2017
*/

namespace Mantid {
namespace Algorithms {
namespace ApodizationFunctions {

double lorentz(double time, double decayConstant);
double gaussian(const double time, const double decayConstant);
double none(const double, const double);
} // namespace ApodizationFunctions
} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_APODIZATIONFUNCTIONS_H_*/
