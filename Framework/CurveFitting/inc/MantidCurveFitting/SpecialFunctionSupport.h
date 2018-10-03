// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SPECIALFUNCTIONSUPPORT_H_
#define SPECIALFUNCTIONSUPPORT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include <cmath>
#include <complex>
#include <functional>
#include <vector>

namespace Mantid {
namespace CurveFitting {
/*
    Special functions used as part of peak shape functions that are not
    included in the Gnu Scientific Lib or other C/C++ open source code projects

    @author Anders J Markvardsen, Rutherford Appleton Laboratory
    @date 30/09/2009
 */
namespace SpecialFunctionSupport {
/// Compute exp(z)*E1(z) where z is complex and E1(z) is the Exponential
/// Integral
std::complex<double>
    DLLExport exponentialIntegral(const std::complex<double> &z);

} // namespace SpecialFunctionSupport
} // namespace CurveFitting
} // namespace Mantid

#endif /*SPECIALFUNCTIONSUPPORT_H_*/
