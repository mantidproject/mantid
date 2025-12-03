// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Spline.h"
#include "MantidKernel/DllConfig.h"

namespace Mantid::Kernel {

EXTERN_MANTID_KERNEL template class MANTID_KERNEL_DLL CubicSpline<double, double>;
EXTERN_MANTID_KERNEL template class MANTID_KERNEL_DLL LinearSpline<double, double>;

} // namespace Mantid::Kernel
