#include "MantidKernel/UnitLabelTypes.h"

namespace Mantid {
namespace Kernel {
namespace Units {
// Empty label
const UnitLabel Symbol::EmptyLabel("");
// Second
const UnitLabel Symbol::Second("s");
// Microsecond
const UnitLabel Symbol::Microsecond("microsecond", L"\u03bcs", "\\mu s");
/// Nanosecond
const UnitLabel Symbol::Nanosecond("ns");
// Angstrom
const UnitLabel Symbol::Angstrom("Angstrom", L"\u212b", "\\AA");
// InverseAngstrom
const UnitLabel Symbol::InverseAngstrom("Angstrom^-1", L"\u212b\u207b\u00b9",
                                        "\\AA^{-1}");
// InverseAngstromSq
const UnitLabel Symbol::InverseAngstromSq("Angstrom^-2", L"\u212b\u207b\u00b2",
                                          "\\AA^{-2}");
/// MilliElectronVolts
const UnitLabel Symbol::MilliElectronVolts("meV");
/// Metre
const UnitLabel Symbol::Metre("m");
/// Nanometre
const UnitLabel Symbol::Nanometre("nm");
/// Inverse centimeters
const UnitLabel Symbol::InverseCM("cm^-1", L"cm\u207b\u00b9", "cm^{-1}");

} // namespace Units
}
} // namespace Mantid::Kernel
