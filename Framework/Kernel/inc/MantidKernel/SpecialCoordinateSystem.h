// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_SpecialCoordinateSystem_H_
#define MANTID_KERNEL_SpecialCoordinateSystem_H_

namespace Mantid {
namespace Kernel {
/**
 * Special coordinate systems for Q3D.
 */
enum SpecialCoordinateSystem {
  None = 0,
  QLab = 1,
  QSample = 2,
  HKL = 3
}; // Do NOT alter existing values
} // namespace Kernel
} // namespace Mantid

#endif
