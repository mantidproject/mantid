// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/DllConfig.h"
#include <string>

namespace Mantid {
namespace Kernel {
//-------------------------------------------------------------------------
// Forward declarations
//-------------------------------------------------------------------------
class Unit;

/**
 * A set of static helper methods to perform conversions between units
 */
class MANTID_KERNEL_DLL UnitConversion {
public:
  /// Convert a single value between the given units (as strings)
  static double run(const std::string &src, const std::string &dest, const double srcValue, const double l1,
                    const double l2, const double theta, const DeltaEMode::Type emode, const double efixed);
  /// Convert a single value between the given units
  static double run(Unit &srcUnit, Unit &destUnit, const double srcValue, const double l1, const double l2,
                    const double theta, const DeltaEMode::Type emode, const double efixed);

  /// Convert to ElasticQ from Energy
  static double convertToElasticQ(const double theta, const double efixed);

private:
  /// Perform a quick conversion
  static double convertQuickly(const double srcValue, const double factor, const double power);
  /// Convert through TOF
  static double convertViaTOF(Unit &srcUnit, Unit &destUnit, const double srcValue, const double l1, const double l2,
                              const double theta, const DeltaEMode::Type emode, const double efixed);
};

} // namespace Kernel
} // namespace Mantid
