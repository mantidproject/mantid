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

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
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
