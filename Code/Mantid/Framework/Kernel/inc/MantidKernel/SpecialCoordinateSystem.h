#ifndef MANTID_KERNEL_SpecialCoordinateSystem_H_
#define MANTID_KERNEL_SpecialCoordinateSystem_H_

namespace Mantid {
namespace Kernel {
/**
 * Special coordinate systems for Q3D.
 */
enum SpecialCoordinateSystem { None = 0, QLab = 1, QSample = 2, HKL = 3 }; // Do NOT alter existing values
}
}

#endif
