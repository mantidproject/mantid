#include "MantidKernel/VisibleWhenProperty.h"
#include <memory>

namespace Mantid {
namespace Kernel {

/** Constructor
 * @param otherPropName :: Name of the OTHER property that we will check.
 * @param when :: Criterion to evaluate
 * @param value :: For the IS_EQUAL_TO or IS_NOT_EQUAL_TO condition, the value
 * (as string) to check for
 */
VisibleWhenProperty::VisibleWhenProperty(std::string otherPropName,
                                         ePropertyCriterion when,
                                         std::string value)
    : EnabledWhenProperty(otherPropName, when, value) {}

/** Multiple conditions constructor - takes two  VisibleWhenProperty
 * objects and returns the product of them with the specified logic operator.
 *
 * @param conditionOne :: First VisibleWhenProperty object to use
 * @param conditionTwo :: Second VisibleWhenProperty object to use
 * @param logicOperator :: The logic operator to apply across both
 *conditions
 *
 */
VisibleWhenProperty::VisibleWhenProperty(
    const VisibleWhenProperty &conditionOne,
    const VisibleWhenProperty &conditionTwo, eLogicOperator logicOperator)
    : // For the python interface to be able to easily copy objects in
      // Make a deep copy and turn into a shared pointer and forward on
      VisibleWhenProperty(std::make_shared<VisibleWhenProperty>(conditionOne),
                          std::make_shared<VisibleWhenProperty>(conditionTwo),
                          logicOperator) {}

/** Multiple conditions constructor - takes two shared pointers to
 * VisibleWhenProperty objects and returns the product of them
 * with the specified logic operator.
 *
 * @param conditionOne :: First VisibleWhenProperty object to use
 * @param conditionTwo :: Second VisibleWhenProperty object to use
 * @param logicOperator :: The logic operator to apply across both
 *conditions
 *
 */
VisibleWhenProperty::VisibleWhenProperty(
    std::shared_ptr<VisibleWhenProperty> &&conditionOne,
    std::shared_ptr<VisibleWhenProperty> &&conditionTwo,
    eLogicOperator logicOperator)
    : m_comparisonDetails{
          std::make_shared<ComparisonDetails<VisibleWhenProperty>>(
              ComparisonDetails<VisibleWhenProperty>{std::move(conditionOne),
                                                     std::move(conditionTwo),
                                                     logicOperator})} {}

/**
 * Checks if the user specified combination of visible criterion
 * returns a true or false value
 *
 * @param algo :: Pointer to the algorithm containing the property
 * @return true if user specified combination was true.
 * @throw If any problems was found
 */
bool VisibleWhenProperty::checkComparison(const IPropertyManager *algo) const {
  const auto &comparison = m_comparisonDetails;
  const auto &objectOne = comparison->conditionOne;
  const auto &objectTwo = comparison->conditionTwo;

  switch (comparison->logicOperator) {
  case AND:
    return objectOne->isVisible(algo) && objectTwo->isVisible(algo);
    break;
  case OR:
    return objectOne->isVisible(algo) || objectTwo->isVisible(algo);
    break;
  case XOR:
    return objectOne->isVisible(algo) ^ objectTwo->isVisible(algo);
    break;
  default:
    throw std::runtime_error("Unknown logic operator in EnabledWhenProperty");
  }
}

/**
 * Returns true always in VisibleWhenProperty as we only consider
 * visibility case
 *
 * @param algo :: The pointer to the algorithm containing this property
 * @return :: True - Property is always enabled
 */
bool VisibleWhenProperty::isEnabled(const IPropertyManager *algo) const {
  UNUSED_ARG(algo);
  return true;
}

/**
 * Returns true or false depending on whether the property
 * or combination of properties specified satisfies the criterion
 *
 * @param algo :: Pointer to the algorithm containing the property
 * @return :: True if conditions satisfied else false
 * @throw :: On an error condition such as property not being found
 */
bool VisibleWhenProperty::isVisible(const IPropertyManager *algo) const {
  if (m_propertyDetails) {
    return checkCriterion(algo);
  } else if (m_comparisonDetails) {
    return checkComparison(algo);
  } else {
    throw std::runtime_error("Both PropertyDetails and ComparisonDetails were "
                             "null in VisibleWhenProperty");
  }
}

/**
 * Clones the current VisibleWhenProperty object and returns
 * a pointer to the new object. The caller is responsible
 * for deleting this pointer when finished
 *
 * @return Pointer to cloned VisisbleWhenProperty object
 */
IPropertySettings *VisibleWhenProperty::clone() const {
  return new VisibleWhenProperty(*this);
}

} // namespace Kernel
} // namespace Mantid
