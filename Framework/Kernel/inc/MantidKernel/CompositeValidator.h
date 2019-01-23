// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_COMPOSITEVALIDATOR_H_
#define MANTID_KERNEL_COMPOSITEVALIDATOR_H_

#include "MantidKernel/IValidator.h"
#include "MantidKernel/System.h"

#include <boost/make_shared.hpp>

#include <list>
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {

/** A composite validator that can combine any 2+ arbitrary validators together.

    @author Russell Taylor, Janik Zikovsky
    @date Aug 25, 2011
*/

enum class CompositeRelation { AND = 0, OR = 1 };

class DLLExport CompositeValidator : public IValidator {
public:
  /// Default constructor
  CompositeValidator(
      const CompositeRelation &relation = CompositeRelation::AND);
  /// Destructor
  ~CompositeValidator() override;

  /// Gets the type of the validator
  std::string getType() const { return "composite"; }
  /// Return the instersection of allowed values from children
  std::vector<std::string> allowedValues() const override;
  /// Clones this and the children into a new Validator
  IValidator_sptr clone() const override;
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
  /// Returns true if the child list contains a validator of the specified
  /// template type
  template <typename T> bool contains() {
    for (const auto &validator : m_children) {
      // avoid boost::dynamic_pointer cast to avoid constructing
      // a temporary shared_ptr type
      if (dynamic_cast<T *>(validator.get())) {
        return true;
      }
    }
    return false;
  }

private:
  /// Verify the value with the child validators
  std::string check(const boost::any &value) const override;
  /// Verify the value with the child validators with logical "and" relationship
  std::string checkAll(const boost::any &value) const;
  /// Verify the value with the child validators with logical "or" relationship
  std::string checkAny(const boost::any &value) const;
  /// build an error message for OR relations
  std::string buildErrorMessage(const bool valid,
                                const std::string &errors) const;
  /// Private Copy constructor: NO DIRECT COPY ALLOWED
  CompositeValidator(const CompositeValidator &);

  /// A container for the child validators
  std::list<IValidator_sptr> m_children;
  /// Store what relationship child validators have
  const CompositeRelation m_relation;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_COMPOSITEVALIDATOR_H_ */
