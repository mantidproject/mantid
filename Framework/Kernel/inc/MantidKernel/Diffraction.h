// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_DIFFRACTION_H_
#define MANTID_KERNEL_DIFFRACTION_H_

#include "MantidKernel/DllConfig.h"
#include <functional>

namespace Mantid {
namespace Kernel {
namespace Diffraction {

/** Diffraction : Collection of functions useful in diffraction scattering
*/

MANTID_KERNEL_DLL double calcTofMin(const double difc, const double difa,
                                    const double tzero,
                                    const double tofmin = 0.);

MANTID_KERNEL_DLL double calcTofMax(const double difc, const double difa,
                                    const double tzero, const double tofmax);

MANTID_KERNEL_DLL std::function<double(double)>
getTofToDConversionFunc(const double difc, const double difa,
                        const double tzero);

MANTID_KERNEL_DLL std::function<double(double)>
getDToTofConversionFunc(const double difc, const double difa,
                        const double tzero);

} // namespace Diffraction
} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_DIFFRACTION_H_ */
