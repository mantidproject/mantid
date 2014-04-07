#include "MantidKernel/UnitLabelTypes.h"

namespace Mantid { namespace Kernel
{
  namespace Units
  {
    // Empty label
    const UnitLabel Symbol::EmptyLabel("");
    // Second
    const UnitLabel Symbol::Second("s");
    // Microsecond
    const UnitLabel Symbol::Microsecond("microsecond", L"\u03bcs");
    /// Nanosecond
    const UnitLabel Symbol::Nanosecond("ns");
    // Angstrom
    const UnitLabel Symbol::Angstrom("Angstrom", L"\u212b");
    // InverseAngstrom
    const UnitLabel Symbol::InverseAngstrom("Angstrom^-1", L"\u212b\u207b\u00b9");
    // InverseAngstromSq
    const UnitLabel Symbol::InverseAngstromSq("Angstrom^-2", L"\u212b\u207b\u00b2");
    /// MilliElectronVolts
    const UnitLabel Symbol::MilliElectronVolts("meV");
    /// Metre
    const UnitLabel Symbol::Metre("m");
    /// Nanometre
    const UnitLabel Symbol::Nanometre("nm");
    /// Inverse centimeters
    const UnitLabel Symbol::InverseCM("cm^-1", L"cm\u207b\u00b9");

  } //namespace Units
}} // namespace Mantid::Kernel
