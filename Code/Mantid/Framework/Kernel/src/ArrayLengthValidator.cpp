#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;

namespace Mantid {
namespace Kernel {

//----------------------------------------------------------------------------------------------
/** No arg constructor
 */
template <typename TYPE>
ArrayLengthValidator<TYPE>::ArrayLengthValidator()
    : TypedValidator<std::vector<TYPE>>(), m_arraySize(size_t(0)),
      m_hasArraySize(false), m_arraySizeMin(size_t(0)),
      m_hasArraySizeMin(false), m_arraySizeMax(size_t(0)),
      m_hasArraySizeMax(false) {}
//----------------------------------------------------------------------------------------------
/** Constructor
 * @param len:: the legth of the array
 */
template <typename TYPE>
ArrayLengthValidator<TYPE>::ArrayLengthValidator(const size_t len)
    : TypedValidator<std::vector<TYPE>>(), m_arraySize(size_t(len)),
      m_hasArraySize(true), m_arraySizeMin(size_t(0)), m_hasArraySizeMin(false),
      m_arraySizeMax(size_t(0)), m_hasArraySizeMax(false) {}
//----------------------------------------------------------------------------------------------
/** Constructor
 * @param lenmin:: the minimum legth of the array
 * @param lenmax:: the maximum legth of the array
 */
template <typename TYPE>
ArrayLengthValidator<TYPE>::ArrayLengthValidator(const size_t lenmin,
                                                 const size_t lenmax)
    : TypedValidator<std::vector<TYPE>>(), m_arraySize(size_t(0)),
      m_hasArraySize(false), m_arraySizeMin(size_t(lenmin)),
      m_hasArraySizeMin(true), m_arraySizeMax(size_t(lenmax)),
      m_hasArraySizeMax(true) {}
//----------------------------------------------------------------------------------------------
/** Destructor
 */
template <typename TYPE> ArrayLengthValidator<TYPE>::~ArrayLengthValidator() {}

/**
  Check if length is set
  @returns true/false
 */
template <typename TYPE> bool ArrayLengthValidator<TYPE>::hasLength() const {
  return this->m_hasArraySize;
}

/**
  Check if minimum length is set
  @returns true/false
 */
template <typename TYPE> bool ArrayLengthValidator<TYPE>::hasMinLength() const {
  return this->m_hasArraySizeMin;
}

/**
  Check if maximum length is set
  @returns true/false
 */
template <typename TYPE> bool ArrayLengthValidator<TYPE>::hasMaxLength() const {
  return this->m_hasArraySizeMax;
}

/**
  Function to retun the set length
  @returns  m_arraySize
 */
template <typename TYPE>
const size_t &ArrayLengthValidator<TYPE>::getLength() const {
  return this->m_arraySize;
}

/**
  Function to retun the set minimum length
  @returns  m_arraySize
 */
template <typename TYPE>
const size_t &ArrayLengthValidator<TYPE>::getMinLength() const {
  return this->m_arraySizeMin;
}

/**
  Function to retun the set maximum length
  @returns  m_arraySize
 */
template <typename TYPE>
const size_t &ArrayLengthValidator<TYPE>::getMaxLength() const {
  return this->m_arraySizeMax;
}

/**
  Function to set the length. It will automatically clear the minimum and
  maximum
  @param  value:: size_t type
 */
template <typename TYPE>
void ArrayLengthValidator<TYPE>::setLength(const size_t &value) {
  this->m_hasArraySize = true;
  this->m_arraySize = value;
  this->clearLengthMax();
  this->clearLengthMin();
}

/**
  Function to set the minimum length. It will automatically clear the set length
  @param  value:: size_t type
 */
template <typename TYPE>
void ArrayLengthValidator<TYPE>::setLengthMin(const size_t &value) {
  this->m_hasArraySizeMin = true;
  this->m_arraySizeMin = value;
  this->clearLength();
}
/**
  Function to set the maximum length. It will automatically clear the set length
  @param  value:: size_t type
 */
template <typename TYPE>
void ArrayLengthValidator<TYPE>::setLengthMax(const size_t &value) {
  this->m_hasArraySizeMax = true;
  this->m_arraySizeMax = value;
  this->clearLength();
}

/**
  Function to unset the length. It sets  m_hasArraySize to false, and the
  m_arraySize to 0
 */
template <typename TYPE> void ArrayLengthValidator<TYPE>::clearLength() {
  this->m_hasArraySize = false;
  this->m_arraySize = size_t(0);
}

/**
  Function to unset the minimum length. It sets  m_hasArraySizeMin to false, and
  the m_arraySizeMin to 0
 */
template <typename TYPE> void ArrayLengthValidator<TYPE>::clearLengthMin() {
  this->m_hasArraySizeMin = false;
  this->m_arraySizeMin = size_t(0);
}

/**
  Function to unset the maximum length. It sets  m_hasArraySizeMax to false, and
  the m_arraySizeMax to 0
 */
template <typename TYPE> void ArrayLengthValidator<TYPE>::clearLengthMax() {
  this->m_hasArraySizeMax = false;
  this->m_arraySizeMax = size_t(0);
}

/**
  Clone function
  @returns a clone of the validator
  */
template <typename TYPE>
IValidator_sptr ArrayLengthValidator<TYPE>::clone() const {
  return boost::make_shared<ArrayLengthValidator>(*this);
}

/**
Private function to check validity
@returns a string. The string is emty if everything is OK, otherwise returns the
error
*/
template <typename TYPE>
std::string ArrayLengthValidator<TYPE>::checkValidity(
    const std::vector<TYPE> &value) const {
  if (this->hasLength() && value.size() != this->m_arraySize) {
    return "Incorrect size";
  }
  if (this->hasMinLength() && value.size() < this->m_arraySizeMin) {
    return "Array size too short";
  }
  if (this->hasMaxLength() && value.size() > this->m_arraySizeMax) {
    return "Array size too long";
  }
  return "";
}

// Required explicit instantiations
template class ArrayLengthValidator<double>;
template class ArrayLengthValidator<int>;
template class ArrayLengthValidator<long>;
template class ArrayLengthValidator<std::string>;
} // namespace Mantid
} // namespace Kernel
