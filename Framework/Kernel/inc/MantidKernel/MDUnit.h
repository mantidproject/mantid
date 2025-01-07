// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/UnitLabel.h"
#include <memory>
#include <string>

namespace Mantid {
namespace Kernel {

/** MDUnit : Unit type for multidimensional data types. Not convertable to/from
  TOF unlike Unit.
*/
class MANTID_KERNEL_DLL MDUnit {
public:
  virtual UnitLabel getUnitLabel() const = 0;
  virtual bool canConvertTo(const MDUnit &other) const = 0;
  virtual bool isQUnit() const = 0;
  virtual MDUnit *clone() const = 0;
  bool operator==(const MDUnit &other) const;
  virtual ~MDUnit() = default;
};

/// QUnit base
class MANTID_KERNEL_DLL QUnit : public MDUnit {
public:
  bool isQUnit() const override;
};

/// Dimensionless RLU
class MANTID_KERNEL_DLL ReciprocalLatticeUnit : public QUnit {
public:
  ReciprocalLatticeUnit();
  ReciprocalLatticeUnit(UnitLabel unitLabel);
  UnitLabel getUnitLabel() const override;
  bool canConvertTo(const MDUnit &other) const override;
  ReciprocalLatticeUnit *clone() const override;

private:
  bool isSpecialRLUUnitLabel() const;
  UnitLabel m_unitLabel;
};

/// Inverse Angstroms unit
class MANTID_KERNEL_DLL InverseAngstromsUnit : public QUnit {
public:
  UnitLabel getUnitLabel() const override;
  bool canConvertTo(const MDUnit &other) const override;
  InverseAngstromsUnit *clone() const override;
};

class MANTID_KERNEL_DLL LabelUnit : public MDUnit {
private:
  UnitLabel m_unitLabel;

public:
  LabelUnit(UnitLabel unitLabel);
  UnitLabel getUnitLabel() const override;
  bool canConvertTo(const MDUnit &other) const override;
  bool isQUnit() const override;
  LabelUnit *clone() const override;
};

using MDUnit_uptr = std::unique_ptr<MDUnit>;
using MDUnit_const_uptr = std::unique_ptr<const MDUnit>;

} // namespace Kernel
} // namespace Mantid
