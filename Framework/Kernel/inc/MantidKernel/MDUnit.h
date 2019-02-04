// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_MDUNIT_H_
#define MANTID_KERNEL_MDUNIT_H_

#include "MantidKernel/System.h"
#include "MantidKernel/UnitLabel.h"
#include <memory>
#include <string>

namespace Mantid {
namespace Kernel {

/** MDUnit : Unit type for multidimensional data types. Not convertable to/from
  TOF unlike Unit.
*/
class DLLExport MDUnit {
public:
  virtual UnitLabel getUnitLabel() const = 0;
  virtual bool canConvertTo(const MDUnit &other) const = 0;
  virtual bool isQUnit() const = 0;
  virtual MDUnit *clone() const = 0;
  bool operator==(const MDUnit &other) const;
  virtual ~MDUnit() = default;
};

/// QUnit base
class DLLExport QUnit : public MDUnit {
public:
  bool isQUnit() const override;
};

/// Dimensionless RLU
class DLLExport ReciprocalLatticeUnit : public QUnit {
public:
  ReciprocalLatticeUnit();
  ReciprocalLatticeUnit(const UnitLabel &unitLabel);
  UnitLabel getUnitLabel() const override;
  bool canConvertTo(const MDUnit &other) const override;
  ReciprocalLatticeUnit *clone() const override;

private:
  bool isSpecialRLUUnitLabel() const;
  UnitLabel m_unitLabel;
};

/// Inverse Angstroms unit
class DLLExport InverseAngstromsUnit : public QUnit {
public:
  UnitLabel getUnitLabel() const override;
  bool canConvertTo(const MDUnit &other) const override;
  InverseAngstromsUnit *clone() const override;
};

class DLLExport LabelUnit : public MDUnit {
private:
  UnitLabel m_unitLabel;

public:
  LabelUnit(const UnitLabel &unitLabel);
  UnitLabel getUnitLabel() const override;
  bool canConvertTo(const MDUnit &other) const override;
  bool isQUnit() const override;
  LabelUnit *clone() const override;
};

using MDUnit_uptr = std::unique_ptr<MDUnit>;
using MDUnit_const_uptr = std::unique_ptr<const MDUnit>;

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_MDUNIT_H_ */
