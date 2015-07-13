#ifndef MANTID_KERNEL_MDUNIT_H_
#define MANTID_KERNEL_MDUNIT_H_

#include "MantidKernel/System.h"
#include "MantidKernel/UnitLabel.h"
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
  MDUnit();
  virtual UnitLabel getUnitLabel() const = 0;
  virtual bool canConvertTo(MDUnit &other) const = 0;
  virtual bool isQUnit() const = 0;
  virtual ~MDUnit();
};

/// QUnit base
class DLLExport QUnit : public MDUnit {
public:
  virtual ~QUnit();
  bool isQUnit() const;
};

/// Dimensionless RLU
class DLLExport ReciprocalLatticeUnit : public QUnit {
public:
  UnitLabel getUnitLabel() const;
  bool canConvertTo(MDUnit &other) const;
  virtual ~ReciprocalLatticeUnit();
};

/// Inverse Angstroms unit
class DLLExport InverseAngstromsUnit : public QUnit {
public:
  UnitLabel getUnitLabel() const;
  bool canConvertTo(MDUnit &other) const;
  virtual ~InverseAngstromsUnit();
};

class DLLExport LabelUnit : public MDUnit {
private:
  std::string m_unitLabel;
public:
  LabelUnit(const std::string& unitLabel);
  UnitLabel getUnitLabel() const;
  bool canConvertTo(MDUnit &other) const;
  bool isQUnit() const;
  virtual ~LabelUnit();
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_MDUNIT_H_ */
