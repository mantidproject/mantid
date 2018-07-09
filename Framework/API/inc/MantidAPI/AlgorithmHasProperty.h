#ifndef MANTID_API_ALGORITHMHASPROPERTY_H_
#define MANTID_API_ALGORITHMHASPROPERTY_H_

#include "MantidAPI/DllConfig.h"
#include "MantidKernel/TypedValidator.h"

namespace Mantid {
namespace API {

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
  AlgorithmHasProperty(const std::string &propName);
  std::string getType() const;
  Kernel::IValidator_sptr clone() const override;

protected:
  std::string
  checkValidity(const boost::shared_ptr<IAlgorithm> &value) const override;

private:
  /// Store the property name
  std::string m_propName;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_ALGORITHMHASPROPERTY_H_ */
