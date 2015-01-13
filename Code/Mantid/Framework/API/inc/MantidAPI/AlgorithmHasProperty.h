#ifndef MANTID_API_ALGORITHMHASPROPERTY_H_
#define MANTID_API_ALGORITHMHASPROPERTY_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/TypedValidator.h"

namespace Mantid {
namespace API {

//------------------------------------------------------------------------------
// Forward declaration
//------------------------------------------------------------------------------
class IAlgorithm;

/**
  A validator to check whether a given algorithm has a named property.
  The algorithm's property must be valid for the validator to pass.

  @author Martyn Gigg, Tessella plc
  @date 30/03/2011

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_API_DLL AlgorithmHasProperty
    : public Kernel::TypedValidator<boost::shared_ptr<IAlgorithm>> {
public:
  /// Constructor
  AlgorithmHasProperty(const std::string &propName);
  /// Destructor
  ~AlgorithmHasProperty();
  /**
   * Get a string representation of the type
   * @returns A string containing the validator type
   */
  inline std::string getType() const { return "AlgorithmHasProperty"; }
  /// Make a copy of the present type of validator
  inline Kernel::IValidator_sptr clone() const {
    return boost::make_shared<AlgorithmHasProperty>(*this);
  }

protected:
  /**
   * Checks the value based on the validator's rules
   * @param value :: The input algorithm to check
   * @returns An error message to display to users or an empty string on no
   * error
   */
  virtual std::string
  checkValidity(const boost::shared_ptr<IAlgorithm> &value) const;

private:
  /// Default constructor
  AlgorithmHasProperty();

  /// Store the property name
  std::string m_propName;
};

} // namespace Mantid
} // namespace API

#endif /* MANTID_API_ALGORITHMHASPROPERTY_H_ */
