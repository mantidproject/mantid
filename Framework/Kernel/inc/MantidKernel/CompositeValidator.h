#ifndef MANTID_KERNEL_COMPOSITEVALIDATOR_H_
#define MANTID_KERNEL_COMPOSITEVALIDATOR_H_

#include "MantidKernel/IValidator.h"
#include <list>
#include <set>

namespace Mantid {
namespace Kernel {

//===============================================================================================
/** A composite validator that can combine any 2+ arbitrary validators together.

    @author Russell Taylor, Janik Zikovsky
    @date Aug 25, 2011

    Copyright &copy; 2008-2010 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

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
class DLLExport CompositeValidator : public IValidator {
public:
  /// Default constructor
  CompositeValidator();
  /// Destructor
  ~CompositeValidator();

  /// Gets the type of the validator
  std::string getType() const { return "composite"; }
  /// Return the instersection of allowed values from children
  std::vector<std::string> allowedValues() const;
  /// Clones this and the children into a new Validator
  IValidator_sptr clone() const;
  /// Adds a validator to the group of validators to check
  void add(IValidator_sptr child);
  /// Add a validator based on a template type. Useful for validators that need
  /// no arguments
  template <typename T> void add() { this->add(boost::make_shared<T>()); }
  /// Add a validator based on the first template type with the second as an
  /// argument.
  /// The argument is used to feed into the validator constructor
  template <typename T, typename U> void add(const U &arg) {
    this->add(boost::make_shared<T>(arg));
  }

private:
  /// Private Copy constructor: NO DIRECT COPY ALLOWED
  CompositeValidator(const CompositeValidator &);
  /// Verify the value with the child validators
  std::string check(const boost::any &value) const;
  /// A container for the child validators
  std::list<IValidator_sptr> m_children;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_COMPOSITEVALIDATOR_H_ */
