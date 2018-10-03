#include "MantidKernel/CompositeValidator.h"
#include <sstream>
#include <unordered_set>

using namespace Mantid::Kernel;

namespace Mantid {
namespace Kernel {
/// Default constructor
CompositeValidator::CompositeValidator(const CompositeRelation &relation)
    : IValidator(), m_children(), m_relation(relation) {}

/// Destructor
CompositeValidator::~CompositeValidator() { m_children.clear(); }

/**
 * The allowed values for the composite validator. This returns
 * the intersection of the allowedValues for the child validators
 * @return
 */
std::vector<std::string> CompositeValidator::allowedValues() const {
  std::unordered_set<std::string> elem_unique;
  std::unordered_multiset<std::string> elem_all;
  // how many validators return non-empty list of allowed values
  int n_combinations(0);
  for (const auto &itr : m_children) {
    std::vector<std::string> subs = itr->allowedValues();
    if (subs.empty())
      continue;
    elem_unique.insert(subs.begin(), subs.end());
    elem_all.insert(subs.begin(), subs.end());
    n_combinations++;
  }
  // empty or single set of allowed values
  if (n_combinations < 2)
    return std::vector<std::string>(elem_unique.begin(), elem_unique.end());
  // there is more then one combination and we have to identify its union;
  for (const auto &its : elem_unique) {
    auto im = elem_all.find(its);
    elem_all.erase(im);
  }
  std::unordered_set<std::string> rez;
  for (const auto &im : elem_all) {
    rez.insert(im);
  }
  return std::vector<std::string>(rez.begin(), rez.end());
}

/**
 * Clone the validator
 * @return A newly constructed validator object. Each child is also cloned
 */
Kernel::IValidator_sptr CompositeValidator::clone() const {
  boost::shared_ptr<CompositeValidator> copy =
      boost::make_shared<CompositeValidator>(m_relation);
  for (const auto &itr : m_children) {
    copy->add(itr->clone());
  }
  return copy;
}

/** Adds a validator to the group of validators to check
 *  @param child :: A pointer to the validator to add
 */
void CompositeValidator::add(Kernel::IValidator_sptr child) {
  m_children.push_back(child);
}

/** Checks the value of all child validators. Fails if any child fails.
 *  @param value :: The workspace to test
 *  @return A user level description of the first problem it finds otherwise ""
 */
std::string CompositeValidator::check(const boost::any &value) const {
  switch (m_relation) {
  case CompositeRelation::AND:
    return checkAll(value);
  case CompositeRelation::OR:
    return checkAny(value);
  default:
    throw std::runtime_error("Unimplemented composite validator relation");
  }
}

std::string CompositeValidator::checkAll(const boost::any &value) const {
  for (const auto &validator : m_children) {
    const auto error = validator->check(value);
    // exit on the first error, to avoid passing doing more tests on invalid
    // objects that could fail
    if (!error.empty())
      return error;
  }
  // there were no errors
  return "";
}

std::string CompositeValidator::checkAny(const boost::any &value) const {
  std::stringstream errorStream;

  // Lambda to check if a validator is valid. If it is not valid then
  // capture its error message to a stream so we can potentially print it out
  // to the user if required.
  const auto checkIfValid = [&errorStream,
                             &value](const IValidator_sptr validator) {
    const auto errorMessage = validator->check(value);
    if (errorMessage.empty()) {
      return true;
    } else {
      // capture error message to possibly later.
      errorStream << errorMessage << "\n";
      return false;
    }
  };

  const auto valid =
      std::any_of(m_children.begin(), m_children.end(), checkIfValid);
  return buildErrorMessage(valid, errorStream.str());
}

/** Return a user friendly error message
 *
 * Returns empty string if no errors were found.
 *
 * @param valid :: whether all the validator succeded
 * @param errors :: the combined list of errors from the child validators
 * @return a user friendly message with all the child validator messages
 * combined.
 */
std::string
CompositeValidator::buildErrorMessage(const bool valid,
                                      const std::string &errors) const {
  if (!valid) {
    return "Invalid property. You must statisfy one of the following "
           "conditions:\n" +
           errors;
  } else {
    // there were no errors
    return "";
  }
}

} // namespace Kernel
} // namespace Mantid
