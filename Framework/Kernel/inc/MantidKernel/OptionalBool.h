#ifndef MANTID_KERNEL_OPTIONALBOOL_H_
#define MANTID_KERNEL_OPTIONALBOOL_H_

#include "MantidKernel/DllConfig.h"
#include <iosfwd>
#include <map>
#include <string>

namespace Mantid {
namespace Kernel {

/** OptionalBool : Tri-state bool. Defaults to unset.

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
class MANTID_KERNEL_DLL OptionalBool {
public:
  enum Value { Unset, True, False };
  OptionalBool();
  OptionalBool(bool arg);
  OptionalBool(Value arg);
  virtual ~OptionalBool();
  OptionalBool(const OptionalBool &other);
  OptionalBool &operator=(const OptionalBool &other);
  bool operator==(const OptionalBool &other) const;
  Value getValue() const;

  static std::map<std::string, Value> strToEmumMap();
  static std::map<Value, std::string> enumToStrMap();
  const static std::string StrUnset;
  const static std::string StrFalse;
  const static std::string StrTrue;

  // or out-of-class friend (friend declaration inside class only)
  friend std::ostream &operator<<(std::ostream &os, OptionalBool const &object);
  friend std::istream &operator>>(std::istream &istream, OptionalBool &object);

private:
  Value m_arg;
};

std::ostream &operator<<(std::ostream &os, OptionalBool const &object);

std::istream &operator>>(std::istream &istream, OptionalBool &object);

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_OPTIONALBOOL_H_ */
