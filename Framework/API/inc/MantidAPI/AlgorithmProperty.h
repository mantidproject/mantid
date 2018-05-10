#ifndef MANTID_API_ALGORITHMPROPERTY_H_
#define MANTID_API_ALGORITHMPROPERTY_H_

#include "MantidAPI/DllConfig.h"
#include "MantidKernel/NullValidator.h"
#include "MantidKernel/PropertyWithValue.h"

namespace Mantid {
namespace API {
//-------------------------------------------------------------------------
// Forward declarations
//-------------------------------------------------------------------------
class IAlgorithm;

#ifdef _WIN32
#pragma warning(push)
// Disable 'multiple assignment operators specified' warning for this class
//  - it's not an accident that we have more than one
#pragma warning(disable : 4522)
#endif

/**
Define an algorithm property that can be used to supply an algorithm object
to a subsequent algorithm. It is a specialized version of PropertyWithValue
where the type a pointer to an object implementing the
API::IAlgorithm interface.

@author Martyn Gigg, Tessella Plc
@date 24/03/2011

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
class DLLExport AlgorithmProperty
    : public Kernel::PropertyWithValue<boost::shared_ptr<IAlgorithm>> {
public:
  /// Typedef the held type
  using HeldType = boost::shared_ptr<IAlgorithm>;

  /// Constructor
  AlgorithmProperty(const std::string &propName,
                    Kernel::IValidator_sptr validator =
                        Kernel::IValidator_sptr(new Kernel::NullValidator),
                    unsigned int direction = Kernel::Direction::Input);
  /// Copy constructor
  AlgorithmProperty(const AlgorithmProperty &rhs);
  // Unhide base class members (at minimum, avoids Intel compiler warning)
  using Kernel::PropertyWithValue<HeldType>::operator=;
  /// Copy-Assignment operator
  AlgorithmProperty &operator=(const AlgorithmProperty &rhs);
  /// 'Virtual copy constructor'
  inline AlgorithmProperty *clone() const override {
    return new AlgorithmProperty(*this);
  }

  /// Add the value of another property. Doesn't make sense here.
  AlgorithmProperty &operator+=(Kernel::Property const *) override {
    throw Kernel::Exception::NotImplementedError(
        "+= operator is not implemented for AlgorithmProperty.");
    return *this;
  }
  /// Return the algorithm as string
  std::string value() const override;
  /// Get the default
  std::string getDefault() const override;
  /// Sets the value of the algorithm
  std::string setValue(const std::string &value) override;

private:
  /// Default constructor
  AlgorithmProperty();

  /// The string used to create the underlying algorithm
  std::string m_algStr;
};

#ifdef _WIN32
#pragma warning(                                                               \
    pop) // Re-enable the warning about multiple assignment operators
#endif

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_ALGORITHMPROPERTY_H_ */
