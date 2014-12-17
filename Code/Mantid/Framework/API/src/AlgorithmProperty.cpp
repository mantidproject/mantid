//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace API {

//-----------------------------------------------------------------------------
// Public member functions
//-----------------------------------------------------------------------------

/** Constructor
*  @param propName :: The name to assign to the property
*  @param validator :: The validator to use for this property (this class will
* take ownership of the validator)
*  @param direction :: Whether this is a Direction::Input, Direction::Output or
* Direction::InOut (Input & Output) property
*/
AlgorithmProperty::AlgorithmProperty(const std::string &propName,
                                     Kernel::IValidator_sptr validator,
                                     unsigned int direction)
    : Kernel::PropertyWithValue<HeldType>(propName, HeldType(), validator,
                                          direction),
      m_algStr("") {}

/**
* Copy constructor
*/
AlgorithmProperty::AlgorithmProperty(const AlgorithmProperty &rhs)
    : Kernel::PropertyWithValue<HeldType>(rhs) {}

/**
* Copy-Assignment operator
*/
AlgorithmProperty &AlgorithmProperty::operator=(const AlgorithmProperty &rhs) {
  if (&rhs != this) {
    Kernel::PropertyWithValue<HeldType>::operator=(rhs);
  }
  return *this;
}

/**
 * Destructor
 */
AlgorithmProperty::~AlgorithmProperty() {}

/**
 * Return the algorithm as string
 * @returns The algorithm serialized as a string
 */
std::string AlgorithmProperty::value() const { return m_algStr; }

/**
 * Get the default
 * @returns An empty string
 */
std::string AlgorithmProperty::getDefault() const { return ""; }

/**
 * Set value of the algorithm
 * Attempts to create an Algorithm object
 * @param value :: The string format for an algorithm, @see Algorithm::toString
 * @return An empty string if the value is valid, otherwise the string will
 * contain the error
*/
std::string AlgorithmProperty::setValue(const std::string &value) {
  std::string message("");
  try {
    Kernel::PropertyWithValue<IAlgorithm_sptr>::m_value =
        Algorithm::fromString(value);
  } catch (Kernel::Exception::NotFoundError &e) {
    message = e.what();
  } catch (std::runtime_error &e) {
    message = e.what();
  }

  if (message.empty()) {
    m_algStr = value;
    // Check against validators
    return isValid();
  } else
    return message;
}

} // namespace Mantid
} // namespace API
