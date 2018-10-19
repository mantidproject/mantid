// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_UNITLABELTYPES_H_
#define MANTID_KERNEL_UNITLABELTYPES_H_

#include "MantidKernel/DllConfig.h"

namespace Mantid {
namespace Kernel {
// Forward declare
class UnitLabel;
namespace Units {
/**
 * A simple class containing common symbol types
 */
class MANTID_KERNEL_DLL Symbol {
public:
  /// Empty label
  static const UnitLabel EmptyLabel;
  /// Second
  static const UnitLabel Second;
  /// Microsecond
  static const UnitLabel Microsecond;
  /// Nanosecond
  static const UnitLabel Nanosecond;
  /// Angstrom
  static const UnitLabel Angstrom;
  /// InverseAngstrom
  static const UnitLabel InverseAngstrom;
  /// InverseAngstromSq
  static const UnitLabel InverseAngstromSq;
  /// MilliElectronVolts
  static const UnitLabel MilliElectronVolts;
  /// GHz
  static const UnitLabel GHz;
  /// Metre
  static const UnitLabel Metre;
  /// Nanometre
  static const UnitLabel Nanometre;
  /// Inverse centimeters
  static const UnitLabel InverseCM;
  /// Reciprocal lattice units
  static const UnitLabel RLU;
};

} // namespace Units
} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_UNITLABELTYPES_H_ */
